// ORP has not been tested

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

Based on Atlas Scientific EZO datasheets:
ORP	v???
============================================================================*/

#define ATLAS_EZO_DEBUG


//#include <HardwareSerial.h>
#include <Atlas_EZO_ORP.h>

/*              ORP PUBLIC METHODS                      */
void EZO_ORP::initialize() {
	_initialize();
	if (debug()) Serial.println(F("ORP Initialization Done"));
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
