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
============================================================================*/

#define ATLAS_EZO_DEBUG


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
			if ( _delayUntilSerialData(10000) != -1 ) {
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
/*              ORP PUBLIC METHODS                      */
 void EZO_ORP::initialize() {
	_initialize();
	if (debug()) Serial.println(F("ORP Initialization Done"));
}

/*
ezo_response EZO_ORP::calibrate(uint32_t known_orp) {
	_command_len = sprintf(_command,"Cal,%ld\r",known_orp);
	return _sendCommand(_command,false,true);
	
}
ezo_response EZO_ORP::calibrate(float known_orp) {
	_command_len = sprintf(_command,"Cal,%6.2f\r",(double)known_orp);
	return _sendCommand(_command,false,true);
	
}
*/
ezo_response EZO_ORP::querySingleReading() {
	 strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	 ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	 strncpy(orp,_result,10);
	 _orp = atof(orp);
	 return response;
}

/*              ORP PRIVATE  METHODS                      */

/*              PH PUBLIC METHODS                      */
void EZO_PH::initialize() {
	 _initialize();
	 if (debug()) Serial.println(F("PH Initialization Done"));
}

ezo_response EZO_PH::querySingleReading() {
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	strncpy(ph,_result,10);
	_ph = atof(ph);
	return response;
}

/*              pH PRIVATE  METHODS                      */


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
	if ( auto_led ) _command_len = sprintf(_command,"L,%d,T\r",brightness);
	else _command_len = sprintf(_command,"L,%d\r",brightness);
	if ( debug() ) { Serial.print(F("Setting LED to ")); Serial.println(brightness); }
	return _sendCommand(_command,true,true);
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