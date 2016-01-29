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
		NOTE THAT THIS IS NOT FOR THE EZO_RGB sensor
============================================================================*/
//#define ATLAS_RGB_DEBUG

#define NO_SENSOR_DATA		-999	// This means we didn't want the data based on configuration
#define NO_SENSOR_COMMS		-888	// Couldn't communicate with sensor
#define SENSOR_COMMS_FAILED	-777	// Communications with sensor failing.

#include <HardwareSerial.h>
#include <AtlasRGB.h>



/*               PUBLIC METHODS                      */

void RGB::initialize(){
	initialize(RGB_UNKNOWN);
}

void RGB::initialize(rgb_mode mode){
	// initialize to NO_SENSOR_DATA beacause we may never query some of these based on configuration.
	_red		= NO_SENSOR_DATA;
	_blue		= NO_SENSOR_DATA;
	_green		= NO_SENSOR_DATA;
	_lx_red		= NO_SENSOR_DATA;
	_lx_blue	= NO_SENSOR_DATA;
	_lx_green	= NO_SENSOR_DATA;
	_lx_total	= NO_SENSOR_DATA;
	_lx_beyond	= NO_SENSOR_DATA;
	flushSerial();
	disableContinuousReadings();
	queryInfo();
	if ( connected() &&  mode != RGB_UNKNOWN) setMode(mode);
}

tristate RGB::querySingleReading(){
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH); // This is the single reading command
	_sendCommand(_command,true);
#ifdef ATLAS_RGB_DEBUG
	Serial.print(F("qSR got _result: ")); Serial.println(_result);
	Serial.print(F("_mode is: "));
	if ( _rgb_mode == RGB_UNKNOWN) Serial.println("?");
	if ( _rgb_mode == RGB_DEFAULT) Serial.println("RGB");
	if ( _rgb_mode == RGB_LUX) Serial.println("lx");
	if ( _rgb_mode == RGB_ALL) Serial.println("ALL");
#endif
	uint8_t min_len;
	if ( _rgb_mode == RGB_DEFAULT ) min_len = 5; // "0,0,0" is shortest possible
	if ( _rgb_mode == RGB_LUX ) min_len = 9;
	if ( _rgb_mode == RGB_ALL ) min_len = 15;
	if ( _result_len < min_len ){ // Didn't get anything appropriate from RGB sensor.
		int16_t sensor_status;
		if ( connected() ) sensor_status = SENSOR_COMMS_FAILED;
		else sensor_status = NO_SENSOR_COMMS;
		_red		= sensor_status;
		_blue		= sensor_status;
		_green		= sensor_status;
		_lx_red		= sensor_status;
		_lx_blue	= sensor_status;
		_lx_green	= sensor_status;
		_lx_total	= sensor_status;
		_lx_beyond	= sensor_status;
		return TRI_OFF;
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
#ifdef ATLAS_RGB_DEBUG
	if ( debug() ) { Serial.print(F("Setting RGB Mode with command "));	Serial.println(_command);}
#endif
	_sendCommand(_command,true);
	// The ENV-RGB will respond:  "[RGB|lx|RGB+lx]\r"
	if ( !_strCmp(_result,"RGB") && mode == RGB_DEFAULT){
		result = TRI_ON;
		_lx_red		= NO_SENSOR_DATA;
		_lx_blue	= NO_SENSOR_DATA;
		_lx_green	= NO_SENSOR_DATA;
		_lx_total	= NO_SENSOR_DATA;
		_lx_beyond	= NO_SENSOR_DATA;
	}
	else if ( !_strCmp(_result,"lx") && mode == RGB_LUX) {
		result = TRI_ON;
		_red		= NO_SENSOR_DATA;
		_blue		= NO_SENSOR_DATA;
		_green		= NO_SENSOR_DATA;
	}
	else if ( !_strCmp(_result,"RGB+lx") && mode == RGB_ALL) {
		result = TRI_ON;
		_red		= NO_SENSOR_DATA;
		_blue		= NO_SENSOR_DATA;
		_green		= NO_SENSOR_DATA;
		_lx_red		= NO_SENSOR_DATA;
		_lx_blue	= NO_SENSOR_DATA;
		_lx_green	= NO_SENSOR_DATA;
		_lx_total	= NO_SENSOR_DATA;
		_lx_beyond	= NO_SENSOR_DATA;
	}
	else result = TRI_OFF;
	return result;
}
tristate RGB::queryInfo(){
	tristate result = TRI_UNKNOWN;
	strncpy(_command,"I\r",ATLAS_COMMAND_LENGTH);
#ifdef ATLAS_RGB_DEBUG
	if ( debug() )  {Serial.print(F("Querying RGB info with command "));	Serial.println(_command);}
#endif
	_sendCommand(_command,true);
	// The ENV-RGB will respond:  "C,V<version>,<date>\r". C is for Color.
	char * pch;
	pch = strtok(_result,",\r");
	if (pch[0] == 'C'){
		pch = strtok(NULL, ",\r");
		if ( pch[0] == 'V') {
			// Parse version
			result = TRI_ON;
			_setConnected();
			strncpy(_firmware_version,pch,sizeof(_firmware_version));
		}
		pch = strtok(NULL, ",\r");
		strncpy(_firmware_date,pch,sizeof(_firmware_version));
	}
	else {
		result = TRI_OFF;
#ifdef ATLAS_RGB_DEBUG
		if ( debug() ) Serial.println(F("Unable to retrieve RGB Info."));
#endif
	}
	return result;
}

/*              PRIVATE METHODS                      */

void RGB::_sendCommand(char * command, bool has_result){_sendCommand(command,has_result,DEFAULT_COMMAND_DELAY);}
void RGB::_sendCommand(char * command, bool has_result,uint16_t result_delay){
	if ( online() ) Serial_AS->print(command);
	if ( has_result ) {
		if ( _delayUntilSerialData(10000) == -1 ){
#ifdef ATLAS_RGB_DEBUG
			Serial.println(F("No data found while waiting for result"));
#endif
		}
		_getResult(result_delay);
	}
}