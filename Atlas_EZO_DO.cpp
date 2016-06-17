/*============================================================================
Atlas Scientific EZO DO sensor library code is placed under the GNU license
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
DO	v2.0
============================================================================*/

#define ATLAS_EZO_DEBUG


//#include <HardwareSerial.h>
#include <Atlas_EZO_DO.h>

/*              DO PUBLIC METHODS                      */

void EZO_DO::initialize() {
	_initialize();
	disableContinuousReadings();
	queryTempComp();
	querySalComp();
	queryPresComp();
	//enableOutput(EZO_DO_OUT_SAT);
	//enableOutput(EZO_DO_OUT_MGL);
	queryOutput();
	if (debug()) Serial.println(F("DO Initialization Done"));

}

ezo_response	setCalibrationAtm();
ezo_response	clearCalibration();


ezo_response EZO_DO::enableOutput(do_output output) {
	return _changeOutput(output,1);
}
ezo_response EZO_DO::disableOutput(do_output output) {
	return _changeOutput(output,0);
}
ezo_response EZO_DO::queryOutput() {
	strncpy(_command,"O,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
																   // _response will be ?O,EC,TDS,S,SG if all are enabled
	if (_result[0] == '?' && _result[1] == 'O' && _result[2] == ',') {
		_sat_output  = TRI_OFF;
		_dox_output = TRI_OFF;
		char * pch;
		pch = strtok(_result+ 3,",\r");
		while ( pch != NULL) {
			if ( !_strCmp(pch,"%"))  _sat_output  = TRI_ON;
			if ( !_strCmp(pch,"DO")) _dox_output = TRI_ON;
			pch = strtok(NULL, ",\r");
		}
	}
	return response;
}

void  EZO_DO::printOutputs(){
	// no need to check _debug here
	Serial.print(F("DO outputs:"));
	if ( _sat_output == TRI_ON ) Serial.print(F("Sat% "));
	else if ( _sat_output == TRI_UNKNOWN )  Serial.print(F("?Sat% "));
	if ( _dox_output == TRI_ON ) Serial.print(F("DOX_mg/l "));
	else if ( _dox_output == TRI_UNKNOWN )  Serial.print(F("?DOX_mg/l "));
	Serial.println();
}
tristate EZO_DO::getOutput(do_output output) {
	switch (output) {
		case EZO_DO_OUT_SAT: return _sat_output;
		case EZO_DO_OUT_MGL: return _dox_output;
		default: return TRI_UNKNOWN;
	}
}


ezo_response EZO_DO::querySingleReading() {
	int8_t width;
	uint8_t precision;
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	bool sat_parsed = false;
	bool dox_parsed = false;
	char * pch;
	pch = strtok(_result,",\r");
	while ( pch != NULL) {
		if ( _dox_output && ! dox_parsed){
			_dox = atof(pch); // convert string to float
			dox_parsed = true;
			width = 8;	precision = 2;
			dtostrf(_dox,width,precision,dox); // Dissolved oxygen in mg/l
			if ( debug() )  {
				Serial.print(F("Raw DO mg/l value: ")); Serial.println(pch);
				Serial.print(F("Dissolved Oxygen is ")); Serial.println(dox);
			}
		}
		else if ( _sat_output && !sat_parsed) {
			_sat = atof(pch);// convert string to float
			sat_parsed = true;
			if ( _sat < 100.0 ) width = 4;
			else width = 5;
			precision = 1;
			dtostrf(_sat,width,precision,sat); // saturation in %
			if ( debug() ) {
				Serial.print(F("Raw Sat.% value ")); Serial.println(pch);
				Serial.print(F("Saturation % is ")); Serial.println(sat);
			}
		}
		pch = strtok(NULL, ",\r");
	}
	return response;
}

ezo_response EZO_DO::setSalComp(uint32_t sal_uS) {
	_sal_uS_comp = sal_uS;
	_sal_ppt_comp = 0.00;
	_command_len = sprintf(_command,"S,%lu\r",sal_uS);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_DO::setSalPPTComp(float sal_ppt) {
	_sal_uS_comp = 0;
	_sal_ppt_comp = sal_ppt;
	_command_len = sprintf(_command,"S,%4.1f,PPT\r",(double)sal_ppt);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_DO::querySalComp(){
	strncpy(_command,"S,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true,true);
	// _result should be in the format "?S,<sal_us>,<uS|ppt>\r" // wrong in documentation
	if ( debug() )  Serial.print(F("Salinity Compensation set to:"));
	if ( _result[0] == '?' && _result[1] == 'S' && _result[2] == ',' ) {
		char * pch;
		char temp_sal_comp[10];
		pch = strtok(_result+ 3,",\r"); // value
		strncpy(temp_sal_comp,pch,10);
		pch = strtok(NULL, ",\r"); // "us" or "ppt"
		if ( !_strCmp(pch,"uS")){
			_sal_uS_comp = atoi(temp_sal_comp);
			_sal_ppt_comp = 0;
			if ( debug() ) { Serial.print(_sal_uS_comp);	Serial.println(" uS");}
		}
		else if ( !_strCmp(pch,"ppt")) {
			_sal_uS_comp = 0;
			_sal_ppt_comp = atof(temp_sal_comp);
			if ( debug() ) {Serial.print(_sal_ppt_comp);	Serial.println(" ppt");}
		}
	} 
	return response;
}


ezo_response EZO_DO::setPresComp(float pressure_kpa) {
	// This parameter can be omitted if the water is less than 10 meters deep.
	_command_len = sprintf(_command,"P,%6.2f\r",(double)pressure_kpa);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_DO::queryPresComp(){
	strncpy(_command,"P,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true,true);
	// _result should be in the format "?T,<temp_C>\r"
	_temp_comp = EZO_EC_DEFAULT_TEMP;
	if ( _result[0] == '?' && _result[1] == 'P' ) {
		_temp_comp = atof(_result + 3);
	}
	if ( debug() ) { Serial.print(F("Pressure Compensation set to:")); Serial.println(_temp_comp);}
	return response;
}


/*              DO PRIVATE METHODS                      */
ezo_response EZO_DO::_changeOutput(do_output output,int8_t enable_output) {
	// format is "O,[parameter],[0|1]\r"
	uint8_t PARAMETER_LEN = 10;
	char parameter[PARAMETER_LEN];
	switch (output) {
		case EZO_DO_OUT_SAT:
			strncpy(parameter,"%",PARAMETER_LEN); break;
		case EZO_DO_OUT_MGL:
			strncpy(parameter,"DO",PARAMETER_LEN); break;
		default: return EZO_RESPONSE_UK;
	}
	_command_len = sprintf(_command,"O,%s,%d\r",parameter,enable_output);
	return _sendCommand(_command,false,true);
}
