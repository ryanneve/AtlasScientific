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

#include <arduino.h>
#include <HardwareSerial.h>
#include "Atlas_EC.h";

#include <Logger_SD.h>;

/*----------( Define Global Objects)----------*/

// Instantiate with something like <name> = AtlasEC(&SERIAL_EC,12);

/** Default constructor
 * 
 */
AtlasEC::AtlasEC(HardwareSerial *serial, uint8_t power_pin){
  //_ECHardSerial = serial;
  _ECHardSerial = &Serial3;
  _power_pin = power_pin;
}


void AtlasEC::initialize( bool quiet) {
  /** Power on and prepare for general usage.
   * mode is 1 - 3 where 1 is the least data and 3 is the most.
   */
   // Initialize variables
  _BUFFER_MAX = 50;
  _buffer_len = 0;
  _version[0] = 0;
  debug = false;
  _sleeping = false;
  // Power on sensor
  pinMode(_power_pin,OUTPUT);
  digitalWrite(_power_pin,HIGH);
  _ECHardSerial->begin(38400);
  wake();
  delay(1000);
  _clearInputBuffer();
  if ( quiet ) quietMode();
  else continuousMode();
  readOutput();
  readStatus();
}

bool AtlasEC::testConnection(){
  return ( getInfo() );
}

void AtlasEC::quietMode() {
  /** Set the EC sensor to quiet mode.
  */
  //Logger_SD::Instance()->msgL(DEBUG,"Setting EC mode to quiet");
  _preCommand();
  _ECHardSerial->print("C,0\n");
  _continuous = false;
  _getResponse(3000);
  if ( debug ) {
    Serial.print("Response to 'C,0\n':"); Serial.println(_buffer); 
  }
}

void AtlasEC::continuousMode() {
  /** Set the EC sensor to continuous mode.
  */
  //Logger_SD::Instance()->msgL(DEBUG,"Setting EC mode to continuous");
  _preCommand();
  _ECHardSerial->print("C,1\n");
  _continuous = true;
}

void AtlasEC::readOutput() {
  // Get query output
  _preCommand();
  _ECHardSerial->print("O,?\n");
  _getResponse(3000);
  if ( debug ) {
    Serial.print("Response to 'O,?\n':"); Serial.println(_buffer); 
  }
  _output_EC  = 0;
  _output_TDS = 0;
  _output_S   = 0;
  _output_SG  = 0;
    // now parse _buffer. we expect:
  // ?O,EC,TDS,S,SG
  // if all are enabled
  for ( uint8_t c = 2 ; c <= _buffer_len ; c++ ) {
    // look for a 1 or 0
    if ( _buffer[c] == ',' && _buffer[c+1] == 'E' && _buffer[c+2] == 'C') {
      _output_EC = 1;
      continue;
    }
    if ( _buffer[c] == ',' && _buffer[c+1] == 'T' && _buffer[c+2] == 'D' && _buffer[c+2] == 'S') {
      _output_TDS = 1;
      continue;
    }
    if ( _buffer[c] == ',' && _buffer[c+1] == 'S' && _buffer[c+2] != 'G') {
      _output_S = 1;
      continue;
    }
    if ( _buffer[c] == ',' && _buffer[c+1] == 'S' && _buffer[c+2] == 'G') {
      _output_SG = 1;
      continue;
    }
  }
}

void AtlasEC::setOutput(char * parameter,bool mode){
  // parameter should be a null terminated string containing one of the following:
  // "EC", "TDS", "S", "SG"
  _preCommand();
  _ECHardSerial->print("O,");
  _ECHardSerial->print(parameter);
  _ECHardSerial->write(',');
  _ECHardSerial->write(mode);
  _ECHardSerial->write(13);
}

void AtlasEC::readStatus() {
  // Get query output
  _preCommand();
  char voltage_str[10];
  voltage_str[0] = 0;
  uint8_t volt_str_idx = 0;
  _ECHardSerial->print("STATUS\n");
  _getResponse(3000);
  _status_restart_code = 0;
  // expect something like "?STATUS,P,5.038<CR>"
  for ( uint8_t c = 0 ; c <= _buffer_len ; c++ ) {
    if ( _buffer[c] == ',' ) {
      if ( _status_restart_code == 0 ) {
        _status_restart_code = _buffer[c+1];
      }
      else {
        voltage_str[volt_str_idx] = _buffer[c+1];
        c++; // skip one.
        volt_str_idx++;
        voltage_str[volt_str_idx] = 0; // null terminate
      }
    }
    else if ( volt_str_idx > 0 ) {
      if ( _buffer[c] >= '0' && _buffer[c] <= '9' ) {
        voltage_str[volt_str_idx] = _buffer[c];
        volt_str_idx++;
        voltage_str[volt_str_idx] = 0; // null terminate
      }
    }
  }
  if ( voltage_str[0] != 0 ) {
    _status_voltage = atof(voltage_str); // convert string to float.
  }
}

bool AtlasEC::getInfo(){
  // Should Respond:
  // ?I,EC,1.0<CR>
  _preCommand();
  _ECHardSerial->print("I\n");
  _getResponse(3000);
  bool result = 0;
  if ( _buffer[1] == 'I' ) {
    if ( _buffer[3] == 'E' && _buffer[4] == 'C' ) {
      result = 1;
      _version[0] = _buffer[6];
      _version[1] = _buffer[7];
      _version[2] = _buffer[8];
      _version[3] = 0;
    }
  }
  return result;
}

