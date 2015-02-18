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
	
============================================================================*/
#include "Atlas.h"
#ifdef ATLAS_EZO_DEBUG
	#define ATLAS_DEBUG 1
#elif ATLAS_RGB_DEBUG
	#define ATLAS_DEBUG 1
#endif

void Atlas::begin(HardwareSerial *serial,uint32_t baud_rate) {
	_baud_rate = baud_rate;
	Serial_AS = serial;
	begin();
}
void Atlas::begin() {
	Serial_AS->begin(_baud_rate);
	flushSerial();
}

uint16_t Atlas::flushSerial(){
	if ( offline() ) return 0;
	uint16_t flushed = 0;
	while (Serial_AS->available()) {
		Serial_AS->read();
		flushed++;
	}
	return flushed;
}

void Atlas::_setConnected() {
	if ( ! _connected ) {
		_connected = true;
#ifdef ATLAS_DEBUG
		if ( debug() ) Serial.println(F("Instrument Connected"));
#endif
	}
}

void Atlas::setOnline() {
	_online = true;
}
void Atlas::setOffline() {
	_online = false;
}

int16_t Atlas::_delayUntilSerialData(uint32_t delay_millis) const{
	if ( offline() ) return -1;
	uint32_t _request_start = millis();
	int16_t peek_byte;
	while ( (millis() - _request_start) <= delay_millis)
	{
		peek_byte = Serial_AS->peek();
		if ( peek_byte != -1 ) return peek_byte;
	}
	return -1;
}

void Atlas::_getResult(uint16_t result_delay){
	// read last message from Serial_AS and save it to _result.
	if ( result_delay ) _delayUntilSerialData(result_delay);
	if ( online() ) _result_len = Serial_AS->readBytesUntil('\r',_result,ATLAS_SERIAL_RESULT_LEN);
	else _result_len = 0;
	_result[_result_len] = 0; // null terminate
#ifdef ATLAS_DEBUG
	if ( debug() ) { Serial.print("Got "); Serial.print(_result_len); Serial.print(" byte result:"); Serial.println(_result);}
#endif
}


uint8_t Atlas::_strCmp(const char *str1, const char *str2) const {
	while (*str1 && *str1 == *str2)
	++str1, ++str2;
	return *str1;
}