/*============================================================================
Atlas Scientific EZO sensor library code is placed under the GNU license
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
	
	Based on Atlas Scientific EZO datasheets:
		DO	v2.0
		EC	v2.4
		ORP	v2.0
		PH	v2.0
		RGB
		RTD
============================================================================*/

#define ATLAS_EZO_DEBUG
#define SEND_COMMAND_DELAY	5000

#include <HardwareSerial.h>
#include <Atlas_EZO.h>



/*              COMMON PUBLIC METHODS                      */
#ifdef ATLAS_EZO_DEBUG
void EZO::printResponse(char * buf, const ezo_response response){
	switch ( response ) {		
		case EZO_RESPONSE_OL:		strncpy(buf,"OL",3); break; 	// Circuit offline
		case EZO_RESPONSE_NA:		strncpy(buf,"NA",3); break; 	// Not in response mod
		case EZO_RESPONSE_UK:		strncpy(buf,"UK",3); break; 	// Unknown
		case EZO_RESPONSE_OK:		strncpy(buf,"OK",3); break; 	// Command accepted
		case EZO_RESPONSE_ER:		strncpy(buf,"ER",3); break; 	// An unknown command
		case EZO_RESPONSE_OV:		strncpy(buf,"OV",3); break; 	// The circuit is bein
		case EZO_RESPONSE_UV:		strncpy(buf,"UV",3); break; 	// The circuit is bein
		case EZO_RESPONSE_RS:		strncpy(buf,"RS",3); break; 	// The circuit has res
		case EZO_RESPONSE_RE:		strncpy(buf,"RE",3); break; 	// The circuit has com
		case EZO_RESPONSE_SL:		strncpy(buf,"SL",3); break; 	// The circuit has bee
		case EZO_RESPONSE_WA:		strncpy(buf,"WA",3); break; 	// The circuit has wok
		case EZO_I2C_RESPONSE_NA:	strncpy(buf,"INA",4); break; 	// No Data = 255
		case EZO_I2C_RESPONSE_ND:	strncpy(buf,"IND",4); break; 	// No Data = 255
		case EZO_I2C_RESPONSE_PE:	strncpy(buf,"IPE",4); break; 	// Pending = 254
		case EZO_I2C_RESPONSE_F:	strncpy(buf,"IF",3); break; 		// Failed = 2
		case EZO_I2C_RESPONSE_S:	strncpy(buf,"IS",3); break; 		// Success = 1
		case EZO_I2C_RESPONSE_UK:	strncpy(buf,"IUK",4); break; 		// UnKnown
	}
}
#endif

