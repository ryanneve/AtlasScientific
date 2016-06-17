/*============================================================================
Atlas Scientific EZO RGB sensor library code is placed under the GNU license
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
EZO RGB	v??
============================================================================*/
#ifndef Atlas_EZO_RGB_h
#define Atlas_EZO_RGB_h


#include "Arduino.h"
#include <Atlas_EZO.h>
/*-------------------- RGB --------------------*/


enum ezo_rgb_output {
	EZO_RGB_UNKNOWN		= 0,
	EZO_RGB_OUT_RGB		= 1,
	EZO_RGB_OUT_PROX	= 2,
	EZO_RGB_OUT_LUX		= 4,
	EZO_RGB_OUT_CIE		= 8
};

class EZO_RGB: public EZO {
public:
	EZO_RGB() {
		_ezo_rgb_output = EZO_RGB_UNKNOWN;
		_rgb_output		= TRI_UNKNOWN;
		_prox_output	= TRI_UNKNOWN;
		_lux_output		= TRI_UNKNOWN;
		_cie_output		= TRI_UNKNOWN;
		_brightness		= -1; // unknown, will be 0 - 100
		_auto_bright	= TRI_UNKNOWN;
		_prox_distance	= -1; // unknown. Will be 0-1023
		_matching		= TRI_UNKNOWN;
		_gamma_correction	= 0.00; // not a valid number. Should be 0.01 to 4.99
		strncpy(_reset_command, "Factory",8); // special for RGB
	}
	void			initialize(); //uses defaults
	void			initialize(int8_t brightness,tristate auto_bright,int16_t prox_distance, int8_t ir_brightness);
	ezo_response	queryOutput();
	tristate		getOutput(ezo_rgb_output output);
	void			printOutputs();
	ezo_response	enableOutput(ezo_rgb_output output);
	ezo_response	disableOutput(ezo_rgb_output output);

	ezo_response	calibrate();
	ezo_response	setLEDbrightness(int8_t brightness); // Brightness is 0 to 100
	ezo_response	setLEDbrightness(int8_t brightness,tristate auto_led);
	ezo_response	setLEDbrightness(int8_t brightness,bool auto_led); // Brightness is 0 to 100
	ezo_response	queryLEDbrightness(); // Brightness is 0 to 100
	int8_t			getLEDbrightness() {return _brightness;}
	// NEW section - implemented
	ezo_response	enableProximity();
	ezo_response	enableProximity(int16_t distance);
	ezo_response	proximityLED_Low();
	ezo_response	proximityLED_Med();
	ezo_response	proximityLED_High();
	ezo_response	disableProximity();
	ezo_response	queryProximity();
	// New section
	ezo_response	enableMatching();
	ezo_response	disableMatching();
	ezo_response	queryMatching();
	// New section
	ezo_response	setGamma(float gamma_correction);
	ezo_response	queryGamma();


	ezo_response	querySingleReading();
	int16_t			getRed() const {return _red;}
	int16_t			getGreen() const {return _green;}
	int16_t			getBlue() const {return _blue;}
	int16_t			getProx() const {return _prox;}
	int16_t			getLux() const {return _lux;}
	float			getCIE_x() const {return _cie_x;}
	float			getCIE_y() const {return _cie_y;}
	int32_t			getCIE_Y() const {return _cie_Y;}

	char			red[5];
	char			green[5];
	char			blue[5];
	char			prox[6];
	char			lux[7];
	char			cie_x[6];
	char			cie_y[6];
	char			cie_Y[7];
protected:
private:
	ezo_response	_changeOutput(ezo_rgb_output output,int8_t enable_output); //DONE

	int8_t		_brightness;	// 0 - 100 % -1 = unknown
	tristate	_auto_bright;
	int16_t		_prox_distance;
	int8_t		_IR_bright;
	tristate	_matching;
	float		_gamma_correction;
	ezo_rgb_output	_ezo_rgb_output;
	int16_t		_red;	// 0 - 255
	int16_t		_green;	// 0 - 255
	int16_t		_blue;	// 0 - 255
	int16_t		_prox;	// 0 to 1023 but usually above 250
	int32_t		_lux;	// 0 - 65535
	float		_cie_x;	// 0.0 to 0.85
	float		_cie_y;	// 0.0 to 0.85
	int32_t		_cie_Y;	// 0 to 65535

	tristate		_rgb_output;
	tristate		_prox_output;
	tristate		_lux_output;
	tristate		_cie_output;
};
#endif
