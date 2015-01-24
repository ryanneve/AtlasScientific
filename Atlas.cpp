#include "Atlas.h"

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



int16_t Atlas::_delayUntilSerialData(uint32_t delay_millis){
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
	// read last message from Serial_EZO and save it to _result.
	if ( result_delay ) _delayUntilSerialData(result_delay);
	if ( online() ) _result_len = Serial_AS->readBytesUntil('\r',_result,ATLAS_SERIAL_RESULT_LEN);
	else _result_len = 0;
	_result[_result_len] = 0; // null terminate
	if ( _debug ) { Serial.print("Got "); Serial.print(_result_len); Serial.print(" byte result:"); Serial.println(_result);}
}


uint8_t Atlas::_strCmp(const char *str1, const char *str2) {
	while (*str1 && *str1 == *str2)
	++str1, ++str2;
	return *str1;
}