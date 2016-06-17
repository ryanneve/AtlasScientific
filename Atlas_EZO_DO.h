/*============================================================================
Atlas Scientific EZO DO sensor library code is placed under the GNU license
Copyright (c) 2016 Ryan Neve <Ryan@PlanktosInstruments.com>

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
============================================================================*/
#ifndef Atlas_EZO_DO_h
#define Atlas_EZO_DO_h


#include "Arduino.h"
#include <Atlas_EZO.h>

/*-------------------- DO --------------------*/
const float DEFAULT_PRESSURE_KPA = 101.325;
enum do_output {
	EZO_DO_OUT_UNKNOWN	= 0,
	EZO_DO_OUT_MGL		= 1,
	EZO_DO_OUT_SAT		= 2,
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
	ezo_response	setSalComp(uint32_t sal_uS);
	uint32_t		getSalComp() {return _sal_uS_comp;}
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
	uint32_t		_sal_uS_comp;
	float			_sal_ppt_comp;
};

#endif