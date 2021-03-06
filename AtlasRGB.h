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
	
	Based on Atlas Scientific datasheets:
		RGB	v1.6
============================================================================*/

#ifndef _Atlas_RGB_h
#define _Atlas_RGB_h


#include <Arduino.h>
//#include <stdint.h>
#include <HardwareSerial.h>
#include <Atlas.h>

#define BAUD_RATE_RGB_DEFAULT 38400
//#define ATLAS_COMMAND_LENGTH 10
#define DEFAULT_COMMAND_DELAY 1000
//#define ATLAS_SERIAL_RESULT_LEN 50
#define RGB_DATA_LEN 6
/*
R		Take a single color reading
C		Take continues color readings every 1200 milliseconds
E		End continues readings, enter standby/quiescent mode
I		Information: Type of device • firmware version • firmware creation date
M[1-3]	Set mode 1,2 or 3 (RGB only, lx only, RGB and lx simultaneously)
*/

enum rgb_mode {
	RGB_UNKNOWN = 0,
	RGB_DEFAULT = 1,
	RGB_LUX		= 2,
	RGB_ALL		= 3
};


class RGB: public Atlas {
	public:
		RGB() {
			_rgb_mode = RGB_UNKNOWN;
			_saturated = false;
		}
		void		initialize();
		void		initialize(const rgb_mode mode);
		tristate	querySingleReading();
		void		enableContinuousReadings();
		void		disableContinuousReadings();
		tristate	queryInfo();
		tristate	setMode(const rgb_mode mode);
		rgb_mode	getMode(){return _rgb_mode;}
		char *		getFirmwareVer(){return _firmware_version;}
		char *		getFirmwareDate(){return _firmware_date;}
		int16_t		getRed() const {return _red;}
		int16_t		getGreen() const {return _green;}
		int16_t		getBlue() const {return _blue;}
		int16_t		getLuxRed() const {return _lx_red;}
		int16_t		getLuxGreen() const {return _lx_green;}
		int16_t		getLuxBlue() const {return _lx_blue;}
		int16_t		getLuxTotal() const {return _lx_total;}
		int16_t		getLuxBeyond() const {return _lx_beyond;}
		bool		getSaturated() const {return _saturated;}
		char		red[RGB_DATA_LEN];
		char		green[RGB_DATA_LEN];
		char		blue[RGB_DATA_LEN];
		char		lx_red[RGB_DATA_LEN];     // Values are 0 - 3235
		char		lx_green[RGB_DATA_LEN];
		char		lx_blue[RGB_DATA_LEN];
		char		lx_total[RGB_DATA_LEN];  // Should be = lx_red + lx_green + lx_blue + lx_non_vis.
		char		lx_beyond[RGB_DATA_LEN]; // lx beyond visible light spectrum
	protected:
	private:
		void		_sendCommand(const char * command, const bool has_result);
		void		_sendCommand(const char * command, const bool has_result,const uint16_t result_delay);
		rgb_mode	_rgb_mode;
		int16_t		_red;
		int16_t		_green;
		int16_t		_blue;
		int16_t		_lx_red;
		int16_t		_lx_green;
		int16_t		_lx_blue;
		int16_t		_lx_total;
		int16_t		_lx_beyond;
		bool		_saturated;
		char		_firmware_version[10];
		char		_firmware_date[10];
};
#endif
