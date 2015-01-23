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

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <HardwareSerial.h>
#include <AtlasScientific/Atlas.h>

#define DEFAULT_ATLAS_TIMEOUT 1100
#define I2C_MIN_ADDRESS 1
#define I2C_MAX_ADDRESS 127
#define EZO_SERIAL_RESULT_LEN 50 // 48 should be max
#define EZO_COMMAND_LENGTH 20
#define EZO_NAME_LENGTH 20
#define EZO_RESPONSE_LENGTH 10
// Common
// C		Enable / Disable or Query continuous readings  (pg.16) Enabled
// I		Device information  (pg.25) N/A
// I2C		Sets the I 2 C ID number  (pg.31) Not set
// L		Enable / Disable or Query the LEDs  (pg.15) LEDs Enabled
// Name		Set or Query the name of the device  (pg.24) Not set
// Response	Enable / Disable or Query response code  (pg.26) Enabled
// Serial	Set the baud rate  (pg.29)
// Sleep	Enter low power sleep mode  (pg.28) N/A
// Status	Retrieve status information  (pg.27) N/A
// T		Temperature compensation (pg.18)
// X		Factory reset  (pg.30) N/A

// DO Device Specific
// O		Enable / Disable or Query parts of the output string  (pg.20) All Enabled
// P		Pressure compensation (pg.20)
// R		Returns a single reading  (pg.17) N/A
// S		Salinity compensation (pg.19)

// EC specific
// Cal		Performs calibration  (pg.22) User must calibrate
// K		Set or Query the probe K constant  (pg.18)   K=1.0
// O		Enable / Disable or Query parts of the output string  (pg.20) All Enabled
// R		Returns a single reading  (pg.17) N/A

// pH Device Specific
// R		Returns a single reading  (pg.17) N/A


const char EZO_RESPONSE_COMMAND[] = "RESPONSE";
const char EZO_NAME_COMMAND[] = "NAME";

enum ezo_circuit_type {
	EZO_UNKNOWN_CIRCUIT,
	EZO_DO_CIRCUIT,
	EZO_EC_CIRCUIT,
	EZO_ORP_CIRCUIT,
	EZO_PH_CIRCUIT
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
	EZO_I2C_RESPONSE_ND,	// No Data = 255
	EZO_I2C_RESPONSE_PE,	// Pending = 254
	EZO_I2C_RESPONSE_F,		// Failed = 2
	EZO_I2C_RESPONSE_S,		// Success = 1
	EZO_I2C_RESPONSE_UK		// UnKnown
};

enum restart_code {
	EZO_RESTART_P,	// Power on reset
	EZO_RESTART_S,	// Software reset
	EZO_RESTART_B,	// brown our reset
	EZO_RESTART_W,	// Watchdog reset
	EZO_RESTART_U,	// unknown
	EZO_RESTART_N	// none or no response
};

class EZO {
	public:
		EZO() { // default constructor
			_continuous_mode = TRI_UNKNOWN;
			_response_mode = TRI_UNKNOWN;
			_last_response = EZO_RESPONSE_NA;
			_i2c_address = 0;
			_voltage = 0.0;
			_temp_comp = 0.0;
			_led = TRI_UNKNOWN;
			_online = true;
			_circuit_type = EZO_UNKNOWN_CIRCUIT;
			strncpy(_firmware,"0.0",6);
			strncpy(_name, "UNKNOWN",EZO_NAME_LENGTH);
		}
		void			begin();
		void			begin(HardwareSerial *serial,uint32_t baud_rate);
		ezo_response	enableContinuousReadings();
		ezo_response	disableContinuousReadings();
		ezo_response	queryContinuousReadings();
		ezo_response	setI2CAddress(uint8_t address);
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
		ezo_response	setBaudRate(uint32_t baud_rate);
		ezo_response	sleep();
		ezo_response	wake();
		ezo_response	queryStatus();
		restart_code	getStatus() {return _restart_code; }
		float			getVoltage() {return _voltage;}
		ezo_response	reset();
		ezo_response	setTempComp(float temp_C);
		ezo_response	queryTempComp();
		float			getTempComp() {return _temp_comp;}
		char *			getResult() { return _result;}
		bool			online() { return _online;}
		bool			offline() { return ! _online;}
		void			setOnline() {_online = true;}
		void			setOffline() {_online = false;}
	protected:
		ezo_response	_sendCommand(char * command, bool has_result, bool has_response);
		ezo_response	_sendCommand(char * command, bool has_result, uint16_t result_delay, bool has_response);
		uint16_t		flushSerial();
		uint8_t			_strCmp(const char *str1, const char *str2);
		HardwareSerial 	* Serial_AS;
		uint32_t		_baud_rate;
		uint8_t			_command_len;
		char			_command[EZO_COMMAND_LENGTH];
		char			_result[EZO_SERIAL_RESULT_LEN];
		uint8_t			_result_len;
		float			_temp_comp;
		bool			_online;
	private:
		bool			_device_information();
		ezo_response	_getResponse(); // Serial only
		void			_getResult(uint16_t result_delay); // reads line into _result[]
		void			_geti2cResult(); // NOT IMPLEMENTED YET
		void			_getReading();
		int16_t			_delayUntilSerialData(uint32_t delay_millis);
		tristate		_continuous_mode;
		char 			 _name[EZO_NAME_LENGTH];
		char			_firmware[6];
		tristate		_response_mode; // Do we expect responses from EZO circuit
		char			_response[EZO_RESPONSE_LENGTH]; // holds string response code "*xx\r" where xx is a two letter code.
		uint8_t			_response_len; 
		ezo_response	_last_response;
		ezo_circuit_type	_circuit_type;
		tristate		_led;
		char			_version[6];
		restart_code	_restart_code;
		uint16_t		_i2c_address;
		float			_voltage;
		uint32_t		_request_timeout;
};


