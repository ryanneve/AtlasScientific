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
RGB	v??
============================================================================*/

#define ATLAS_EZO_DEBUG


//#include <HardwareSerial.h>
#include <Atlas_EZO_RGB.h>
/*              RGB PUBLIC METHODS                      */
void EZO_RGB::initialize() {
	_initialize(); // Generic EZO initialization
				   // Now some EZO_RGB specifics
				   // LEDs default to:
				   // _brightness = 0 and _auto_bright  = TRI_ON
				   // proximity detection disabled and ir brightness low(1)
	initialize(0,TRI_ON,0,1);
	if (debug()) Serial.println(F("RGB Initialization Done"));
}

void EZO_RGB::initialize(int8_t brightness,tristate auto_bright,int16_t prox_distance, int8_t ir_brightness){
	queryLEDbrightness();
	if ( _brightness != brightness || _auto_bright != auto_bright) setLEDbrightness(brightness,auto_bright);
	// proximity defaults to auto disabled and LEDs at high
	queryProximity();
	if (_prox_distance != prox_distance) {
		if ( prox_distance == 0 ) disableProximity();
		else if ( prox_distance == 1 ) enableProximity();
		else enableProximity(prox_distance);
	}
	if (_IR_bright != ir_brightness) {
		switch (ir_brightness) {
			case 1: proximityLED_Low(); break;
			case 2: proximityLED_Med(); break;
			default: proximityLED_High();
		}
	}
} 

ezo_response EZO_RGB::querySingleReading()  {
	enum parsing_modes {PARSING_RGB,PARSING_PROX,PARSING_LUX,PARSING_CIE};
	parsing_modes parsing_data = PARSING_RGB;
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,4000,true); // with 2 sec timeout
																   // Response is a comma delimited set of numbers which end in "\r". There may be up to 6 parameters in the following order:
																   // [R,G,B,][P,<prox>,][Lux,<lux>,][xyY,<CIE_x>,<CIE_y>,<CIE_Y>]. The format of the output is determined by queryOutput() and saved in _xx_output.
	if (debug()) {Serial.print(F("Parsing :")); Serial.println(_result);}
	char * pch;
	pch = strtok(_result,",\r");
	while ( pch != NULL) {
		//Serial.print("  parsing:"); Serial.print(pch);
		if		( !_strCmp(pch,"xyY,") ) parsing_data = PARSING_CIE;
		else if ( !_strCmp(pch,"Lux,") ) parsing_data = PARSING_LUX;
		else if ( !_strCmp(pch,"P,"  ) ) parsing_data = PARSING_PROX;
		else parsing_data = PARSING_RGB; // Which for some reason has no preceding tag.
		switch (parsing_data){
			case PARSING_RGB:
				//Serial.println(" = RGB");
				// Should already have red value
				strncpy(red,pch,sizeof(red));
				_red = atoi(pch);			// Convert parsed string to integer 
				pch = strtok(NULL, ",\r");	// Next value (green)
				strncpy(green,pch,sizeof(green));
				_green = atoi(pch);
				pch = strtok(NULL, ",\r");	// Next value (blue)
				strncpy(blue,pch,sizeof(blue));
				_blue = atoi(pch); 
				break;
			case PARSING_PROX:
				//Serial.println(" = PROX");
				pch = strtok(NULL, ",\r");	 // Get next value
				strncpy(prox,pch,sizeof(prox));
				_prox = atoi(prox); // Convert parsed string to integer
				break;
			case PARSING_LUX:
				//Serial.println(" = LUX");
				pch = strtok(NULL, ",\r");	 // Get next value
				strncpy(lux,pch,sizeof(lux));
				_lux = atoi(lux); // Convert parsed string to float attribute
				break;
			case PARSING_CIE:
				//Serial.println(" = CIE");
				// two floats then an int
				pch = strtok(NULL, ",\r");	 // Get next value
				strncpy(cie_x,pch,sizeof(cie_x));
				_cie_x = atof(cie_x); // Convert parsed string to float attribute
				pch = strtok(NULL, ",\r");	
				strncpy(cie_y,pch,sizeof(cie_y));
				_cie_y = atof(cie_y);
				pch = strtok(NULL, ",\r");	 // Get next value
				strncpy(cie_Y,pch,sizeof(cie_Y));
				_cie_Y = atoi(cie_Y); // Convert parsed string to integer
				break;
		}
		pch = strtok(NULL, ",\r");
	}
	return response;
}

