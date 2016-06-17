/*============================================================================
Atlas Scientific EZO ORP sensor library code is placed under the GNU license
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
EZO ORP	v??
============================================================================*/
#ifndef Atlas_EZO_ORP_h
#define Atlas_EZO_ORP_h


#include "Arduino.h"
#include <Atlas_EZO.h>
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
	float			getORP() const { return _orp;}		
	char			orp[10];
private:
	float			_orp;
};

#endif