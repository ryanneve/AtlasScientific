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
	
	Based on Atlas Scientific datasheets:
		EZO DO	v2.0
		EZO EC	v2.4
		EZO ORP	v2.0
		EZO PH	v2.0
============================================================================*/
#ifndef Atlas_EZO_h
#define Atlas_EZO_h

#define ATLAS_EZO_DEBUG

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <HardwareSerial.h>
#include <Atlas.h>

#define DEFAULT_ATLAS_TIMEOUT 1100
#define I2C_MIN_ADDRESS 1
#define I2C_MAX_ADDRESS 127
#define EZO_NAME_LENGTH 20
#define EZO_RESPONSE_LENGTH 10


const char EZO_RESPONSE_COMMAND[] = "RESPONSE";
const char EZO_NAME_COMMAND[] = "NAME";

const float EZO_EC_DEFAULT_TEMP = 25.1; // Used by EC and DO

enum ezo_circuit_type {
	EZO_UNKNOWN_CIRCUIT,
	EZO_DO_CIRCUIT,
	EZO_EC_CIRCUIT,
	EZO_ORP_CIRCUIT,
	EZO_PH_CIRCUIT,
	EZO_RGB_CIRCUIT,
	EZO_TEMP_CIRCUIT,
	RGB_CIRCUIT		// non-EZO
};
	
enum ezo_response {
	EZO_RESPONSE_OL,	// Circuit offline
	EZO_RESPONSE_NA,	// Not in response mode
	EZO_RESPONSE_UK,	// Unknown
	EZO_RESPONSE_OK,	// Command accepted
	EZO_RESPONSE_ER,	// An unknown command has been sent 
	EZO_RESPONSE_OV,	// The circuit is being over-volted (VCC>=5.5V)
	EZO_RESPONSE_UV,	// The circuit is being under-volted (VCC<=3.1V)
	EZO_RESPONSE_RS,	// The circuit has reset 
	EZO_RESPONSE_RE,	// The circuit has completed boot up
	EZO_RESPONSE_SL,	// The circuit has been put to sleep
	EZO_RESPONSE_WA,	// The circuit has woken up from sleep
	EZO_I2C_RESPONSE_NA,	// I2c not inresponse mode
	EZO_I2C_RESPONSE_ND,	// No Data = 255
	EZO_I2C_RESPONSE_PE,	// Pending = 254
	EZO_I2C_RESPONSE_F,		// Failed = 2
	EZO_I2C_RESPONSE_S,		// Success = 1
	EZO_I2C_RESPONSE_UK		// UnKnown
};

enum ezo_restart_code {
	EZO_RESTART_P,	// Power on reset
	EZO_RESTART_S,	// Software reset
	EZO_RESTART_B,	// brown our reset
	EZO_RESTART_W,	// Watchdog reset
	EZO_RESTART_U,	// unknown
	EZO_RESTART_N	// none or no response
};

enum ezo_cal_status {
	EZO_CAL_UNKNOWN,
	EZO_CAL_CALIBRATED, // ORP only
	EZO_CAL_NOT_CALIBRATED,
	EZO_CAL_SINGLE,
	EZO_CAL_DOUBLE,
	EZO_CAL_TRIPLE // PH only
};

class EZO: public Atlas {
	public:
		EZO() { // default constructor
			_continuous_mode = TRI_UNKNOWN;
			_response_mode = TRI_UNKNOWN;
			_last_response = EZO_RESPONSE_NA;
			_i2c_address = 0;
			_voltage = 0.0;
			_temp_comp = 0.0;
			_led = TRI_UNKNOWN;
			_circuit_type = EZO_UNKNOWN_CIRCUIT;
			_calibration_status = EZO_CAL_UNKNOWN;
			strncpy(_firmware,"0.0",6);
			strncpy(_name, "UNKNOWN",EZO_NAME_LENGTH);
			// The _reset command changed. On older EZO circuits it was X and on newer ones it's "FACTORY". THe change point is:
			// pH v1.9
			// orp v1.7
			// DO v1.7
			// EC v1.8
			strncpy(_reset_command, "X",8); // default
		}
		ezo_response	enableContinuousReadings();
		ezo_response	disableContinuousReadings();
		ezo_response	queryContinuousReadings();
		ezo_response	queryCalibration();
		ezo_cal_status	getCalibration(){return _calibration_status;}
		ezo_response	clearCalibration();
		ezo_response	setI2CAddress(const uint8_t address);
		ezo_response	enableLED();
		ezo_response	disableLED();
		ezo_response	queryLED();
		tristate		getLED() {return _led;}
		ezo_response	setName(char * name);
		ezo_response	queryName();
		char *			getName() {return _name;}
		ezo_response	queryInfo();
		char *			getInfo() {return _firmware;}
		ezo_response	enableResponse();
		ezo_response	disableResponse();
		tristate		queryResponse();
		ezo_response	getLastResponse(){return _last_response;}
		void			printLastResponse();
#ifdef ATLAS_EZO_DEBUG
		void			printResponse(char * buf, const ezo_response response);
#endif
		ezo_response	setBaudRate(const uint32_t baud_rate);
		ezo_response	fixBaudRate(const uint32_t alt_baud_rate);
		ezo_response	sleep();
		ezo_response	wake();
		ezo_response	queryStatus();
		ezo_restart_code	getStatus() {return _restart_code; }
		float			getVoltage() {return _voltage;}
		ezo_response	reset(); // for most but not all EZO sensors
		ezo_response	setTempComp(const float temp_C);
		ezo_response	queryTempComp();
		float			getTempComp() {return _temp_comp;}
		char *			getResult() { return _result;}
	protected:
		ezo_response	_sendCommand(const char * command, const bool has_result, const bool has_response);
		ezo_response	_sendCommand(const char * command, const bool has_result, const uint16_t result_delay, const bool has_response);
		uint8_t			_command_len;
		float			_temp_comp;
		ezo_cal_status	_calibration_status;
		void			_initialize();
		char			_response[EZO_RESPONSE_LENGTH]; // holds string response code "*xx\r" where xx is a two letter code.
		char			_reset_command[8];
	private:
		//bool			_device_information();
		ezo_response	_getResponse(); // Serial only
		void			_geti2cResult(); // NOT IMPLEMENTED YET
		void			_getReading();
		boolean			_checkVersionResetCommand(const float firmware_f);
		tristate		_continuous_mode;
		char 			 _name[EZO_NAME_LENGTH];
		char			_firmware[6];
		tristate		_response_mode; // Do we expect responses from EZO circuit
		uint8_t			_response_len; 
		ezo_response	_last_response;
		ezo_circuit_type	_circuit_type;
		tristate		_led;
		char			_version[6];
		ezo_restart_code	_restart_code;
		uint16_t		_i2c_address;
		float			_voltage;
		uint32_t		_request_timeout;
};




#endif