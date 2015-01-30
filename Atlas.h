/*============================================================================
Atlas Scientific RGB sensor library code is placed under the GNU license
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
	
============================================================================*/


#ifndef _Atlas_h
#define _Atlas_h

#define ATLAS_SERIAL_RESULT_LEN 50
#define ATLAS_COMMAND_LENGTH 20

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <HardwareSerial.h>

enum tristate {
	TRI_UNKNOWN,
	TRI_ON = true,
	TRI_OFF = false
};

class Atlas {
	public:
		void			begin();
		void			begin(HardwareSerial *serial,uint32_t baud_rate);
		uint32_t		getBaudRate() {return _baud_rate;}
		bool			online() { return _online;}
		bool			offline() { return ! _online;}
		void			setOnline() {_online = true;}
		void			setOffline() {_online = false;}
		char			read(){ return Serial_AS->read();} // Used for console mode
		void			write(char write_char) { Serial_AS->write(write_char); }
		void			debugOn(){ _debug = true;}
		void			debugOff(){_debug = false;}
		bool			getDebug(){return _debug;}
	protected:
		HardwareSerial*	Serial_AS;
		void			_getResult(uint16_t result_delay); // reads line into _result[]
		uint16_t		flushSerial();
		int16_t			_delayUntilSerialData(uint32_t delay_millis);
		uint8_t			_strCmp(const char *str1, const char *str2);
		
		uint32_t		_baud_rate;
		bool			_online;
		char			_result[ATLAS_SERIAL_RESULT_LEN];
		uint8_t			_result_len;
		char			_command[ATLAS_COMMAND_LENGTH];
		bool			_debug;
	private:
};
#endif