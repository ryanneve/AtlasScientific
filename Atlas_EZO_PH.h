/*============================================================================
Atlas Scientific EZO PH sensor library code is placed under the GNU license
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
EZO PH	v2.0
============================================================================*/
#ifndef Atlas_EZO_PH_h
#define Atlas_EZO_PH_h


#include "Arduino.h"
#include <Atlas_EZO.h>


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
	float			getPH() const { return _ph;}
	char	ph[10];
private:
	float	_ph;
};


#endif