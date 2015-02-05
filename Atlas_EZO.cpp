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
#include <HardwareSerial.h>
#include <Atlas_EZO.h>


/*              COMMON PUBLIC METHODS                      */

ezo_response EZO::enableContinuousReadings(){
	if ( _i2c_address != 0 ) return EZO_RESPONSE_NA; // i2c mode has no continuous mode
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
	return response;	
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
		if ( !_strCmp(pch,"?RESPONSE")) {
			pch = strtok(NULL, ",\r");
			if ( pch[0] == '0')			_response_mode = TRI_OFF;
			else if ( pch[0] == '1')	_response_mode = TRI_ON;
			else						_response_mode = TRI_UNKNOWN;
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
	Serial_AS->begin(_baud_rate); // This might better be done elsewhere....
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
		if ( debug() ) { Serial.print("Voltage is:"); Serial.println(_voltage);}
	}
	return response;
}

ezo_response EZO::reset(){
	strncpy(_command,"X\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,false, true);
	// User should REALLY call child.initiaize() after this.
	return response;
}

ezo_response EZO::setTempComp(float temp_C){
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
	if ( debug() ) {Serial.print("Temperature Compensation set to:"); Serial.println(_temp_comp);}
	return response;
}
 
/*              COMMON PRIVATE METHODS                      */

void EZO::_initialize() {
	char buf[40];
	// Get setup values
	delay(2000);
	flushSerial();
	Serial.println("Serial flushed, ");
	enableResponse();
	if ( disableContinuousReadings() == EZO_RESPONSE_OK ) setConnected();
	Serial.println("Continuous readings disabled.");
	queryResponse();
	if ( queryContinuousReadings() == EZO_RESPONSE_OK ) setConnected();
	sprintf(buf,"Continuous result: %s.",getResult());
	Serial.println(buf);
	if ( queryStatus() == EZO_RESPONSE_OK ) setConnected();
	if ( queryInfo() == EZO_RESPONSE_OK ) setConnected();
}


ezo_response EZO::_sendCommand(char * command, bool has_result, bool has_response){
	return _sendCommand(command, has_result, 0, has_response); // no extra delay
}

ezo_response EZO::_sendCommand(char * command, bool has_result, uint16_t result_delay, bool has_response) {
	if ( offline() ) return EZO_RESPONSE_OL;
	char byte_to_send = command[0];
	if ( _i2c_address == 0 ) {
		//uint8_t command_len = sizeof(&command);
		if ( debug() ) Serial.print("Sending command:");
		uint8_t i = 0;
		while (byte_to_send != 0 && i < ATLAS_COMMAND_LENGTH) {
			Serial_AS->write((uint8_t)byte_to_send);
			if ( debug() ) {
				if ( byte_to_send == '\r' ) Serial.println("<CR>");
				else Serial.write((char)byte_to_send);
			}
			i++;
			byte_to_send = command[i];
		}
		if ( has_result ) {
			int16_t byte_found = _delayUntilSerialData(10000);
			if ( byte_found == -1 && debug() ) Serial.println("No data found while waiting for result");
			_getResult(result_delay);
		}
		if ( has_response ) {
			delay(300);
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
		if ( has_response ) _last_response = EZO_RESPONSE_NA; // i2c doesn't have response codes
	}
	// response comes after data. For Serial communications is is enabled, for i2c it is a separate request.
	return _last_response;
	
}

ezo_response EZO::_getResponse(){ // Serial only
	if ( offline() ) _last_response = EZO_RESPONSE_OL;
	else {
		// If _resonse_mode is ON, check for command response.
		// Response should be a two letter code preceded by '*'
		if ( Serial_AS->peek() == -1 ) _delayUntilSerialData(1000); // no data yet
		_response_len = Serial_AS->readBytesUntil('\r',_response,EZO_RESPONSE_LENGTH);
		if ( debug() ) {Serial.print("Got response:"); Serial.print(_response);}
		// format: "*<ezo_response>\r"
		if (_response_mode == TRI_OFF)			_last_response = EZO_RESPONSE_NA;
		else if ( !memcmp(_response,"*OK",3))	_last_response = EZO_RESPONSE_OK;
		else if ( !memcmp(_response,"*ER",3))	_last_response = EZO_RESPONSE_ER;
		else if ( !memcmp(_response,"*OV",3))	_last_response = EZO_RESPONSE_OV;
		else if ( !memcmp(_response,"*UV",3))	_last_response = EZO_RESPONSE_UV;
		else if ( !memcmp(_response,"*RS",3))	_last_response = EZO_RESPONSE_RS;
		else if ( !memcmp(_response,"*RE",3))	_last_response = EZO_RESPONSE_RE;
		else if ( !memcmp(_response,"*SL",3))	_last_response = EZO_RESPONSE_SL;
		else if ( !memcmp(_response,"*WA",3))	_last_response = EZO_RESPONSE_WA;
		else									_last_response = EZO_RESPONSE_UK;
	}
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


/*              DO PUBLIC METHODS                      */

void EZO_DO::initialize() {
	_initialize();
	if ( disableContinuousReadings() == EZO_RESPONSE_OK ) setConnected();
	if ( queryOutput()		== EZO_RESPONSE_OK ) setConnected();
	printOutputs();
	if ( queryTempComp()	== EZO_RESPONSE_OK ) setConnected();
	if ( querySalComp()		== EZO_RESPONSE_OK ) setConnected();
	if ( queryPresComp()	== EZO_RESPONSE_OK ) setConnected();
	enableOutput(DO_OUT_PERCENT_SAT);
	enableOutput(DO_OUT_DO_MGL);
	Serial.println("DO Initialization Done");
	
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
	Serial.print("DO outputs:");
	if ( _sat_output == TRI_ON ) Serial.print("SAT % ");
	else if ( _sat_output == TRI_UNKNOWN )  Serial.print("?SAT % ");
	if ( _dox_output == TRI_ON ) Serial.print("DOX MGL ");
	else if ( _dox_output == TRI_UNKNOWN )  Serial.print("?DOX MGL ");
	Serial.println();
}
tristate EZO_DO::getOutput(do_output output) {
	switch (output) {
		case DO_OUT_PERCENT_SAT: return _sat_output;
		case DO_OUT_DO_MGL: return _dox_output;
	}
	return TRI_UNKNOWN;
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
		if ( _sat_output && !sat_parsed) {
			if ( debug() ) { Serial.print("Raw Sat.is value "); Serial.println(pch);}
			_sat = atof(pch);// convert string to float
			sat_parsed = true;
			if ( _sat < 100.0 ) width = 4;
			else width = 5;
			precision = 1;
			dtostrf(_sat,width,precision,sat); // saturation in %
			if ( debug() ) { Serial.print("Saturation is "); Serial.println(sat);}
		}
		else if ( _dox_output && ! dox_parsed){
			if ( debug() )  {Serial.print("Raw DO value: "); Serial.println(pch);}
			_dox = atof(pch); // convert string to float
			dox_parsed = true;
			width = 8;	precision = 2;
			dtostrf(_dox,width,precision,dox); // Dissolved oxygen in mg/l
			if ( debug() )  {Serial.print("Dissolved Oxygen is "); Serial.println(dox);}
		}
		pch = strtok(NULL, ",\r");
	}
	return response;
}

ezo_response EZO_DO::setSalComp(uint32_t sal_us) {
	_sal_us_comp = sal_us;
	_sal_ppt_comp = 0.00;
	_command_len = sprintf(_command,"S,%lu\r",sal_us);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_DO::setSalPPTComp(float sal_ppt) {
	_sal_us_comp = 0;
	_sal_ppt_comp = sal_ppt;
	_command_len = sprintf(_command,"S,%4.1f,PPT\r",(double)sal_ppt);
	return _sendCommand(_command, false,true);
}
ezo_response EZO_DO::querySalComp(){
	strncpy(_command,"S,?\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command, true,true);
	// _result should be in the format "?S,<sal_us>,<uS|ppt>\r" // wrong in documentation
	if ( debug() )  Serial.print("Salinity Compensation set to:");
	if ( _result[0] == '?' && _result[1] == 'S' && _result[2] == ',' ) {
		char * pch;
		char temp_sal_comp[10];
		pch = strtok(_result+ 3,",\r"); // value
		strncpy(temp_sal_comp,pch,10);
		pch = strtok(NULL, ",\r"); // "us" or "ppt"
		if ( !_strCmp(pch,"uS")){
			_sal_us_comp = atoi(temp_sal_comp);
			_sal_ppt_comp = 0;
			if ( debug() ) { Serial.print(_sal_us_comp);	Serial.println(" uS");}
		}
		else if ( !_strCmp(pch,"ppt")) {
			_sal_us_comp = 0;
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
	if ( debug() ) { Serial.print("Pressure Compensation set to:"); Serial.println(_temp_comp);}
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
	_initialize();
	if ( queryCalibration()	== EZO_RESPONSE_OK ) setConnected();
	if ( queryK()			== EZO_RESPONSE_OK ) setConnected();
	if ( queryOutput()		== EZO_RESPONSE_OK ) setConnected();
	printOutputs();
	if ( queryTempComp()	== EZO_RESPONSE_OK ) setConnected();
	enableOutput(EZO_EC_OUT_EC);
	enableOutput(EZO_EC_OUT_TDS);
	enableOutput(EZO_EC_OUT_S);
	enableOutput(EZO_EC_OUT_SG);
	Serial.println("EC Initialization Done");
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
	if ( ec_standard ) {
		response = _sendCommand(_command,false,true);
	}
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
		 if ( debug() ) { Serial.print("EC K value is:"); Serial.println(_k);}
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
	if (_result[0] == '?' && _result[1] == 'O' && _result[2] == ',') {
		_ezo_ec_output  = TRI_OFF;
		_tds_output = TRI_OFF;
		_s_output   = TRI_OFF;
		_sg_output  = TRI_OFF;
		char * pch;
		pch = strtok(_result+ 3,",\r");
		while ( pch != NULL) {
			if ( !_strCmp(pch,"EC"))  _ezo_ec_output  = TRI_ON;
			if ( !_strCmp(pch,"TDS")) _tds_output = TRI_ON;
			if ( !_strCmp(pch,"S"))   _s_output   = TRI_ON;
			if ( !_strCmp(pch,"SG"))  _sg_output  = TRI_ON;
			pch = strtok(NULL, ",\r");
		}
	}
	return response;
}
void  EZO_EC::printOutputs(){
	// No need to check _debug here
	Serial.print("EC outputs:");
	if ( _ezo_ec_output == TRI_ON ) Serial.print("EC ");
	else if ( _ezo_ec_output == TRI_UNKNOWN )  Serial.print("?EC ");
	if ( _tds_output == TRI_ON ) Serial.print("TDS ");
	else if ( _tds_output == TRI_UNKNOWN )  Serial.print("?TDS ");
	if ( _s_output == TRI_ON )   Serial.print("S ");
	else if ( _s_output == TRI_UNKNOWN )  Serial.print("?S ");
	if ( _sg_output == TRI_ON )  Serial.print("SG ");
	else if ( _sg_output == TRI_UNKNOWN )  Serial.print("?SG ");
	Serial.println();
 }
tristate EZO_EC::getOutput(ezo_ec_output output) {
	switch (output) {
		case EZO_EC_OUT_EC: return _ezo_ec_output;
		case EZO_EC_OUT_TDS: return _tds_output;
		case EZO_EC_OUT_S: return _s_output;
		case EZO_EC_OUT_SG: return _sg_output;
	}
	return TRI_UNKNOWN;
}
ezo_response EZO_EC::querySingleReading() {
	int8_t width;
	uint8_t precision;
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	bool ec_parsed = false;
	bool tds_parsed = false;
	bool sal_parsed = false;
	bool sg_parsed = false;
	char * pch;
	pch = strtok(_result+ 3,",\r");
	while ( pch != NULL) {
		if ( _ezo_ec_output && !ec_parsed) {
			_ec = atof(pch);
			ec_parsed = true;
			if ( _ec <= 999.9 ) width = 5;
			else if ( _ec >= 1000 && _ec <= 9999 ) width = 4;
			else if ( _ec >= 10000  && _ec <= 99990 ) width = 5;
			else width = 6; // 100,000+
			if ( _ec <= 99.99 ) precision = 2;
			else if ( _ec <= 999.9 ) precision = 1;
			else precision = 0; // 1000+
			dtostrf(_ec,width,precision,ec);
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
	}
	_command_len = sprintf(_command,"O,%s,%d\r",parameter,enable_output);
	return _sendCommand(_command,false,true);
}
/*              ORP PUBLIC METHODS                      */
 void EZO_ORP::initialize() {
	 _initialize();
	 Serial.println("ORP Initialization Done");
	 
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
	 Serial.println("PH Initialization Done");
}

ezo_response EZO_PH::querySingleReading() {
	strncpy(_command,"R\r",ATLAS_COMMAND_LENGTH);
	ezo_response response = _sendCommand(_command,true,2000,true); // with 2 sec timeout
	strncpy(ph,_result,10);
	_ph = atof(ph);
	return response;
}

/*              pH PRIVATE  METHODS                      */