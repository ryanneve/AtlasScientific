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
#include <AtlasScientific/AtlasRGB.h>



/*               PUBLIC METHODS                      */


void RGB::begin(HardwareSerial *serial,uint32_t baud_rate) {
	_baud_rate = baud_rate;
	Serial_RGB = serial;
	begin();
}
void RGB::begin() {
	Serial_RGB->begin(_baud_rate);
	flushSerial();
}

void RGB::initialize(){
	disableContinuousReadings();
	queryStatus();
}

tristate RGB::querySingleReading(){
	strncpy(_command,"R\r",RGB_COMMAND_LENGTH);
	_sendCommand(_command,true);
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
	while ( pch != NULL) {
		if ( pch[0] == '*' ){
			_saturated = true;
		}
		if ( _mode == RGB_DEFAULT || _mode == RGB_ALL ){
			if ( ! _red_parsed ){
				strncpy(red,pch,RGB_DATA_LEN);
				_red = atoi(red);
				_red_parsed = true;
			}
			else if ( ! _green_parsed ){
				strncpy(green,pch,RGB_DATA_LEN);
				_green = atoi(green);
				_green_parsed = true;
			}
			else if ( ! _blue_parsed ){
				strncpy(blue,pch,RGB_DATA_LEN);
				_blue = atoi(red);
				_blue_parsed = true;
			}
		}
		if ( _mode == RGB_LUX || _mode == RGB_ALL ){
			if ( ! _lx_red_parsed){
				strncpy(lx_red,pch,RGB_DATA_LEN);
				_lx_red = atoi(lx_red);
				_lx_red_parsed = true;
			}
			else if ( ! _lx_green_parsed){
				strncpy(lx_green,pch,RGB_DATA_LEN);
				_lx_green = atoi(lx_green);
				_lx_green_parsed = true;
			}
			else if ( ! _lx_blue_parsed){
				strncpy(lx_blue,pch,RGB_DATA_LEN);
				_lx_blue = atoi(lx_blue);
				_lx_blue_parsed = true;
			}
			else if ( ! _lx_total_parsed){
				strncpy(lx_total,pch,RGB_DATA_LEN);
				_lx_total = atoi(lx_total);
				_lx_total_parsed = true;
			}
			else if ( ! _lx_beyond_parsed){
				strncpy(lx_beyond,pch,RGB_DATA_LEN);
				_lx_beyond = atoi(lx_beyond);
				_lx_beyond_parsed = true;
			}
		}
		if ( pch[0] == '*' ){
			_saturated = true;
		}
		pch = strtok(NULL, ",\r");
	}
	return TRI_ON;
}


void RGB::enableContinuousReadings() {
	strncpy(_command,"C\r",RGB_COMMAND_LENGTH);
	_sendCommand(_command,false);
}

void RGB::disableContinuousReadings() {
	strncpy(_command,"E\r",RGB_COMMAND_LENGTH);
	_sendCommand(_command,false);
}
 
tristate RGB::setMode(rgb_mode mode) {
	char buf[RGB_COMMAND_LENGTH];
	sprintf(buf,"M%d\r",mode);
	_sendCommand(_command,true);
	// The ENV-RGB will respond:  "[RGB|lx|RGB+lx]\r"
	// parse _response and see if we succeeded
	return TRI_ON;
}
tristate RGB::queryStatus(){
	strncpy(_command,"I\r",RGB_COMMAND_LENGTH);
	_sendCommand(_command,true);
	// The ENV-RGB will respond:  "C,V<version>,<date>\r". C is for Color.
	// parse _response and see if we succeeded
	return TRI_ON; // we did
}

/*              PRIVATE METHODS                      */

void RGB::_sendCommand(char * command, bool has_result){
	_sendCommand(command,has_result,DEFAULT_COMMAND_DELAY);
}
void RGB::_sendCommand(char * command, bool has_result,uint16_t result_delay){
	if ( online() ) Serial_RGB->print(command);
	if ( has_result ) {
		int16_t byte_found = _delayUntilSerialData(10000);
		if ( byte_found == -1 ) Serial.println("No data found while waiting for result");
		_getResult(result_delay);
	}
}
void RGB::_getResult(uint16_t result_delay){
	// read last message from Serial_EZO and save it to _result.
	if ( result_delay ) _delayUntilSerialData(result_delay);
	if ( online() ) _result_len = Serial_RGB->readBytesUntil('\r',_result,RGB_SERIAL_RESULT_LEN);
	else _result_len = 0;
	_result[_result_len] = 0; // null terminate
	Serial.print("Got "); Serial.print(_result_len); Serial.print(" byte result:"); Serial.println(_result);
}

uint16_t RGB::flushSerial(){
	if ( offline() ) return 0;
	uint16_t flushed = 0;
	char byte_read;
	Serial.print("Flushing RGB:");
	while (Serial_RGB->available()) {
		byte_read = Serial_RGB->read();
		flushed++;
		if ( byte_read == '\r' ) Serial.println();
		else Serial.print(byte_read);
	}
	Serial.println(':');
	return flushed;
}

int16_t RGB::_delayUntilSerialData(uint32_t delay_millis){
	if ( offline() ) return -1;
	uint32_t _request_start = millis();
	int16_t peek_byte;
	while ( (millis() - _request_start) <= delay_millis)
	{
		peek_byte = Serial_RGB->peek();
		if ( peek_byte != -1 ) return peek_byte;
	}
	return -1;
}

