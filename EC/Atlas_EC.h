// Atlas Scientific EC EZO sensor library 
// Based on Atlas Scientific EC_EZO document V 1.8 
// 9/20/2014 by Ryan Neve <Ryan@PlanktosInstruments.com>
//

/* ============================================
Atlas Scientific EC EZO sensor library code is placed under the MIT license
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

#ifndef _Atlas_EC_EZO_h
#define _Atlas_EC_EZO_h

#include <stdint.h>
#include <HardwareSerial.h>

// Log levels (used for my SD logging system)
#define DEBUG 0
#define INFO 1
#define WARN 2
#define ERROR 3
#define CRITICAL 4

class AtlasEC {
  public:
    AtlasEC(HardwareSerial *serial, uint8_t power_pin);
    void          initialize(bool quiet);
    bool          testConnection();
    void          quietMode();
    void          continuousMode();
    void          readData();  // Call this before the next four.
    float         getEC()  { return _ec; }
    float         getTDS() { return _tds; }
    float         getSal() { return _sal; }
    float         getSG()  { return _sg; }
    void          readOutput(); // Will set private variables
    bool          getOutputEC()  { return _output_EC  ; }
    bool          getOutputTDS() { return _output_TDS ; }
    bool          getOutputS()   { return _output_S   ; }
    bool          getOutputSG()  { return _output_SG  ; }
    void          setOutput(char * parameter,bool mode);
    void          readStatus(); // sets private variables
    bool          getInfo();
    void          setLED(bool mode); // 0 = off, 1 = on
    bool          getLED();
    void          setResponseMode(bool mode);
    bool          getResponseMode();
    void          sleep();
    void          wake();
    bool          isSleeping() { return _sleeping; }
    float         getStatusVoltage() { return _status_voltage; }
    char *        getVersion() { return _version; }
    float         getProbeK();
    void          setTemperature(float temp_C);
    float         getTemperature();
    bool          debug;
    
    // Not implemented yet
    bool          getMode();
    void          setName(char * name);
    void          getName(char * name);
    char          getStatusRestartCode();
    

  private:
    void             _getResponse(uint16_t timeout);
    void             _preCommand();
    uint8_t          _clearInputBuffer();
    uint8_t          _power_pin;
    HardwareSerial*  _ECHardSerial;
    bool             _continuous; // true = continuous, false = prompted.
    char             _read_char;
    char             _buffer[50];
    char             _BUFFER_MAX; // Max size
    uint8_t          _buffer_len; // current size of _buffer
    char             _status_restart_code;
    char             _version[4];   // EC EZO firmware version
    float            _status_voltage;
    bool             _sleeping;
    float            _probe_K; 
    bool             _output_EC;
    bool             _output_TDS;
    bool             _output_S;
    bool             _output_SG;
    float            _tempComp;
    float            _ec;
    float            _tds;
    float            _sal;
    float            _sg;
   
};
#endif