/*-------------------- DO --------------------*/
const float DEFAULT_PRESSURE_KPA = 101.325;
enum do_output {
	DO_OUT_PERCENT_SAT,
	DO_OUT_DO_MGL
};

class EZO_DO: public EZO {
	public:
	EZO_DO() {
		_sat_output = TRI_UNKNOWN;
		_dox_output = TRI_UNKNOWN;
	}
	void			initialize();
	ezo_response	enableOutput(do_output output);
	ezo_response	disableOutput(do_output output);
	ezo_response	queryOutput();
	tristate		getOutput(do_output output);
	void			printOutputs();
	ezo_response	querySingleReading();
	ezo_response	setPresComp(float pressure_kpa);
	ezo_response	queryPresComp();
	float			getPressure() {return _pressure;}
	
	ezo_response	setSalComp(uint32_t sal_us);
	ezo_response	setSalPPTComp(float sal_ppt);
	ezo_response	querySalComp();
	
	// How to combine these?
	uint16_t		querySal();
	float			querySalPPT();
	float			getSat() {return _sat;}
	float			getDOx() { return _dox;}
	char			sat[10];
	char			dox[10];
	protected:
	private:
	ezo_response	_changeOutput(do_output output,int8_t enable_output);
	tristate		_sat_output;
	tristate		_dox_output;
	float			_sat;
	float			_dox;
	float			_pressure;
	uint32_t		_sal_us_comp;
	float			_sal_ppt_comp;
};

/*-------------------- EC --------------------*/


const float EZO_EC_DEFAULT_TEMP = 25.1;

enum ec_output {
	EC_OUT_EC, 
	EC_OUT_TDS,
	EC_OUT_S,
	EC_OUT_SG
};

enum ec_calibration_command {
	EC_CAL_CLEAR,
	EC_CAL_DRY,
	EC_CAL_ONE,
	EC_CAL_LOW,
	EC_CAL_HIGH,
	EC_CAL_QUERY
};

enum ec_cal_status {
	EC_CAL_UNKNOWN,
	EC_CAL_NOT_CALIBRATED,
	EC_CAL_SINGLE,
	EC_CAL_DOUBLE
};

class EZO_EC: public EZO {
	public:
		EZO_EC() {
			_k = -1.0; // Unknown
			_ec_output = TRI_UNKNOWN;
			_tds_output = TRI_UNKNOWN;
			_s_output = TRI_UNKNOWN;
			_sg_output = TRI_UNKNOWN;
			_ec = 0.0;
			_tds = 0.0;
			_sal = 0.0;
			_sg = 0.0;
		}
		void			initialize();
		ezo_response	calibrate(ec_calibration_command command);
		ezo_response	calibrate(ec_calibration_command command,uint32_t standard);
		ezo_response	queryCalibration();
		ec_cal_status	getCalStatus(){return _calibration_status;}
		ezo_response	setK(float k);
		ezo_response	queryK();
		float			getK() {return _k; }
		ezo_response	enableOutput(ec_output output);
		ezo_response	disableOutput(ec_output output);
		ezo_response	queryOutput();
		tristate		getOutput(ec_output output);
		void			printOutputs();
		ezo_response	querySingleReading();
		float			getEC()  { return _ec;}
		float			getTDS() { return _tds;}
		float			getSAL() { return _sal;}
		float			getSG()  { return _sg;}
		char			ec[10];
		char			tds[10];
		char			sal[10];
		char			sg[10];
	protected:
	private:
		ezo_response	_changeOutput(ec_output output,int8_t enable_output);
		float			_k;
		ec_cal_status	_calibration_status;
		tristate		_ec_output;
		tristate		_tds_output;
		tristate		_s_output;
		tristate		_sg_output;
		float			_ec;
		float			_tds;
		float			_sal;
		float			_sg;
};

/*-------------------- ORP --------------------*/

enum orp_cal_status {
	ORP_CAL_UNKNOWN,
	ORP_CAL_NOT_CALIBRATED = false,
	ORP_CAL_CALIBRATED = true
};

class EZO_ORP: public EZO {
	public:
		EZO_ORP() {
			_orp = 0.0;
		}
		void			initialize();
		ezo_response	clearCalibration();
		ezo_response	calibrate(uint32_t known_orp);
		ezo_response	calibrate(float known_orp);
		ezo_response	queryCalibration();
		orp_cal_status	getCalStatus(){return _calibration_status;}
		ezo_response	querySingleReading();
		char			orp[10];
	private:
		float			_orp;
		orp_cal_status	_calibration_status;
};

/*-------------------- pH --------------------*/

class EZO_PH: public EZO {
	public:
		EZO_PH() {
			_ph = 0.0;
		}
		void			initialize();
		ezo_response	querySingleReading();
		char	ph[10];
	private:
		float	_ph;
};


#endif