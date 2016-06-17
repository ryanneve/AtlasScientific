/*============================================================================
Atlas Scientific EZO EC sensor library code is placed under the GNU license
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
EZO DO	v2.4
============================================================================*/
#ifndef Atlas_EZO_EC_h
#define Atlas_EZO_EC_h


#include "Arduino.h"
#include <Atlas_EZO.h>
/*-------------------- EC --------------------*/

enum ezo_ec_output {
	EZO_EC_OUT_UNKNOWN	= 0,
	EZO_EC_OUT_EC		= 1, 
	EZO_EC_OUT_TDS		= 2,
	EZO_EC_OUT_S		= 4,
	EZO_EC_OUT_SG		= 8
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
	tristate		_ec_output;
	tristate		_tds_output;
	tristate		_s_output;
	tristate		_sg_output;
	float			_ec;	// uS
	float			_tds;	//mg/L
	float			_sal;	// PSS-78 (no units)
	float			_sg;	// Dimensionless unit
};
#endif