ezo_response EZO::enableContinuousReadings(){
	if ( _i2c_address != 0 ) return EZO_I2C_RESPONSE_NA; // i2c mode has no continuous mode
	strncpy(_command,"C,1\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::disableContinuousReadings(){
	if ( _i2c_address != 0 ) {
		_continuous_mode = TRI_OFF;
		return EZO_RESPONSE_OK; // i2c mode has no continuous mode
	}
	strncpy(_command,"C,0\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::queryContinuousReadings(){
	if ( _i2c_address != 0 ) {
		_continuous_mode = TRI_OFF;
		return EZO_RESPONSE_NA; // i2c mode has no continuous mode
	}
	strncpy(_command,"C,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	_continuous_mode = TRI_UNKNOWN;
	// _result will be "?C,<0|1>\r"
	if ( _result[0] == '?' && _result[1] == 'C' && _result[2] == ',') {
		if ( _result[3] == '0')      _continuous_mode = TRI_OFF;
		else if ( _result[3] == '1') _continuous_mode = TRI_ON;
	}
	return response;
}


ezo_response EZO::queryCalibration() {
	strncpy(_command,"Cal,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// _result will be "?Cal,<n>\r"
	char * pch;
	pch = strtok(_result,",\r");
	if ( !_strCmp(pch,"Cal,")) {
		pch = strtok(NULL, ",\r"); // should be a single digit
		_calibration_status	= EZO_CAL_UNKNOWN;
		switch ( pch[0] ) {
			case '0': // All
				_calibration_status	= EZO_CAL_NOT_CALIBRATED;
				break;
			case '1': // All, but ORP is different
				if ( _circuit_type == EZO_ORP_CIRCUIT ) _calibration_status	= EZO_CAL_CALIBRATED;
				else _calibration_status	= EZO_CAL_SINGLE;
				break;
			case '2': // DO,EC,PH
				_calibration_status	= EZO_CAL_DOUBLE;
				break;
			case '3':  // only PH
				_calibration_status	= EZO_CAL_TRIPLE;
				break;
			default:
				break;
		}
	}
	return response;
}

ezo_response EZO::clearCalibration(){
	strncpy(_command,"Cal,clear\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}

ezo_response EZO::setName(char * name){
	_command_len = sprintf(_command,"NAME,%s\r",name);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::queryName(){
	strncpy(_command,"NAME,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// Parse _result
	// Format: "?NAME,<NAME>\r". If there is no name, nothing will be returned!
	char * pch;
	pch = strtok(_result,",\r"); // should be "?NAME"
	if ( !_strCmp(pch,"?NAME")) {
		pch = strtok(NULL, ",\r");
		strncpy(_name,pch,sizeof(_name));
	}
	else strncpy(_name,"UNKNOWN",EZO_NAME_LENGTH); // TEMPORARY UNTIL WE ACTUALLY DO THE PARSING
	return response;
}

ezo_response EZO::queryInfo(){
	strncpy(_command,"I\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// reply is in the format "?I,<device>,<firmware>\r"
	char * pch;
	pch = strtok(_result+ 3,",\r");
	if      ( !_strCmp(pch,"DO"))  _circuit_type = EZO_DO_CIRCUIT;
	else if ( !_strCmp(pch,"EC"))  _circuit_type = EZO_EC_CIRCUIT;
	else if ( !_strCmp(pch,"ORP")) _circuit_type = EZO_ORP_CIRCUIT;
	else if ( !_strCmp(pch,"PH"))  _circuit_type = EZO_PH_CIRCUIT;
	pch = strtok(NULL, ",\r");
	strncpy(_firmware,pch,6);
	//float firmware_f = atof(pch);
	if ( _checkVersionResetCommand(atof(_firmware)) ) strncpy(_reset_command, "Factory",8); 
	else strncpy(_reset_command, "X",8);
	return response;	
}
boolean EZO::_checkVersionResetCommand(const float firmware_f){
	// The reset command varies by device and firmware version.
	if ( _circuit_type == EZO_RGB_CIRCUIT ) return true; // all EZO_RGB use new command.
	if ( _circuit_type == EZO_DO_CIRCUIT	&& firmware_f >= 1.65 ) return true;
	if ( _circuit_type == EZO_EC_CIRCUIT	&& firmware_f >= 1.75 ) return true;
	if ( _circuit_type == EZO_ORP_CIRCUIT	&& firmware_f >= 1.65 ) return true;
	if ( _circuit_type == EZO_PH_CIRCUIT	&& firmware_f >= 1.85 ) return true;
	return false;
}
ezo_response EZO::enableLED(){
	strncpy(_command,"L,1\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::disableLED(){
	strncpy(_command,"L,0\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::queryLED(){
	strncpy(_command,"L,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	_led = TRI_UNKNOWN;
	// Parse _result
	// Format: "?L,<1|0>\r"
	char * pch;
	pch = strtok(_result,",\r");
	if ( !_strCmp(pch,"?L")) {
		pch = strtok(NULL, ",\r");
		if ( pch[0] == '0')			_led = TRI_OFF;
		else if ( pch[0] == '1')	_led = TRI_ON;
		else						_led = TRI_UNKNOWN;
	}
	return response;
}

ezo_response EZO::setI2CAddress(uint8_t address){
	ezo_response response = EZO_RESPONSE_ER;
	if ( address >= I2C_MIN_ADDRESS && address <= I2C_MAX_ADDRESS ){
		_command_len = sprintf(_command,"I2C,%d\r",address);
		response = _sendCommand(_command, false,true);
		if ( response != EZO_RESPONSE_ER ) _i2c_address = address;
	}
	else response = EZO_RESPONSE_ER;
	return response;
}

ezo_response EZO::enableResponse(){
	if ( _i2c_address != 0 ) return EZO_I2C_RESPONSE_S; // Not Applicable
	_command_len = sprintf(_command,"%s,1\r",EZO_RESPONSE_COMMAND);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::disableResponse(){
	if ( _i2c_address != 0 ) return EZO_I2C_RESPONSE_F; // Not Applicable
	_command_len = sprintf(_command,"%s,0\r",EZO_RESPONSE_COMMAND);
	return _sendCommand(_command,false,false);
}
tristate EZO::queryResponse() {
	if ( _i2c_address != 0 ) {
		_response_mode = TRI_ON;  // Always on for i2c
	}
	else {
		// See if _response_mode is ON
		_command_len = sprintf(_command,"%s,?\r",EZO_RESPONSE_COMMAND);
		_sendCommand(_command, true,true); // Documentation is wrong
		// Parse _result. Reply should be "?RESPONSE,<1|0>\r";
		char * pch;
		pch = strtok(_result,",\r");
		//Serial.print("Checking("); Serial.print(pch); Serial.println(")");
		if ( !_strCmp(pch,"?RESPONSE")) {
			pch = strtok(NULL, ",\r");
			if ( pch[0] == '0') 		_response_mode = TRI_OFF;
			else if ( pch[0] == '1')	_response_mode = TRI_ON;
			else						_response_mode = TRI_UNKNOWN;
		}
		if (debug()) {
			Serial.print(F("Response mode from: ")); Serial.print(pch);
			if ( _response_mode == TRI_OFF ) Serial.println(F(" off"));
			else if ( _response_mode == TRI_ON ) Serial.println(F(" on"));
			else if ( _response_mode == TRI_UNKNOWN ) Serial.println(F(" unknown"));
		}
	}
	return _response_mode;
}

ezo_response EZO::setBaudRate(const uint32_t baud_rate) {
	// Command is "SERIAL,<baud_rate>\r"
	switch (baud_rate){ // CHeck if it's a valid value
		case 300:
		case 1200:
		case 2400:
		case 9600:
		case 19200:
		case 38400:
		case 57600:
		case 115200:
			_baud_rate = baud_rate;
			break;
		default:
			return EZO_RESPONSE_ER;
	}
	// send command to circuit
	_command_len = sprintf(_command,"SERIAL,%lu\r",_baud_rate);
	ezo_response response = _sendCommand(_command,false,true);
	Serial_AS->begin(_baud_rate); // This might better be done elsewhere....
	_delayUntilSerialData(500);	flushSerial(); // We might get a *RS and *RE after this which we want to ignore
	return response;
}

ezo_response EZO::fixBaudRate(const uint32_t desired_baud_rate){
	// tries to change baud rate  to desired_baud_rate
	uint32_t baud_rates[] = {1200,38400,9600,19200,57600,2400,300,115200}; // 8 choices in order of likelyhood.
	uint8_t i = 0;
	ezo_response response;
	for ( i = 0 ; i < 7 ; i++) {
		//Serial.print("Trying baud rate:"); Serial.println(baud_rates[i]);
		Serial_AS->begin(baud_rates[i]); // Sets local baud rate
		Serial_AS->write('\r');
		//_delayUntilSerialData(500);	flushSerial();
		disableContinuousReadings(); 
		_delayUntilSerialData(500);	flushSerial();
		response = setBaudRate(desired_baud_rate);
		if ( response == EZO_RESPONSE_OK ) break;
	}
	//if ( response == EZO_RESPONSE_OK) { Serial.print("Success at "); Serial.println(_baud_rate);}
	//else Serial.println("Failed to fix baud rate");
	//Serial_AS->begin(alt_baud_rate);
	//Serial_AS->write('\r');
	//flushSerial();
	//return setBaudRate(_baud_rate);
	delay(1000 >> CLKPR);	flushSerial();
	return response;
}

ezo_response EZO::sleep(){
	strncpy(_command,"SLEEP\r",ATLAS_COMMAND_LENGTH);
	return _sendCommand(_command, false,true);
}
ezo_response EZO::wake(){
	flushSerial();	//Need to clear "*SL"
	strncpy(_command,"\r",ATLAS_COMMAND_LENGTH); // any character
	return _sendCommand(_command, false,true); // EZO_RESPONSE_WA if successful
}

ezo_response EZO::queryStatus(){
	strncpy(_command,"STATUS\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true, true);
	// _result should be in the format "?STATUS,<ezo_restart_code>,<voltage>\r"
	// parse code into ezo_restart_code;
	Serial.print(F("Parsing(")); Serial.print(_result); Serial.println(")");
	char code = 'U';
	if ( _result[0] == '?' ) {
		code = _result[8];
		switch ( code ){
			case 'P': _restart_code = EZO_RESTART_P; break;	// Power on reset
			case 'S': _restart_code = EZO_RESTART_S; break;	// Software reset
			case 'B': _restart_code = EZO_RESTART_B; break;	// brown our reset
			case 'W': _restart_code = EZO_RESTART_W; break;	// Watchdog reset
			case 'U': _restart_code = EZO_RESTART_U; break;	// unknown
			default:  _restart_code = EZO_RESTART_N; break;	// none or no response
		}
		// parse voltage
		Serial.print(F("Parsing(")); Serial.print(_result+10); Serial.println(")");
		_voltage = atof(_result + 10);
		if ( debug() ) { Serial.print(F("Voltage is:")); Serial.println(_voltage);}
	}
	return response;
}

ezo_response EZO::reset(){
	_command_len = sprintf(_command,"%s\r",_reset_command); // depends on device now.
	//strncpy(_command,"X\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,false, true);
	// User should REALLY call child.initiaize() after this.
	return response;
}

ezo_response EZO::setTempComp(const float temp_C){
	char buf[10];
	dtostrf(temp_C,4,1,buf);
	_temp_comp = temp_C; // store value locally
	_command_len = sprintf(_command,"T,%s\r",buf);
	return _sendCommand(_command, false,true);
}
ezo_response EZO::queryTempComp(){
	strncpy(_command,"T,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true,true);
	// _result should be in the format "?T,<temp_C>\r"
	_temp_comp = EZO_EC_DEFAULT_TEMP;
	if ( _result[0] == '?' && _result[1] == 'T' ) {
		_temp_comp = atof(_result + 3);
	}
	if ( debug() ) {Serial.print(F("Temperature Compensation set to:")); Serial.println(_temp_comp);}
	return response;
}
 
/*              COMMON PRIVATE METHODS                      */

void EZO::_initialize() {
	// Get setup values
	delay(2000 >> CLKPR);
	if (debug())	Serial.println(F("Flushing Serial, "));
	flushSerial();
	if (debug()) Serial.println(F("Querying Response, "));
	queryResponse();
	if (!connected()) {
		if (debug()) Serial.println(F("Communications falure, aborting"));
		return;
	}
	if ( _response_mode != TRI_ON) {
		if (debug()) Serial.println(F("Enabling RESPONSE, "));
		enableResponse();
	}
	if (debug()) Serial.println(F("Disabling continuous readings, "));
	disableContinuousReadings();
	if (debug()) Serial.println(F("Querying continuous readings, "));
	queryContinuousReadings();
	if (debug()) {
		char buf[40];
		sprintf(buf,"Continuous result: %s.",getResult());
		Serial.println(buf);
		Serial.println(F("Querying status, "));
	}
	queryStatus();
	if (debug()) Serial.println(F("Querying info "));
	queryInfo();
}


ezo_response EZO::_sendCommand(const char * command, const bool has_result, const bool has_response){
	return _sendCommand(command, has_result, 0, has_response); // no extra delay
}

ezo_response EZO::_sendCommand(const char * command, const bool has_result, const uint16_t result_delay, const bool has_response) {
	if ( offline() ) return EZO_RESPONSE_OL;
	char byte_to_send = command[0];
	if ( _i2c_address == 0 ) {
		//uint8_t command_len = sizeof(&command);
		if ( debug() ) Serial.print(F("Sending command:"));
		int8_t i = 0;
		while (byte_to_send != 0 && i < ATLAS_COMMAND_LENGTH) {
			Serial_AS->write((uint8_t)byte_to_send);
			if ( debug() ) {
				if ( byte_to_send == '\r' ) Serial.println(F("<CR>"));
				else Serial.write((char)byte_to_send);
			}
			i++;
			byte_to_send = command[i];
		}
		if ( has_result ) {
			if ( _delayUntilSerialData(SEND_COMMAND_DELAY) != -1 ) {
				_getResult(result_delay);
			}
			else if ( debug() ) Serial.println(F("No data found while waiting for result"));
		}
		if ( has_response ) {
			delay(300 >> CLKPR);
			if ( Serial_AS->peek() == '*' || _response_mode == TRI_ON  || _response_mode == TRI_UNKNOWN ) {
				_last_response = _getResponse();
			}
			else _last_response = EZO_RESPONSE_UK;
		}
		else _last_response = EZO_RESPONSE_NA;
	}
	else { // i2c NOT DONE YET
		// Send command via i2c
		if ( has_result ) {
			_geti2cResult(); // This get Response and Reply(data);
		}
		if ( has_response ) _last_response = EZO_I2C_RESPONSE_NA; // i2c doesn't have response codes
	}
	// response comes after data. For Serial communications is is enabled, for i2c it is a separate request.
	return _last_response;
	
}

ezo_response EZO::_getResponse(){ // Serial only
	if ( offline() ) _last_response = EZO_RESPONSE_OL;
	else {
		// If _resonse_mode is ON, check for command response.
		// Response should be a two letter code preceded by '*'
		//if ( Serial_AS->peek() == -1 || ) 
		_delayUntilSerialData(1000); // Wait for first non <CR> data
		_response_len = Serial_AS->readBytesUntil('\r',_response,EZO_RESPONSE_LENGTH);
		// format: "*<ezo_response>\r"
		if (_response_mode == TRI_OFF)			_last_response = EZO_RESPONSE_NA;
		else if ( !memcmp(_response,"*OK",3))	{ _last_response = EZO_RESPONSE_OK; _setConnected(); }
		else if ( !memcmp(_response,"*ER",3))	{ _last_response = EZO_RESPONSE_ER; _setConnected(); }
		else if ( !memcmp(_response,"*OV",3))	{ _last_response = EZO_RESPONSE_OV; _setConnected(); }
		else if ( !memcmp(_response,"*UV",3))	{ _last_response = EZO_RESPONSE_UV; _setConnected(); }
		else if ( !memcmp(_response,"*RS",3))	{ _last_response = EZO_RESPONSE_RS; _setConnected(); }
		else if ( !memcmp(_response,"*RE",3))	{ _last_response = EZO_RESPONSE_RE; _setConnected(); }
		else if ( !memcmp(_response,"*SL",3))	{ _last_response = EZO_RESPONSE_SL; _setConnected(); }
		else if ( !memcmp(_response,"*WA",3))	{ _last_response = EZO_RESPONSE_WA; _setConnected(); }
		else									_last_response = EZO_RESPONSE_UK;
	}
	if ( debug() ) {
		Serial.print(F("Got response:")); Serial.print(_response); Serial.print(F("= "));
		char buf[5] ; printResponse(buf,_last_response);	Serial.print(buf);	
		if ( _response_mode == TRI_OFF ) Serial.println(F(" OFF"));
		if ( _response_mode == TRI_ON ) Serial.println(F(" ON"));
		if ( _response_mode == TRI_UNKNOWN ) Serial.println(F(" UNKNOWN"));
	}
	return _last_response;
}

void EZO::_geti2cResult(){
	delay(300 >> CLKPR);  //After 300ms an I2C read command can be issued to get the response/reply
	// Send read command
	// Parse.
	// First byte is response code [255,254,2,1] should be put in _response[0]
	_response_len = 1; // Always
	switch (_response[0] + 1) { // MAKE SURE THE +1 is CORRECT.
		case 255:
			_last_response = EZO_I2C_RESPONSE_ND; break;
		case 254:
			_last_response = EZO_I2C_RESPONSE_PE; break; // did we not wait long enough?
		case 2:
			_last_response = EZO_I2C_RESPONSE_F; break; // is it worth continuing?
		case 1:
			_last_response = EZO_I2C_RESPONSE_S; break; // Success!
		default:
			_last_response = EZO_I2C_RESPONSE_UK;
	}
	// remaining bytes until null are data and should be put in _result and _result_length
}



