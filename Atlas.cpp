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

#define ATLAS_DEBUG 1
#define ATLAS_EZO_DEBUG 1
#ifdef ATLAS_EZO_DEBUG
	#define ATLAS_DEBUG 1
#endif 
#ifdef ATLAS_RGB_DEBUG
	#define ATLAS_DEBUG 1
#endif

void Atlas::begin(HardwareSerial *serial,const uint32_t baud_rate) {
	// Need to do this at least once
	Serial_AS = serial;
	begin(baud_rate);
}
void Atlas::begin(const uint32_t baud_rate) {
	// To change baud rate
	_baud_rate = baud_rate;
	begin();
}
void Atlas::begin() {
	// To re-establish communications
	Serial_AS->begin(_baud_rate << CLKPR);
	flushSerial();
}

uint16_t Atlas::flushSerial(){
	if ( offline() ) return 0;
	uint16_t flushed = 0;
	char flush_char;
	if (debug()) Serial.print(F("Flushing:"));
	while (Serial_AS->available()) {
		flush_char = Serial_AS->read();
		if (debug()) Serial.print(flush_char);
		flushed++;
	}
	if (debug()) { Serial.print(F("\r\nFlushed:")); Serial.println(flushed);}
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
		if ( peek_byte == 13 ) Serial_AS->read(); // pop off a CR.
		else if ( peek_byte != -1 ) return peek_byte;
	}
	return -1;
}

void Atlas::_getResult(const uint16_t result_delay){
	// read last message from Serial_AS and save it to _result.
	if ( debug()) {
		if ( result_delay ) Serial.print(_delayUntilSerialData(result_delay));
		else Serial.print(F("noDelay"));
	}
	if ( online() ) {
		Serial_AS->setTimeout(3000);
		_result_len = Serial_AS->readBytesUntil('\r',_result,ATLAS_SERIAL_RESULT_LEN);
		Serial_AS->setTimeout(1000); // default
	}
	else _result_len = 0;
	_result[_result_len] = 0; // null terminate
	if ( debug()) { Serial.print(F("Got ")); Serial.print(_result_len); Serial.print(F(" byte result:")); Serial.println(_result);}
}


uint8_t Atlas::_strCmp(const char *str1, const char *str2) const {
	// Compares two strings. returns mismatch or 0 if they match
	while (*str1 && *str1 == *str2)
	++str1, ++str2;
	return *str1;
}