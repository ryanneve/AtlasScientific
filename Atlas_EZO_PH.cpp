/*============================================================================
Atlas Scientific EZO pH sensor library code is placed under the GNU license
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

Based on Atlas Scientific EZO datasheets:
PH	v2.0
============================================================================*/

#define ATLAS_EZO_DEBUG


//#include <HardwareSerial.h>
#include <Atlas_EZO_PH.h>
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