ezo_response EZO_RGB::queryOutput() {
	strncpy(_command,"O,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
																   // _response will be ?O,[RGB,][PROX,][LUX,][CIE] if all are enabled
	if (debug()) {Serial.print(F("RGB Parsing:"));Serial.println(_result);}
	char * pch;
	pch = strtok(_result,",\r");
	if (!_strCmp(pch,"?O,")) {
		_rgb_output  = TRI_OFF;
		_prox_output = TRI_OFF;
		_lux_output  = TRI_OFF;
		_cie_output  = TRI_OFF;
		while ( pch != NULL) {
			if		( !_strCmp(pch,"RGB"))	_rgb_output  = TRI_ON;
			else if ( !_strCmp(pch,"PROX"))	_prox_output = TRI_ON;
			else if ( !_strCmp(pch,"LUX"))	_lux_output  = TRI_ON;
			else if ( !_strCmp(pch,"CIE"))	_cie_output  = TRI_ON;
			pch = strtok(NULL, ",\r");
		}
	}
	return response;
}
void  EZO_RGB::printOutputs(){
	// No need to check _debug here
	Serial.print(F("RGB outputs:"));
	if ( _rgb_output == TRI_ON ) Serial.print("RGB ");
	else if ( _rgb_output == TRI_UNKNOWN )  Serial.print("?RGB ");
	if ( _prox_output == TRI_ON ) Serial.print("PROX ");
	else if ( _prox_output == TRI_UNKNOWN )  Serial.print("?PROX ");
	if ( _lux_output == TRI_ON )   Serial.print("LUX ");
	else if ( _lux_output == TRI_UNKNOWN )  Serial.print("?LUX ");
	if ( _cie_output == TRI_ON )  Serial.print("CIE ");
	else if ( _cie_output == TRI_UNKNOWN )  Serial.print("?CIE ");
	Serial.println();
}
tristate EZO_RGB::getOutput(ezo_rgb_output output) {
	switch (output) {
		case EZO_RGB_OUT_RGB:	return _rgb_output;
		case EZO_RGB_OUT_PROX:	return _prox_output;
		case EZO_RGB_OUT_LUX:	return _lux_output;
		case EZO_RGB_OUT_CIE:	return _cie_output;
		default:				return TRI_UNKNOWN;
	}
}
ezo_response EZO_RGB::enableOutput(ezo_rgb_output output) {
	return _changeOutput(output,1);
}
ezo_response EZO_RGB::disableOutput(ezo_rgb_output output) {
	return _changeOutput(output,0);
}

ezo_response EZO_RGB::setLEDbrightness(int8_t brightness){
	return setLEDbrightness(brightness,true);
}
ezo_response EZO_RGB::setLEDbrightness(int8_t brightness,tristate auto_led) {
	// convert tristate to boolean then call setLEDbrightness(int8_t brightness,bool auto_led)
	bool auto_led_bool;
	if ( auto_led == TRI_ON ) auto_led_bool = true;
	else auto_led_bool = false;
	return setLEDbrightness(brightness,auto_led_bool);
}
ezo_response EZO_RGB::setLEDbrightness(int8_t brightness,bool auto_led) {
	// Set LED brightness from 0 to 100
	// Response is "L,%[,T]<CR>"
	ezo_response brightness_result = EZO_RESPONSE_UK;
	if ( auto_led ) _command_len = sprintf(_command,"L,%d,T\r",brightness);
	else _command_len = sprintf(_command,"L,%d\r",brightness);
	if ( debug() ) { Serial.print(F("Setting LED to ")); Serial.println(brightness); }
	brightness_result  = _sendCommand(_command,true,true);
	if (brightness_result == EZO_RESPONSE_OK) _brightness = brightness; // We can probably make this assumption.
	return brightness_result;
}
ezo_response EZO_RGB::queryLEDbrightness() {
	// Find out what LED brightness is. Call getLEDbrightness() for value
	// Response is:?L,<%>[,T]<CR>
	strncpy(_command,"L,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	if (debug()) {Serial.print(F("RGB Parsing LED:"));Serial.println(_result);}
	char * pch;
	pch = strtok(_result,",\r");
	if (!_strCmp(pch,"?L,")) {
		pch = strtok(NULL, ",\r");
		_brightness = atoi(pch);
		pch = strtok(NULL, ",\r");
		if ( pch[0] == 'T' ) _auto_bright = TRI_ON;
		else _auto_bright = TRI_OFF;
	}
	return response;
}

ezo_response EZO_RGB::disableProximity(){
	strncpy(_command,"P,0\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::enableProximity(){
	strncpy(_command,"P,1\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::enableProximity(int16_t distance){
	//make sure we're in range. MAY NOT BE NECESSARY
	_command_len = sprintf(_command,"P,%d\r",distance);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::proximityLED_Low(){
	strncpy(_command,"P,L\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::proximityLED_Med(){
	strncpy(_command,"P,M\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::proximityLED_High(){
	strncpy(_command,"P,H\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::queryProximity(){
	strncpy(_command,"P,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// Response is:
	// ?P,<distance>,<LED_power>
	// Where distance = 0,2-1023 and LED_power = H|M|L
	if (debug()) {Serial.print(F("EZO_RGB Prox Parsing:"));Serial.println(_result);}
	char * pch;
	pch = strtok(_result,",\r");
	if (!_strCmp(pch,"?P,")) {
		pch = strtok(NULL, ",\r");
		_prox_distance = atoi(pch);
		pch = strtok(NULL, ",\r");
		if ( pch[0] == 'H' ) _IR_bright = 3;
		else if ( pch[0] == 'M' ) _IR_bright = 2;
		else if ( pch[0] == 'L' ) _IR_bright = 1;
		else _IR_bright = 0;
	}
	return response;
}

ezo_response EZO_RGB::enableMatching(){
	strncpy(_command,"M,1\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::disableMatching(){
	strncpy(_command,"M,0\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::queryMatching(){
	strncpy(_command,"M,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// Response is:
	// ?M,<matching><CR>
	// Where matching = 0 or 1
	if (debug()) {Serial.print(F("EZO_RGB matching Parsing:"));Serial.println(_result);}
	char * pch;
	pch = strtok(_result,",\r");
	if (!_strCmp(pch,"?M,")) {
		pch = strtok(NULL, ",\r");
		if ( pch[0] == '0' ) _matching = TRI_OFF;
		else if ( pch[0] == '1' ) _matching = TRI_ON;
		else _matching = TRI_UNKNOWN;
	}
	return response;
}

ezo_response EZO_RGB::setGamma(float gamma_correction){
	_command_len = sprintf(_command,"g,%4.3f\r",(double)gamma_correction);
	return _sendCommand(_command,false,true);
}
ezo_response EZO_RGB::queryGamma(){
	strncpy(_command,"G,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// Response is:
	// ?G,<gamma><CR>
	// Where gamma = 0.01 to 4.99
	if (debug()) {Serial.print(F("EZO_RGB gamma Parsing:"));Serial.println(_result);}
	char * pch;
	pch = strtok(_result,",\r");
	if (!_strCmp(pch,"?G,")) {
		pch = strtok(NULL, ",\r");
		_gamma_correction = atof(pch);
	}
	return response;
}
/*              RGB PRIVATE  METHODS                      */

ezo_response EZO_RGB::_changeOutput(ezo_rgb_output output,int8_t enable_output) {
	// format is "O,[parameter],[0|1]\r"
	uint8_t PARAMETER_LEN = 10;
	char parameter[PARAMETER_LEN];
	switch (output) {
		case EZO_RGB_OUT_RGB:
			strncpy(parameter,"RGB",PARAMETER_LEN); break;
		case EZO_RGB_OUT_PROX:
			strncpy(parameter,"PROX",PARAMETER_LEN); break;
		case EZO_RGB_OUT_LUX:
			strncpy(parameter,"LUX",PARAMETER_LEN); break;
		case EZO_RGB_OUT_CIE:
			strncpy(parameter,"CIE",PARAMETER_LEN); break;
		default:
			break;
	}
	_command_len = sprintf(_command,"O,%s,%d\r",parameter,enable_output);
	return _sendCommand(_command,false,true);
}