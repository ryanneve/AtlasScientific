// comment

#include <HardwareSerial.h>
#include <AtlasScientific/Atlas_EZO.h>


/*              COMMON PUBLIC METHODS                      */


void EZO::begin() {
	Serial_EZO->begin(_baud_rate);
	flushSerial();
}

ezo_response EZO::enableContinuousReadings(){
	if ( _i2c_address != 0 ) return EZO_RESPONSE_NA; // i2c mode has no continuous mode
	strncpy(_command,"C,1\r",EZO_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::disableContinuousReadings(){
	if ( _i2c_address != 0 ) {
		_continuous_mode = EZO_OFF;
		return EZO_RESPONSE_OK; // i2c mode has no continuous mode
	}
	strncpy(_command,"C,0\r",EZO_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::queryContinuousReadings(){
	if ( _i2c_address != 0 ) {
		_continuous_mode = EZO_OFF;
		return EZO_RESPONSE_NA; // i2c mode has no continuous mode
	}
	strncpy(_command,"C,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	_continuous_mode = EZO_UNKNOWN;
	// _result will be "?C,<0|1>\r"
	if ( _result[0] == '?' && _result[1] == 'C' && _result[2] == ',') {
		if ( _result[3] == '0')      _continuous_mode = EZO_OFF;
		else if ( _result[3] == '1') _continuous_mode = EZO_ON;
	}
	return response;
}

ezo_response EZO::setName(char * name){
	_command_len = sprintf(_command,"NAME,%s\r",name);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::queryName(){
	strncpy(_command,"NAME,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	// Parse _result
	// Format: "?NAME,<NAME>\r". If there is no name, nothing will be returned!
	// put name in _name array
	strncpy(_name,"UNKNOWN",EZO_NAME_LENGTH); // TEMPORARY UNTIL WE ACTUALLY DO THE PARSING
	return response;
}

ezo_response EZO::enableLED(){
	strncpy(_command,"L,1\r",EZO_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::disableLED(){
	strncpy(_command,"L,0\r",EZO_COMMAND_LENGTH);
	return _sendCommand(_command,false,true);
}
ezo_response EZO::queryLED(){
	strncpy(_command,"L,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,true);
	_led = EZO_UNKNOWN;
	// Parse _result
	// Format: "?L,<1|0>\r"
	if ( _result[0] == '?' && _result[1] == 'L' && _result[2] == ',') {
		if ( _result[3] == '0')      _led = EZO_OFF;
		else if ( _result[3] == '1') _led = EZO_ON;
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
		_response_mode = EZO_ON;  // Always on for i2c
	}
	else {
		// See if _response_mode is ON
		_command_len = sprintf(_command,"%s,?\r",EZO_RESPONSE_COMMAND);
		_sendCommand(_command, true,true); // Documentation is wrong
		// Parse _result. Reply should be "?RESPONSE,<1|0>\r";
		Serial.print("Response mode is:");
		if ( _result[0] == '?' && _result[10] == '1') {
			_response_mode = EZO_ON;
			Serial.println("ON");
		}
		else if ( _result[0] == '?' && _result[10] == '0') {
			Serial.println("ON");
			_response_mode = EZO_OFF;
		}
		else {
			Serial.println("UNKNOWN");
			_response_mode = EZO_UNKNOWN; // DEFAULT
		}
	}
	return _response_mode;
}

ezo_response EZO::setBaudRate(uint32_t baud_rate) {
	ezo_response response;
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
	response = _sendCommand(_command,false,true);
	//Serial_EZO->begin(_baud_rate); // This might better be done elsewhere....
	return response;
}

ezo_response EZO::sleep(){
	strncpy(_command,"SLEEP\r",EZO_COMMAND_LENGTH);
	return _sendCommand(_command, false,true);
}
ezo_response EZO::wake(){
	flushSerial();	//Need to clear "*SL"
	strncpy(_command,"\r",EZO_COMMAND_LENGTH); // any character
	return _sendCommand(_command, false,true); // EZO_RESPONSE_WA if successful
}

ezo_response EZO::queryStatus(){
	strncpy(_command,"STATUS\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true, true);
	// _result should be in the format "?STATUS,<restart_code>,<voltage>\r"
	// parse code into restart_code;
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
		_voltage = atof(_result + 10);
		Serial.print("Voltage is:"); Serial.println(_voltage);
	}
	return response;
}

ezo_response EZO::reset(){
	strncpy(_command,"X\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,false, true);
	// User should REALLY call child.initiaize() after this.
	return response;
}

ezo_response EZO::setTempComp(float temp_C){
	_command_len = sprintf(_command,"T,%4.1f\r",(double)temp_C);
	return _sendCommand(_command, false,true);
}
ezo_response EZO::queryTempComp(){
	strncpy(_command,"T,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true,true);
	// _result should be in the format "?T,<temp_C>\r"
	_temp_comp = EZO_EC_DEFAULT_TEMP;
	if ( _result[0] == '?' && _result[1] == 'T' ) {
		_temp_comp = atof(_result + 3);
	}
	Serial.print("Temperature Compensation set to:"); Serial.println(_temp_comp);
	return response;
}
 
/*              COMMON PRIVATE METHODS                      */




ezo_response EZO::_sendCommand(char * command, bool has_result, bool has_response){
	return _sendCommand(command, has_result, 0, has_response); // no extra delay
}

ezo_response EZO::_sendCommand(char * command, bool has_result, uint16_t result_delay, bool has_response) {
	char byte_to_send = command[0];
	if ( _i2c_address == 0 ) {
		//uint8_t command_len = sizeof(&command);
		Serial.print("Sending command:");
		uint8_t i = 0;
		while (byte_to_send != 0 && i < EZO_COMMAND_LENGTH) {
			Serial_EZO->write((uint8_t)byte_to_send);
			if ( byte_to_send == '\r' ) Serial.println("<CR>");
			else Serial.write((char)byte_to_send);
			i++;
			byte_to_send = command[i];
		}
		if ( has_result ) {
			int16_t byte_found = _delayUntilSerialData(10000);
			if ( byte_found == -1 ) Serial.println("No data found while waiting for result");
			//else { Serial.print("found:"); Serial.println((char)byte_found);}
			_getResult(result_delay);
		}
		if ( has_response ) {
			delay(300);
			if ( Serial_EZO->peek() == '*' || _response_mode == EZO_ON  || _response_mode == EZO_UNKNOWN ) {
				_last_response = _getResponse();
			}
			else _last_response = EZO_RESPONSE_UK;
		}
		else _last_response = EZO_RESPONSE_NA;
	}
	else { // i2c
		// Send command via i2c
		if ( has_result ) {
			_geti2cResult(); // This get Response and Reply(data);
		}
		if ( has_response ) _last_response = EZO_RESPONSE_NA; // i2c doesn't have response codes
	}
	// response comes after data. For Serial communications is is enabled, for i2c it is a separate request.
	return _last_response;
	
}

ezo_response EZO::_getResponse(){ // Serial only
	// If _resonse_mode is ON, check for command response.
	// Response should be a two letter code preceded by '*'
	char _code[] = "UK";
	if ( Serial_EZO->peek() == -1 ) _delayUntilSerialData(1000); // no data yet
	_response_len = Serial_EZO->readBytesUntil('\r',_response,EZO_RESPONSE_LENGTH);
	Serial.print("Got response:"); Serial.println(_response);
	// format: "*<ezo_response>\r"
	if ( _response[0] == '*' ) {
		_code[0] = _response[1];
		_code[1] = _response[2];
	}
	if (_response_mode != EZO_ON) _last_response = EZO_RESPONSE_NA;
	else if ( !_strCmp(_code,"OK"))        _last_response = EZO_RESPONSE_OK;
	else if ( !_strCmp(_code,"ER"))        _last_response = EZO_RESPONSE_ER;
	else if ( !_strCmp(_code,"OV"))        _last_response = EZO_RESPONSE_OV;
	else if ( !_strCmp(_code,"UV"))        _last_response = EZO_RESPONSE_UV;
	else if ( !_strCmp(_code,"RS"))        _last_response = EZO_RESPONSE_RS;
	else if ( !_strCmp(_code,"RE"))        _last_response = EZO_RESPONSE_RE;
	else if ( !_strCmp(_code,"SL"))        _last_response = EZO_RESPONSE_SL;
	else if ( !_strCmp(_code,"WA"))        _last_response = EZO_RESPONSE_WA;
	else                                   _last_response = EZO_RESPONSE_UK;
	return _last_response;
}

void EZO::_geti2cResult(){
	delay(300);  //After 300ms an I2C read command can be issued to get the response/reply
	// Send read command
	// Parse.
	// First byte is response code [255,254,2,1] should be put in _response[0]
	_response_len = 1; // Always
	switch (_response[0]) {
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

void EZO::_getResult(uint16_t result_delay){
	// read last message from Serial_EZO and save it to _result.
	if ( result_delay ) _delayUntilSerialData(result_delay);
	_result_len = Serial_EZO->readBytesUntil('\r',_result,EZO_SERIAL_RESULT_LEN);
	_result[_result_len] = 0; // null terminate
	Serial.print("Got "); Serial.print(_result_len); Serial.print(" byte result:"); Serial.println(_result);
}

uint8_t EZO::_strCmp(const char *str1, const char *str2) {
	while (*str1 && *str1 == *str2)
	++str1, ++str2;
	return *str1;
}

uint16_t EZO::flushSerial(){
	uint16_t flushed = 0;
	char byte_read;
	Serial.print("Flushing:");
	while (Serial_EZO->available()) {
		byte_read = Serial_EZO->read();
		flushed++;
		if ( byte_read == '\r' ) Serial.println();
		else Serial.print(byte_read);
	}
	Serial.println(':');
	return flushed;
}

int16_t EZO::_delayUntilSerialData(uint32_t delay_millis){
	uint32_t _request_start = millis();
	int16_t peek_byte;
	while ( (millis() - _request_start) <= delay_millis)
	{
		peek_byte = Serial_EZO->peek();
		if ( peek_byte != -1 ) return peek_byte;
	}
	return -1;
}

/*              DO PUBLIC METHODS                      */

void EZO_DO::initialize() {
	char buf[40];
	// Get setup values
	delay(2000);
	flushSerial();
	Serial.println("Serial flushed, ");
	disableContinuousReadings();
	Serial.println("Continuous readings disabled.");
	queryResponse();
	queryContinuousReadings();
	sprintf(buf,"Continuous result: %s.",getResult());
	Serial.println(buf);
	queryStatus();
	queryOutput();
	printOutputs();
	queryTempComp();
	querySalComp();
	queryPresComp();
	enableOutput(DO_OUT_PERCENT_SAT);
	enableOutput(DO_OUT_DO_MGL);
	Serial.println("Initialization Done");
	
}

ezo_response EZO_DO::enableOutput(do_output output) {
	 return _changeOutput(output,1);
}
ezo_response EZO_DO::disableOutput(do_output output) {
	 return _changeOutput(output,0);
}
ezo_response EZO_DO::queryOutput() {
	strncpy(_command,"O,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	// _response will be ?O,EC,TDS,S,SG if all are enabled
	if (_result[0] == '?' && _result[1] == 'O' && _result[2] == ',') {
		_sat_output  = EZO_OFF;
		_dox_output = EZO_OFF;
		char * pch;
		pch = strtok(_result+ 3,",");
		while ( pch != NULL) {
			if ( !_strCmp(pch,"%"))  _sat_output  = EZO_ON;
			if ( !_strCmp(pch,"DO")) _dox_output = EZO_ON;
			pch = strtok(NULL, ",");
		}
	}
	return response;
}

void  EZO_DO::printOutputs(){
	Serial.print("DO outputs:");
	if ( _sat_output == EZO_ON ) Serial.print("SAT % ");
	else if ( _sat_output == EZO_UNKNOWN )  Serial.print("?SAT % ");
	if ( _dox_output == EZO_ON ) Serial.print("DOX MGL ");
	else if ( _dox_output == EZO_UNKNOWN )  Serial.print("?DOX MGL ");
	Serial.println();
}
tristate EZO_DO::getOutput(do_output output) {
	switch (output) {
		case DO_OUT_PERCENT_SAT: return _sat_output;
		case DO_OUT_DO_MGL: return _dox_output;
	}
	return EZO_UNKNOWN;
}


ezo_response EZO_DO::querySingleReading() {
	int8_t width;
	uint8_t precision;
	strncpy(_command,"R\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	bool sat_parsed = false;
	bool dox_parsed = false;
	char * pch;
	pch = strtok(_result+ 3,",");
	while ( pch != NULL) {
		if ( _sat_output && !sat_parsed) {
			_sat = atof(pch);
			sat_parsed = true;
			if ( _sat < 100.0 ) width = 4;
			else width = 5;
			precision = 1;
			dtostrf(_sat,width,precision,sat);
		}
		else if ( _dox_output && ! dox_parsed){
			_dox = atof(pch);
			dox_parsed = true;
			width = 8;
			precision = 2;
			dtostrf(_dox,width,precision,dox);
		}
		pch = strtok(NULL, ",");
	}
	return response;
}

ezo_response EZO_DO::setSalComp(uint32_t sal_us) {
	//
	_command_len = sprintf(_command,"S,%lu\r",sal_us);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_DO::setSalPPTComp(float sal_ppt) {
	// This parameter can be omitted if the water is less than 10 meters deep.
	_command_len = sprintf(_command,"S,%4.1f,PPT\r",(double)sal_ppt);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_DO::querySalComp(){
	strncpy(_command,"S,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true,true);
	// _result should be in the format "?S,<sal_us>,<uS|ppt>\r" // wrong in documentation
	Serial.print("Salinity Compensation set to:");
	if ( _result[0] == '?' && _result[1] == 'S' && _result[2] == ',' ) {
		//NEED TO PARSE THIS using strtok
		char * pch;
		char temp_sal_comp[10];
		pch = strtok(_result+ 3,","); // value
		strncpy(temp_sal_comp,pch,10);
		pch = strtok(NULL, ","); // "us" or "ppt"
		if ( !_strCmp(pch,"uS")){
			_sal_us_comp = atoi(temp_sal_comp);
			_sal_ppt_comp = 0;
			Serial.print(_sal_us_comp);
			Serial.println(" uS");
		}
		else if ( !_strCmp(pch,"ppt")) {
			_sal_us_comp = 0;
			_sal_ppt_comp = atof(temp_sal_comp);
			Serial.print(_sal_ppt_comp);
			Serial.println(" ppt");
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
	strncpy(_command,"P,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true,true);
	// _result should be in the format "?T,<temp_C>\r"
	_temp_comp = EZO_EC_DEFAULT_TEMP;
	if ( _result[0] == '?' && _result[1] == 'P' ) {
		_temp_comp = atof(_result + 3);
	}
	Serial.print("Pressure Compensation set to:"); Serial.println(_temp_comp);
	return response;
}


/*              DO PRIVATE METHODS                      */
ezo_response EZO_DO::_changeOutput(do_output output,int8_t enable_output) {
	 // format is "O,[parameter],[0|1]\r"
	 uint8_t PARAMETER_LEN = 10;
	 char parameter[PARAMETER_LEN];
	 switch (output) {
		 case DO_OUT_PERCENT_SAT:
		 strncpy(parameter,"%",PARAMETER_LEN); break;
		 case DO_OUT_DO_MGL:
		 strncpy(parameter,"DO",PARAMETER_LEN); break;
	 }
	 _command_len = sprintf(_command,"O,%s,%d\r",parameter,enable_output);
	 return _sendCommand(_command,false,true);
}


/*              EC PUBLIC METHODS                      */

 void EZO_EC::initialize() {
	char buf[40];
	// Get setup values
	delay(2000);
	flushSerial();
	Serial.println("Serial flushed, ");
	disableContinuousReadings();
	Serial.println("Continuous readings disabled.");
	queryResponse();
	queryContinuousReadings();
	sprintf(buf,"Continuous result: %s.",getResult());
	Serial.println(buf);
	queryStatus();
	queryCalibration();
	queryK();
	queryOutput();
	printOutputs();
	queryTempComp();
	enableOutput(EC_OUT_EC);
	enableOutput(EC_OUT_TDS);
	enableOutput(EC_OUT_S);
	enableOutput(EC_OUT_SG);
	Serial.println("Initialization Done");
 }
 ezo_response EZO_EC::calibrate(ec_calibration_command command) {
	// NOT YET IMPLEMENTED
	return EZO_RESPONSE_UK;
 }
 ezo_response EZO_EC::calibrate(ec_calibration_command command,uint32_t standard) {
	 // NOT YET IMPLEMENTED
	 return EZO_RESPONSE_UK;
 }
 ezo_response EZO_EC::queryCalibration() {
	 strncpy(_command,"Cal,?\r",EZO_COMMAND_LENGTH);
	 ezo_response response = _sendCommand(_command,true,true);
	 // _result will be "?Cal,<0|1|2>\r"
	 _calibration_status = EC_CAL_UNKNOWN;
	 if ( _result[0] == '?' && _result[1] == 'C' && _result[2] == 'a') {
		 if ( _result[5] == '0') _calibration_status = EC_CAL_NOT_CALIBRATED;
		 else if ( _result[5] == '1') _calibration_status = EC_CAL_SINGLE;
		 else if ( _result[5] == '2') _calibration_status = EC_CAL_DOUBLE;
	 }
	 return response;
 }
 
 ezo_response EZO_EC::setK(float k) {
	 _command_len = sprintf(_command,"K,%4.1f\r",(double)k);
	 return _sendCommand(_command, false,true);
 }
 ezo_response EZO_EC::queryK() {
	 strncpy(_command,"K,?\r",EZO_COMMAND_LENGTH);
	 ezo_response response = _sendCommand(_command,true,true);
	 // _result will be "?K,<floating point K number>\r"
	 if ( _result[0] == '?' && _result[1] == 'K') {
		 // parse k
		 _k = atof(_result + 3);
		 Serial.print("EC K value is:"); Serial.println(_k);
	 }
	 return response;
}
ezo_response EZO_EC::enableOutput(ec_output output) {
	return _changeOutput(output,1);
}
ezo_response EZO_EC::disableOutput(ec_output output) {
	return _changeOutput(output,0);
}
ezo_response EZO_EC::queryOutput() {
	strncpy(_command,"O,?\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	// _response will be ?O,EC,TDS,S,SG if all are enabled
	if (_result[0] == '?' && _result[1] == 'O' && _result[2] == ',') {
		_ec_output  = EZO_OFF;
		_tds_output = EZO_OFF;
		_s_output   = EZO_OFF;
		_sg_output  = EZO_OFF;
		char * pch;
		pch = strtok(_result+ 3,",");
		while ( pch != NULL) {
			if ( !_strCmp(pch,"EC"))  _ec_output  = EZO_ON;
			if ( !_strCmp(pch,"TDS")) _tds_output = EZO_ON;
			if ( !_strCmp(pch,"S"))   _s_output   = EZO_ON;
			if ( !_strCmp(pch,"SG"))  _sg_output  = EZO_ON;
			pch = strtok(NULL, ",");
		}
	}
	return response;
}
void  EZO_EC::printOutputs(){
	Serial.print("EC outputs:");
	if ( _ec_output == EZO_ON ) Serial.print("EC ");
	else if ( _ec_output == EZO_UNKNOWN )  Serial.print("?EC ");
	if ( _tds_output == EZO_ON ) Serial.print("TDS ");
	else if ( _tds_output == EZO_UNKNOWN )  Serial.print("?TDS ");
	if ( _s_output == EZO_ON )   Serial.print("S ");
	else if ( _s_output == EZO_UNKNOWN )  Serial.print("?S ");
	if ( _sg_output == EZO_ON )  Serial.print("SG ");
	else if ( _sg_output == EZO_UNKNOWN )  Serial.print("?SG ");
	Serial.println();
 }
tristate EZO_EC::getOutput(ec_output output) {
	switch (output) {
		case EC_OUT_EC: return _ec_output;
		case EC_OUT_TDS: return _tds_output;
		case EC_OUT_S: return _s_output;
		case EC_OUT_SG: return _sg_output;
	}
	return EZO_UNKNOWN;
}
ezo_response EZO_EC::querySingleReading() {
	int8_t width;
	uint8_t precision;
	strncpy(_command,"R\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	bool ec_parsed = false;
	bool tds_parsed = false;
	bool sal_parsed = false;
	bool sg_parsed = false;
	char * pch;
	pch = strtok(_result+ 3,",");
	while ( pch != NULL) {
		if ( _ec_output && !ec_parsed) {
			_ec = atof(pch);
			ec_parsed = true;
			if ( _ec <= 999.9 ) width = 5;
			else if ( _ec > 999.9 && _ec <=  9999 ) width = 4;
			else if ( _ec > 9999  && _ec <= 99990 ) width = 5;
			else width = 6; // 100,000+
			if ( _ec <= 99.99 ) precision = 2;
			else if ( _ec <= 999.9 ) precision = 1;
			else precision = 0; // 1000+
			dtostrf(_ec,width,precision,ec);
		}
		else if ( _tds_output && ! tds_parsed){
			_tds = atof(pch);
			tds_parsed = true;
			width = 8;
			precision = 2;
			dtostrf(_tds,width,precision,tds);
		}
		else if ( _s_output && ! sal_parsed){
			_sal = atof(pch);
			sal_parsed = true;
			width = 8;
			precision = 2;
			dtostrf(_sal,width,precision,sal);
		}
		else if ( _sg_output && ! sg_parsed){
			_sg = atof(pch);
			sg_parsed = true;
			width = 8;
			precision = 2;
			dtostrf(_sg,width,precision,sg);
		}
		pch = strtok(NULL, ",");
	}
	return response;
 }

/*              EC PRIVATE  METHODS                      */

ezo_response EZO_EC::_changeOutput(ec_output output,int8_t enable_output) {
	// format is "O,[parameter],[0|1]\r"
	uint8_t PARAMETER_LEN = 10;
	char parameter[PARAMETER_LEN];
	switch (output) {
		case EC_OUT_EC:
		strncpy(parameter,"EC",PARAMETER_LEN); break;
		case EC_OUT_TDS:
		strncpy(parameter,"TDS",PARAMETER_LEN); break;
		case EC_OUT_S:
		strncpy(parameter,"S",PARAMETER_LEN); break;
		case EC_OUT_SG:
		strncpy(parameter,"SG",PARAMETER_LEN); break;
	}
	_command_len = sprintf(_command,"O,%s,%d\r",parameter,enable_output);
	return _sendCommand(_command,false,true);
}
 /*              pH PUBLIC METHODS                      */
 void EZO_PH::initialize() {
	 char buf[40];
	 // Get setup values
	 delay(2000);
	 flushSerial();
	 Serial.println("Serial flushed, ");
	 disableContinuousReadings();
	 Serial.println("Continuous readings disabled.");
	 queryResponse();
	 queryContinuousReadings();
	 sprintf(buf,"Continuous result: %s.",getResult());
	 Serial.println(buf);
	 queryStatus();
	 Serial.println("Initialization Done");
	 
 }
 
 
 ezo_response EZO_PH::querySingleReading() {
	strncpy(_command,"R\r",EZO_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	strncpy(ph,_result,10);
	_ph = atof(ph);
	return response;
 }
 
 /*              pH PRIVATE  METHODS                      */