// Atlas Scientific RGB sensor library 
// Based on Atlas Scientific ENV-RGB document V 1.6 (ENV-RGB.pdf)
// 4/12/2014 by Ryan Neve <Ryan@PlanktosInstruments.com>
//

/* ============================================
Atlas Scientific RGB sensor library code is placed under the MIT license
Copyright (c) 2014 Ryan Neve

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

#ifndef _Atlas_RGB_h
#define _Atlas_RGB_h

#include <stdint.h>
#include <HardwareSerial.h>
// To which serial port is your device attached
#define SERIAL_RGB Serial1
// Whic pin powers the sensor
#define RGB_POWER_PIN 12

// Log levels (used for my SD logging system)
#define DEBUG 0
#define INFO 1
#define WARN 2
#define ERROR 3
#define CRITICAL 4

class AtlasRGB {
  public:
    AtlasRGB(HardwareSerial *serial);
    void          initialize(uint8_t mode, bool quiet);
    bool          testConnection();
    uint8_t       getRGB();
    // Setters
    void          setMode(uint8_t desired_mode);
    void          setQuiet();
    void          setContinuous();
    // Getters
    void          getInfo();
    bool          getContinuous();
    uint8_t       getMode();
    uint16_t      getRed();
    uint16_t      getBlue();
    uint16_t      getGreen();
    uint16_t      getLxRed();
    uint16_t      getLxBlue();
    uint16_t      getLxGreen();
    uint16_t      getLxTotal();
    uint16_t      getLxBeyond();
    bool          getSat();
    uint32_t      getMillisTime();
    // Utility
    void          printRGB();
  private:
    void          _getPromptedResponse(uint32_t timeout);
    void          _getContinuousResponse(uint32_t timeout);
    uint16_t      _clearBuffer(); 
    
    HardwareSerial*  _RGBHardSerial;
    bool             _continuous; // true = continuous, false = prompted.
    uint8_t          _rgb_mode; // 1 (RGB), 2 (lx) or 3 (RGB+lx)
    char             _red[6];        // values are saved as char[] to make logging 
    char             _green[6];      // to SD easier. values are 0 - 255
    char             _blue[6];
    char             _lx_red[6];     // Values are 0 - 3235
    char             _lx_green[6];
    char             _lx_blue[6];
    char             _lx_total[6];  // Should be = lx_red + lx_green + lx_blue + lx_non_vis.
    char             _lx_beyond[6]; // lx beyond visible light spectrum
    bool             _light_sat;    // More than 3235 lx detected.
    char             sample_time[7]; // HHmmss\0 if you have a RTC (not implemented)
    uint32_t         _millis_time;   // Sample time in millis()
    char             _device_type;
    char             _firmware[8];
    char             _firmware_date[8];
    char             comm_error;
    char             _read_char;
    char             _rgb_buffer[50];
    char             _BUFFER_SIZE; // Max size
    uint8_t          _buffer_len; // current size of _rgb_buffer
    
};
//Logger_SD logger(uint8_t);
#endif