void AtlasEC::setLED(bool mode){
  // Turns LEDs on {1} or off {0}
  _preCommand();
  _ECHardSerial->print("L,");
  _ECHardSerial->write(mode);
  _ECHardSerial->write(13); //<CR>
}

bool AtlasEC::getLED(){
  _preCommand();
  _ECHardSerial->print("L,?\n");
  _getResponse(3000);
  // Expect "?L,n<CR>" where n is 1 or 0
  if ( _buffer[0] == '?' ) {
    if ( _buffer[1] == 'L' ) {
      if ( _buffer[3] == '1' ) return 1;
    }
  }
  return 0;
}

bool AtlasEC::getResponseMode(){
  _preCommand();
  _ECHardSerial->print("Response,?\n");
  _getResponse(3000);
  // Expect "?RESPONSE,n<CR>" where n is 1 or 0
  if ( _buffer[0] == '?' ) {
    if ( _buffer[1] == 'R' ) {
      if ( _buffer[10] == '1' ) return 1;
    }
  }
  return 0;
}

void AtlasEC::setResponseMode(bool mode){
  _preCommand();
  _ECHardSerial->print("Response,");
  _ECHardSerial->write(mode);
  _ECHardSerial->print(13); // <CR>
}

void AtlasEC::sleep(){
  _preCommand();
  _ECHardSerial->print("SLEEP\n");
  _sleeping = true;
}

void AtlasEC::wake(){
  _preCommand();
  _ECHardSerial->print("\n");
  _sleeping = false;
  _getResponse(3000); // clear out "*WA" response code.
}

float AtlasEC::getProbeK(){
  _preCommand();
  _ECHardSerial->print("K,?\n");
  _getResponse(3000);
  // Expect "?K,x<CR>" where x is floating point value of K
  if ( _buffer[0] == '?'  && _buffer[1] == 'K' ) {
    char buf[10]; uint8_t buf_idx = 0;
    for ( uint8_t i = 2 ; i < 10 ; i++ ) {
      if ( _buffer[i] >= '.' ) {
        buf[buf_idx] = _buffer[i];
        buf_idx++;
        buf[buf_idx] = 0;
      }
      else { // got <CR>
         _probe_K = atof(buf); // convert string to float
         return _probe_K;
      }
    }
  }
  return 0;
}

float AtlasEC::getTemperature(){
  _preCommand();
  _ECHardSerial->print("T,?\n");
  _getResponse(3000);
  // Expects "?T,x<CR>" where x is a float.
  // Expect "?K,x<CR>" where x is floating point value of K
  if ( _buffer[0] == '?' && _buffer[1] == 'T' ) {
    char buf[10]; uint8_t buf_idx = 0;
    for ( uint8_t i = 2 ; i < 10 ; i++ ) {
      if ( _buffer[i] >= '.' ) {
        buf[buf_idx] = _buffer[i];
        buf_idx++;
        buf[buf_idx] = 0;
      }
      else { // got <CR>
         _tempComp = atof(buf); // convert string to float
         return _tempComp;
      }
    }
  }
  return 0;
}

void AtlasEC::setTemperature(float temp_C){
  _preCommand();
  char buf[10];
  dtostrf(temp_C,5,2,buf); // convert float to string
  _ECHardSerial->print("T,");
  _ECHardSerial->print(buf);
  _ECHardSerial->write(13);
}

void AtlasEC::readData(){
  _preCommand();
  _ECHardSerial->print("R\n");
  delay(1400); 
  _getResponse(3000);
  // Expects response like:
  // EC,TDS,SAL,SG<CR>
  char * pch;
  pch = strtok(_buffer,",\n");
  _ec = atof(pch);
  pch = strtok(NULL,",\n");
  _tds = atof(pch);
  pch = strtok(NULL,",\n");
  _sal = atof(pch);
  pch = strtok(NULL,",\n");
  _sg = atof(pch);
}
  
void AtlasEC::_getResponse(uint16_t timeout){
  uint32_t end_millis = millis() + timeout;
  _buffer_len = 0;
  _buffer[0] = 0;
  while ( millis() < end_millis ) {
    if ( _ECHardSerial->available() ) {
      _read_char = _ECHardSerial->read();
      _buffer[_buffer_len] = _read_char;
      _buffer_len++;
    }
    else delay(20);
  }
  //_ECHardSerial->setTimeout(timeout);
  //_buffer[0] = 0;
  //_buffer_len = _ECHardSerial->readBytesUntil(13,_buffer,_BUFFER_MAX);
  if ( debug ) {
    Serial.write('>'); Serial.print (_buffer);
    Serial.write('-'); Serial.println(_buffer_len);
  }
}

void AtlasEC::_preCommand(){
  // Run this before any command to clear serial buffer.
  if ( _sleeping ) wake();
  _clearInputBuffer();
}

uint8_t AtlasEC::_clearInputBuffer(){
  uint8_t count = 0;
  while ( _ECHardSerial->read() >= 0 ){
     count++;
     delay(20);
   }
   if ( debug && count) {
     Serial.print("cleared "); Serial.println(count);
   }
   return count;
}