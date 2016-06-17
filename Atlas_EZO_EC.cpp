/*============================================================================
Atlas Scientific EZO EC sensor library code is placed under the GNU license
Copyright (c) 2016 Ryan Neve <Ryan@PlanktosInstruments.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Based on Atlas Scientific EZO datasheets:
EC	v2.4
============================================================================*/

#define ATLAS_EZO_DEBUG


//#include <HardwareSerial.h>
#include <Atlas_EZO_EC.h>

/*              EC PUBLIC METHODS                      */

void EZO_EC::initialize() {
	_initialize();
	queryCalibration();
	queryK();
	queryTempComp();
	queryOutput();
	if (debug()) Serial.println(F("EC Initialization Done"));
}

ezo_response EZO_EC::calibrate(ezo_ec_calibration_command command,uint32_t ec_standard) {
	// NOT YET TESTED
	ezo_response response = EZO_RESPONSE_UK;
	switch ( command ){
		case EZO_EC_CAL_CLEAR:	snprintf(_command,ATLAS_COMMAND_LENGTH,"Cal,clear\r");	ec_standard = 1;	break;
		case EZO_EC_CAL_DRY:	snprintf(_command,ATLAS_COMMAND_LENGTH,"Cal,dry\r");	ec_standard = 1;	break;
		case EZO_EC_CAL_ONE:	snprintf(_command,ATLAS_COMMAND_LENGTH,"Cal,one,%lu\r",ec_standard);		break;
		case EZO_EC_CAL_LOW:	snprintf(_command,ATLAS_COMMAND_LENGTH,"Cal,low,%lu\r",ec_standard);		break;
		case EZO_EC_CAL_HIGH:	snprintf(_command,ATLAS_COMMAND_LENGTH,"Cal,high,%lu\r",ec_standard);		break;
		case EZO_EC_CAL_QUERY:	response = queryCalibration(); ec_standard = 0;	break;
		default:			ec_standard = 0;	break;
	}
	if ( ec_standard ) response = _sendCommand(_command,false,true);
	return response;
}

