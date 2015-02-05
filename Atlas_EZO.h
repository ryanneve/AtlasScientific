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
#include <Atlas.h>

#define DEFAULT_ATLAS_TIMEOUT 1100
#define I2C_MIN_ADDRESS 1
#define I2C_MAX_ADDRESS 127
#define EZO_NAME_LENGTH 20
#define EZO_RESPONSE_LENGTH 10


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
		}
		ezo_response	enableContinuousReadings();
		ezo_response	disableContinuousReadings();
		ezo_response	queryContinuousReadings();
		ezo_response	queryCalibration();
		ezo_cal_status	getCalibration(){return _calibration_status;}
		ezo_response	clearCalibration();
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
		ezo_restart_code	getStatus() {return _restart_code; }
		float			getVoltage() {return _voltage;}
		ezo_response	reset();
		ezo_response	setTempComp(float temp_C);
		ezo_response	queryTempComp();
		float			getTempComp() {return _temp_comp;}
		char *			getResult() { return _result;}
	protected:
		ezo_response	_sendCommand(char * command, bool has_result, bool has_response);
		ezo_response	_sendCommand(char * command, bool has_result, uint16_t result_delay, bool has_response);
		uint8_t			_command_len;
		float			_temp_comp;
		ezo_cal_status	_calibration_status;
		void			_initialize();
	private:
		bool			_device_information();
		ezo_response	_getResponse(); // Serial only
		void			_geti2cResult(); // NOT IMPLEMENTED YET
		void			_getReading();
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
		ezo_restart_code	_restart_code;
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

enum ezo_do_calibration_command {
	EZO_DO_CAL_CLEAR,
	EZO_DO_CAL_ATM,
	EZO_DO_CAL_ZERO,
	EZO_DO_CAL_QUERY
};

class EZO_DO: public EZO {
	public:
	EZO_DO() {
		_sat_output = TRI_UNKNOWN;
		_dox_output = TRI_UNKNOWN;
	}
	void			initialize();
	ezo_response	calibrate(ezo_do_calibration_command command);
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
	uint32_t		getSalComp() {return _sal_us_comp;}
	ezo_response	setSalPPTComp(float sal_ppt);
	float			getSalPPTComp(){return _sal_ppt_comp;}
	ezo_response	querySalComp();
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

enum ezo_ec_output {
	EZO_EC_OUT_EC, 
	EZO_EC_OUT_TDS,
	EZO_EC_OUT_S,
	EZO_EC_OUT_SG
};

enum ezo_ec_calibration_command {
	EZO_EC_CAL_CLEAR,
	EZO_EC_CAL_DRY,
	EZO_EC_CAL_ONE,
	EZO_EC_CAL_LOW,
	EZO_EC_CAL_HIGH,
	EZO_EC_CAL_QUERY
};



class EZO_EC: public EZO {
	public:
		EZO_EC() {
			_k = -1.0; // Unknown
			_ezo_ec_output = TRI_UNKNOWN;
			_tds_output = TRI_UNKNOWN;
			_s_output = TRI_UNKNOWN;
			_sg_output = TRI_UNKNOWN;
			_ec = 0.0;
			_tds = 0.0;
			_sal = 0.0;
			_sg = 0.0;
		}
		void			initialize();		
		ezo_response	calibrate(ezo_ec_calibration_command command) { return calibrate(command,0);}
		ezo_response	calibrate(ezo_ec_calibration_command command,uint32_t ec_standard);
		ezo_response	setK(float k);
		ezo_response	queryK();
		float			getK() const {return _k; }
		ezo_response	enableOutput(ezo_ec_output output);
		ezo_response	disableOutput(ezo_ec_output output);
		ezo_response	queryOutput();
		tristate		getOutput(ezo_ec_output output);
		void			printOutputs();
		ezo_response	querySingleReading();
		float			getEC() const { return _ec;}
		float			getTDS() const { return _tds;}
		float			getSAL() const { return _sal;}
		float			getSG()  const { return _sg;}
			
		char			ec[10];
		char			tds[10];
		char			sal[10];
		char			sg[10];
	protected:
	private:
		ezo_response	_changeOutput(ezo_ec_output output,int8_t enable_output);
		
		float			_k;
		tristate		_ezo_ec_output;
		tristate		_tds_output;
		tristate		_s_output;
		tristate		_sg_output;
		float			_ec;
		float			_tds;
		float			_sal;
		float			_sg;
};

/*-------------------- ORP --------------------*/

enum ezo_orp_calibration_command {
	EZO_ORP_CAL_CLEAR,
	EZO_ORP_CAL_ATM,
	EZO_ORP_CAL_ZERO,
	EZO_ORP_CAL_QUERY
};

class EZO_ORP: public EZO {
	public:
		EZO_ORP() {
			_orp = 0.0;
		}
		void			initialize();
		ezo_response	calibrate(ezo_orp_calibration_command command) { return calibrate(command,(uint32_t)0);}
		ezo_response	calibrate(ezo_orp_calibration_command command,float orp_standard);
		ezo_response	calibrate(ezo_orp_calibration_command command,uint32_t orp_standard);
		ezo_response	querySingleReading();
		
		char			orp[10];
	private:
		float			_orp;
};

/*-------------------- pH --------------------*/

enum ezo_ph_calibration_command {
	EZO_PH_CAL_CLEAR,
	EZO_PH_CAL_ATM,
	EZO_PH_CAL_ZERO,
	EZO_PH_CAL_QUERY
};

class EZO_PH: public EZO {
	public:
		EZO_PH() {
			_ph = 0.0;
		}
		void			initialize();
		ezo_response	querySingleReading();
		ezo_response	calibrate(ezo_ph_calibration_command command) { return calibrate(command,0);}
		ezo_response	calibrate(ezo_ph_calibration_command command,uint32_t ph_standard);
		
		char	ph[10];
	private:
		float	_ph;
};


#endif