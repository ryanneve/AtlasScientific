/*============================================================================
Atlas Scientific RGB sensor library code is placed under the GNU license
Copyright (c) 2015 Ryan Neve <Ryan@PlanktosInstruments.com>

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
	
	Based on Atlas Scientific datasheets:
		RGB	v1.6
============================================================================*/
#include <arduino.h>
#include <HardwareSerial.h>
#include <AtlasRGB.h>



/*               PUBLIC METHODS                      */

void RGB::initialize(){
	disableContinuousReadings();
	queryInfo();
}

tristate RGB::querySingleReading(){
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	_sendCommand(_command,true);
	if ( _debug ) {
		Serial.print("qSR got _result: "); Serial.println(_result);
		Serial.print("_mode is: ");
		if ( _rgb_mode == RGB_UNKNOWN) Serial.println("?");
		if ( _rgb_mode == RGB_DEFAULT) Serial.println("RGB");
		if ( _rgb_mode == RGB_LUX) Serial.println("lx");
		if ( _rgb_mode == RGB_ALL) Serial.println("ALL");
	}
	// now parse _result
	// response will depend on _mode.	
	bool _red_parsed = false;
	bool _green_parsed = false;
	bool _blue_parsed = false;
	bool _lx_red_parsed = false;
	bool _lx_green_parsed = false;
	bool _lx_blue_parsed = false;
	bool _lx_total_parsed = false;
	bool _lx_beyond_parsed = false;
	_saturated = false;
	char * pch;
	pch = strtok(_result,",\r");
	bool parse_success;
	while ( pch != NULL) {
		parse_success = false;
		if ( _rgb_mode == RGB_DEFAULT || _rgb_mode == RGB_ALL ){
			if ( ! _red_parsed ){
				strncpy(red,pch,RGB_DATA_LEN);
				_red = atoi(red);
				_red_parsed = true;
				parse_success = true;
			}
			else if ( ! _green_parsed ){
				strncpy(green,pch,RGB_DATA_LEN);
				_green = atoi(green);
				_green_parsed = true;
				parse_success = true;
			}
			else if ( ! _blue_parsed ){
				strncpy(blue,pch,RGB_DATA_LEN);
				_blue = atoi(red);
				_blue_parsed = true;
				parse_success = true;
			}
		}
		if ( !parse_success && ( _rgb_mode == RGB_LUX || _rgb_mode == RGB_ALL )){
			if ( ! _lx_red_parsed){
				strncpy(lx_red,pch,RGB_DATA_LEN);
				_lx_red = atoi(lx_red);
				_lx_red_parsed = true;
				parse_success = true;
			}
			else if ( ! _lx_green_parsed){
				strncpy(lx_green,pch,RGB_DATA_LEN);
				_lx_green = atoi(lx_green);
				_lx_green_parsed = true;
				parse_success = true;
			}
			else if ( ! _lx_blue_parsed){
				strncpy(lx_blue,pch,RGB_DATA_LEN);
				_lx_blue = atoi(lx_blue);
				_lx_blue_parsed = true;
				parse_success = true;
			}
			else if ( ! _lx_total_parsed){
				strncpy(lx_total,pch,RGB_DATA_LEN);
				_lx_total = atoi(lx_total);
				_lx_total_parsed = true;
				parse_success = true;
			}
			else if ( ! _lx_beyond_parsed){
				strncpy(lx_beyond,pch,RGB_DATA_LEN);
				_lx_beyond = atoi(lx_beyond);
				_lx_beyond_parsed = true;
				parse_success = true;
			}
		}
		if ( pch[0] == '*' ){
			_saturated = true;
			parse_success = true;
		}
		pch = strtok(NULL, ",\r");
	}
	return TRI_ON;
}


void RGB::enableContinuousReadings() {
	strncpy(_command,"C\r",ATLAS_COMMAND_LENGTH);
	_sendCommand(_command,false);
}

void RGB::disableContinuousReadings() {
	strncpy(_command,"E\r",ATLAS_COMMAND_LENGTH);
	_sendCommand(_command,false);
	delay(1100); // Time for one last set of values
	flushSerial();
}
 
tristate RGB::setMode(rgb_mode mode) {
	tristate result = TRI_UNKNOWN;
	_rgb_mode = mode;
	sprintf(_command,"M%d\r",mode);
	if ( _debug ) { Serial.print("Setting RGB Mode with command ");	Serial.println(_command);}
	_sendCommand(_command,true);
	// The ENV-RGB will respond:  "[RGB|lx|RGB+lx]\r"
	if ( !_strCmp(_result,"RGB") && mode == RGB_DEFAULT)		result = TRI_ON;
	else if ( !_strCmp(_result,"lx") && mode == RGB_LUX)		result = TRI_ON;
	else if ( !_strCmp(_result,"RGB+lx") && mode == RGB_ALL)	result = TRI_ON;
	else result = TRI_OFF;
	return result;
}
tristate RGB::queryInfo(){
	tristate result = TRI_UNKNOWN;
	strncpy(_command,"I\r",ATLAS_COMMAND_LENGTH);
	if ( _debug )  {Serial.print("Querying RGB info with command ");	Serial.println(_command);}
	_sendCommand(_command,true);
	// The ENV-RGB will respond:  "C,V<version>,<date>\r". C is for Color.
	char * pch;
	pch = strtok(_result,",\r");
	if (pch[0] == 'C'){
		result = TRI_ON;
		pch = strtok(NULL, ",\r");
		if ( pch[0] == 'V') {
			// Parse version
			strncpy(_firmware_version,pch,sizeof(_firmware_version));
		}
		pch = strtok(NULL, ",\r");
		strncpy(_firmware_date,pch,sizeof(_firmware_version));
	}
	else result = TRI_OFF;
	return result;
}

/*              PRIVATE METHODS                      */

void RGB::_sendCommand(char * command, bool has_result){
	_sendCommand(command,has_result,DEFAULT_COMMAND_DELAY);
}
void RGB::_sendCommand(char * command, bool has_result,uint16_t result_delay){
	if ( online() ) Serial_AS->print(command);
	if ( has_result ) {
		int16_t byte_found = _delayUntilSerialData(10000);
		if ( byte_found == -1 ) Serial.println("No data found while waiting for result");
		_getResult(result_delay);
	}
}