ezo_response EZO_EC::setK(float k) {
	_command_len = sprintf(_command,"K,%4.1f\r",(double)k);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_EC::queryK() {
	strncpy(_command,"K,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// _result will be "?K,<floating point K number>\r"
	if ( _result[0] == '?' && _result[1] == 'K') {
		// parse k
		_k = atof(_result + 3);
		if ( debug() ) { Serial.print(F("EC K value is:")); Serial.println(_k);}
	}
	return response;
}
ezo_response EZO_EC::enableOutput(ezo_ec_output output) {
	return _changeOutput(output,1);
}
ezo_response EZO_EC::disableOutput(ezo_ec_output output) {
	return _changeOutput(output,0);
}
ezo_response EZO_EC::queryOutput() {
	strncpy(_command,"O,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
																   // _response will be ?O,EC,TDS,S,SG if all are enabled
	if (debug())  {Serial.print(F("EC Parsing:"));Serial.println(_result);}
	char * pch;
	pch = strtok(_result,",\r");
	if (!_strCmp(pch,"?O,")) {
		_ec_output  = TRI_OFF;
		_tds_output = TRI_OFF;
		_s_output   = TRI_OFF;
		_sg_output  = TRI_OFF;
		while ( pch != NULL) {
			if ( !_strCmp(pch,"EC"))  _ec_output  = TRI_ON;
			else if ( !_strCmp(pch,"TDS")) _tds_output = TRI_ON;
			else if ( !_strCmp(pch,"S"))   _s_output   = TRI_ON;
			else if ( !_strCmp(pch,"SG"))  _sg_output  = TRI_ON;
			pch = strtok(NULL, ",\r");
		}
	}
	return response;
}
void  EZO_EC::printOutputs(){
	// No need to check _debug here
	Serial.print(F("EC outputs:"));
	if ( _ec_output == TRI_ON ) Serial.print(F("EC "));
	else if ( _ec_output == TRI_UNKNOWN )  Serial.print(F("?EC "));
	if ( _tds_output == TRI_ON ) Serial.print(F("TDS "));
	else if ( _tds_output == TRI_UNKNOWN )  Serial.print(F("?TDS "));
	if ( _s_output == TRI_ON )   Serial.print(F("S "));
	else if ( _s_output == TRI_UNKNOWN )  Serial.print(F("?S "));
	if ( _sg_output == TRI_ON )  Serial.print(F("SG "));
	else if ( _sg_output == TRI_UNKNOWN )  Serial.print(F("?SG "));
	Serial.println();
}
tristate EZO_EC::getOutput(ezo_ec_output output) {
	switch (output) {
		case EZO_EC_OUT_EC: return _ec_output;
		case EZO_EC_OUT_TDS: return _tds_output;
		case EZO_EC_OUT_S: return _s_output;
		case EZO_EC_OUT_SG: return _sg_output;
		default: return TRI_UNKNOWN;
	}
}
ezo_response EZO_EC::querySingleReading() {
	int8_t width;
	uint8_t precision;
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
																   // Response starts "EC," and ends in "\r". There may be up to 4 parameters in the following order:
																   // EC,TDS,SAL,SG. The format of the output is determined by queryOutput() and saved in _xx_output.
	bool ec_parsed = false;
	bool tds_parsed = false;
	bool sal_parsed = false;
	bool sg_parsed = false;
	//Serial.print("Parsing :"); Serial.println(_result);
	char * pch;
	pch = strtok(_result,",\r");
	while ( pch != NULL) {
		//Serial.print("  part:"); Serial.println(pch);
		if ( _ec_output && !ec_parsed) {
			_ec = atof(pch); // Convert parsed string to float attribute
			ec_parsed = true;
			if ( _ec <= 999.9 ) width = 5;
			else if ( _ec >= 1000 && _ec <= 9999 ) width = 4;
			else if ( _ec >= 10000  && _ec <= 99990 ) width = 5;
			else width = 6; // 100,000+
			if ( _ec <= 99.99 ) precision = 2;
			else if ( _ec <= 999.9 ) precision = 1;
			else precision = 0; // 1000+
			dtostrf(_ec,width,precision,ec); // Save to ec char array for easier logging.
											 //Serial.print("EC= "); Serial.print(_ec); Serial.print(" = "); Serial.println(ec);
		}
		else if ( _tds_output && ! tds_parsed){
			_tds = atof(pch);
			tds_parsed = true;
			width = 6;
			precision = 1;
			dtostrf(_tds,width,precision,tds);
		}
		else if ( _s_output && ! sal_parsed){
			_sal = atof(pch);
			sal_parsed = true;
			width = 7;
			precision = 2;
			dtostrf(_sal,width,precision,sal);
		}
		else if ( _sg_output && ! sg_parsed){
			_sg = atof(pch);
			sg_parsed = true;
			if ( _sg < 10.00 ) {
				width = 5; precision = 3;
			}
			else {
				width = 7;
				precision = 2;
			}
			dtostrf(_sg,width,precision,sg);
		}
		pch = strtok(NULL, ",\r");
	}
	return response;
}

/*              EC PRIVATE  METHODS                      */

ezo_response EZO_EC::_changeOutput(ezo_ec_output output,int8_t enable_output) {
	// format is "O,[parameter],[0|1]\r"
	uint8_t PARAMETER_LEN = 10;
	char parameter[PARAMETER_LEN];
	switch (output) {
		case EZO_EC_OUT_EC:
			strncpy(parameter,"EC",PARAMETER_LEN); break;
		case EZO_EC_OUT_TDS:
			strncpy(parameter,"TDS",PARAMETER_LEN); break;
		case EZO_EC_OUT_S:
			strncpy(parameter,"S",PARAMETER_LEN); break;
		case EZO_EC_OUT_SG:
			strncpy(parameter,"SG",PARAMETER_LEN); break;
		default: return EZO_RESPONSE_UK;
	}
	_command_len = sprintf(_command,"O,%s,%d\r",parameter,enable_output);
	return _sendCommand(_command,false,true);
}