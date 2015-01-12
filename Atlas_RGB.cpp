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
#include <arduino.h>
#include <HardwareSerial.h>
#include "Atlas_RGB.h"

//#include <Logger_SD.h>// for my SD logging system

/*----------( Define Global Objects)----------*/

// Instantiate with something like <name> = AtlasRGB(&SERIAL_RGB);

/** Default constructor
 * @see SERIAL_RGB
 */
AtlasRGB::AtlasRGB(HardwareSerial *serial){
  _RGBHardSerial = serial;
}

/** Power on and prepare for general usage.
 * mode is 1 - 3 where 1 is the least data and 3 is the most.
 */
void AtlasRGB::initialize(uint8_t mode, bool quiet) {
  _BUFFER_SIZE = 50;
  pinMode(RGB_POWER_PIN,OUTPUT);
  digitalWrite(RGB_POWER_PIN,HIGH);
  _RGBHardSerial->begin(38400);
  if ( quiet ) setQuiet();
  else setContinuous();
  setMode(mode);
}

/** Verify the serial connection and make sure this is the device we think it is.
 */
bool AtlasRGB::testConnection(){
  getInfo();
  return ( _device_type == 'C' );
}

/** Prompt the RGB sensor for a string of readings.
 * Parse these values and save as class attributes.
 * Returns an error code which cab be used for debugging.
 */
uint8_t AtlasRGB::getRGB(){
  uint8_t field = 1;
  char temp_buffer[6];
  uint8_t temp_idx = 0;
  if ( getContinuous() ) _getContinuousResponse(2000);
  else {
    _rgb_buffer[0] = 'R'; _rgb_buffer[1] = 13; _rgb_buffer[2] = 0; _buffer_len = 3;
    _getPromptedResponse(1200);
  }
  _light_sat = 0; // Assume not light_sat
  //Logger_SD::Instance()->msgL(DEBUG,"_MODE is %d",_rgb_mode);
  for ( uint8_t i = 0 ; i < _buffer_len ; i++) {
    Serial.print(_rgb_buffer[i]);
    if ( i >= (_BUFFER_SIZE - 1)) return 1; // Array to big
    else if ( _rgb_buffer[i] == ' ')    return 2; // value includes space
    else if ( _rgb_buffer[i] == 'R')    continue; // Sometimes we see the R we sent as a command.
    else if ( _rgb_buffer[i] == '*' )  _light_sat = 1; // We're getting saturation
    else if ( _rgb_buffer[i] == ',' || _rgb_buffer[i] == 13 || _rgb_buffer[i] == 10) {
      temp_buffer[temp_idx] = '\0';
      switch (field) {
        case 1:
          //Logger_SD::Instance()->msgL(DEBUG,"first RGB field is >%s< with index %d",temp_buffer,temp_idx);
          if ( _rgb_mode == 1 || _rgb_mode == 3 ) memcpy(_red,   temp_buffer,temp_idx+1);
          else memcpy(_lx_red,temp_buffer,temp_idx+1);
          break;
        case 2:
          //Logger_SD::Instance()->msgL(DEBUG,"second RGB field is %s",temp_buffer);
          if ( _rgb_mode == 1 || _rgb_mode == 3 ) memcpy(_green,   temp_buffer,temp_idx+1);
          else memcpy(_lx_green,temp_buffer,temp_idx+1);
          break;
        case 3:
          //Logger_SD::Instance()->msgL(DEBUG,"third RGB field is %s",temp_buffer);
          if ( _rgb_mode == 1 || _rgb_mode == 3 ) memcpy(_blue,   temp_buffer,temp_idx+1);
          else memcpy(_lx_blue,temp_buffer,temp_idx+1);
          break;
        case 4:
          //Logger_SD::Instance()->msgL(DEBUG,"fourth RGB field is %s",temp_buffer);
          if ( _rgb_mode == 1 ) return 7;
          else if ( _rgb_mode == 2 ) memcpy(_lx_total,temp_buffer,temp_idx+1);
          else memcpy(_lx_red,  temp_buffer,temp_idx+1);
          break;
        case 5:
          //Logger_SD::Instance()->msgL(DEBUG,"fifth RGB field is %s",temp_buffer);
          if ( _rgb_mode == 1 ) return 7;
          else if ( _rgb_mode == 2 ) memcpy(_lx_beyond,temp_buffer,temp_idx+1);
          else memcpy(_lx_green, temp_buffer,temp_idx+1);
          break;
        case 6:
          //Logger_SD::Instance()->msgL(DEBUG,"sixth RGB field is %s",temp_buffer);
          if ( _rgb_mode != 3 ) return 7;
          memcpy(_lx_blue,temp_buffer,temp_idx+1);
          break;
        case 7:
          //Logger_SD::Instance()->msgL(DEBUG,"seventh RGB field is %s",temp_buffer);
          if ( _rgb_mode != 3 ) return 7;
          memcpy(_lx_total,temp_buffer,temp_idx+1);
          break;
        case 8:
          //Logger_SD::Instance()->msgL(DEBUG,"eighth RGB field is %s",temp_buffer);
          if ( _rgb_mode != 3 ) return 7;
          memcpy(_lx_beyond,temp_buffer,temp_idx+1);
          break;
        default: return 7;
      } // end switch
      field++; temp_idx = 0;
      if ( _rgb_buffer[i] == 10 || _rgb_buffer[i] == 13) break; // out of for loop
    }
    else {
      temp_buffer[temp_idx] = _rgb_buffer[i];
      temp_idx++;
    }
  }
  _millis_time = millis();
  return 3;  // Saw no characters
}

/** Set the RGB sensor to the desired _rgb_mode.
 * Checks response for confirmation.
 */
void AtlasRGB::setMode(uint8_t desired_mode){
  _rgb_buffer[0] = 'M'; _rgb_buffer[1] = desired_mode + '0'; _rgb_buffer[2] = 13; _rgb_buffer[3] = 0; _buffer_len = 4;
  //Logger_SD::Instance()->msgL(DEBUG,"setMode sending %s.",_rgb_buffer);
  _getPromptedResponse(2000); //The ENV-RGB will respond: RGB<CR> or lx<CR> or RGB+lx<CR>
  // The ENV-RGB will respond: RGB<CR> or lx<CR> or RGB+lx<CR>
  _rgb_mode = 0; // reset _rgb_mode
  for ( uint8_t i = 0 ; i < _buffer_len ; i++ ) {
    Serial.print(_rgb_buffer[i]);
    if ( _rgb_buffer[i] == 'R' ) _rgb_mode +=1;
    else if ( _rgb_buffer[i] == 'l' ) _rgb_mode += 2;
    else if ( _rgb_buffer[i] == 13 ) break;
  }
  if ( _rgb_mode != desired_mode ) {
    //Logger_SD::Instance()->msgL(WARN," Requested _rgb_mode %d but got _rgb_mode %d (%s)",desired_mode,_rgb_mode,_rgb_buffer);
    _rgb_mode = 3; //TEMPORARY FIX FOR UNKNOWN COMMUNICATIONS PROBLEM RYAN
  }
  else {
    //Logger_SD::Instance()->msgL(DEBUG,"Set Light output _rgb_mode to %d.",_rgb_mode);
  }
}

/** Set the RGB sensor to quiet mode.
 */
void AtlasRGB::setQuiet() {
  //Logger_SD::Instance()->msgL(DEBUG,"Setting RGB mode to quiet");
  _rgb_buffer[0] = 'E'; _rgb_buffer[1] = 13; _rgb_buffer[2] = 0; _buffer_len = 3;
  _continuous = false;
  _getPromptedResponse(3000);
}

/** Set the RGB sensor to continuous _rgb_mode.
 *  This isn't really handled yet, so use setQuiet().
 */
void AtlasRGB::setContinuous() {
  _continuous = true;
  _RGBHardSerial->write('C');
  _RGBHardSerial->write(13);
}

/** Prompt the RGB sensor for version information.
 * Parse these values and save as class attributes.
 */
void AtlasRGB::getInfo(){
  _rgb_buffer[0] = 'I'; _rgb_buffer[1] = 13; _rgb_buffer[2] = 0; _buffer_len = 3;
  //strncpy(_rgb_buffer,"I\r\0",3);
  _getPromptedResponse(500);
  uint8_t field = 1;
  char temp_buffer[10];
  uint8_t temp_idx = 0;
  for ( uint8_t i = 0 ; i < _buffer_len ; i++ ) {
    if ( (_rgb_buffer[i] == ',') || (_rgb_buffer[i] == 13) ){
      switch (field) {
        case (1): _device_type = temp_buffer[temp_idx]; break;
        case (2): memcpy(_firmware,temp_buffer,temp_idx+1);break;
        case (3): memcpy(_firmware_date,temp_buffer,temp_idx+1);return;
        default: return;
      }
      if ( _rgb_buffer[i] == 13 ) break;
      field++;
      temp_idx = 0;
    }
    else {
      temp_buffer[temp_idx] = _rgb_buffer[i];
      temp_idx++;
    }
  }
}

/** Sends contents of _rgb_buffer[] to the RGB sensor.
 *  Saves response back to _rgb_buffer[] until a CR is seen.
 *  Saves number of characters received to _buffer_len.
 */
void AtlasRGB::_getPromptedResponse(uint32_t timeout) {
  _buffer_len = 0; // will be incremented
  uint16_t i = 0;
  while (_RGBHardSerial->available()) {// Clear out read buffer.
    _RGBHardSerial->read();
    i++;
  }
  //if (i) Logger_SD::Instance()->msgL(DEBUG,"Cleared %d bytes from serial port",i);
  _RGBHardSerial->print(_rgb_buffer);
  _RGBHardSerial->flush(); // Wait for bytes to be written.
  //Logger_SD::Instance()->msgL(DEBUG,"Sent [%s] to RGB sensor",_rgb_buffer);
  uint32_t read_begin = millis();
  while ((millis() - read_begin) <= timeout ){
     if ( _RGBHardSerial->available() ) {
        _read_char = _RGBHardSerial->read();
        _rgb_buffer[_buffer_len] = _read_char;
        _buffer_len++;
        if ( _read_char == 13 ) break;
        if ( _buffer_len == (_BUFFER_SIZE - 1)) break; // Should never get this big!
        //if ((millis() - read_begin)  >= (timeout - 100) ) read_begin += 100; // Add a little time if we're near the end
     }
     delay(1);
  }
  if ((millis() - read_begin) > timeout ) Serial.println("Timed out waiting for Reply");
  _rgb_buffer[_buffer_len] = '\0'; // For safety
  //Logger_SD::Instance()->msgL(DEBUG,"Got buffer [%s] of length %d.",_rgb_buffer,_buffer_len);

}

/** Saves response between first and second CR back to _rgb_buffer[].
 *  Saves number of characters received to _buffer_len.
 */
void AtlasRGB::_getContinuousResponse(uint32_t timeout) {
  _buffer_len = 0; // will be incremented
  //uint16_t bytes_cleared = _clearBuffer();
  //if (bytes_cleared) Logger_SD::Instance()->msgL(DEBUG,"Cleared %d old bytes from buffer",bytes_cleared);
  uint32_t read_begin = millis();
  uint16_t bytes_read = 0;
  while ((millis() - read_begin ) <= timeout ) { // First ignore everything until first CR 
    if ( _RGBHardSerial->available() ) {
      _read_char = _RGBHardSerial->read();
      bytes_read++;
      if ( _read_char == 13 ) break;
    }
  }
  //if (bytes_read) Logger_SD::Instance()->msgL(DEBUG,"Cleared %d new bytes looking for CR",bytes_read);
  read_begin = millis();
  while ((millis() - read_begin) <= timeout ){ // now record anything up to the next CR or timeout
     if ( _RGBHardSerial->available() ) {
        _read_char = _RGBHardSerial->read();
        _rgb_buffer[_buffer_len] = _read_char;
        _buffer_len++;
        if ( _read_char == 13 ) break; // Got our terminating CR
        if ( _buffer_len == (_BUFFER_SIZE - 1)) break; // Should never get this big!
        //if ((millis() - read_begin) < 100 ) read_begin += 100; // Add a little time if we're near the end
     }
  }
  if ((millis() - read_begin) > timeout ) Serial.println("Timed out waiting for Reply");
  _rgb_buffer[_buffer_len] = '\0'; // For safety
  //Logger_SD::Instance()->msgL(DEBUG,"Got buffer [%s] of length %d.",_rgb_buffer,_buffer_len);
}

/** clear any characters in buffer
 */
uint16_t AtlasRGB::_clearBuffer(){
  uint16_t i = 0;
  while ( _RGBHardSerial->available() ) {
    _RGBHardSerial->read();
    i++;
  }
  return i;
}
/** Prints values of RGB data
 */
void AtlasRGB::printRGB(){
  Serial.println("\r\nLIGHT readings");
  Serial.print("     red       "); Serial.println(_red); 
  Serial.print("     green     "); Serial.println(_green); 
  Serial.print("     blue      "); Serial.println(_blue); 
  Serial.print("     lx_red    "); Serial.println(_lx_red);
  Serial.print("     lx_green  "); Serial.println(_lx_green);
  Serial.print("     lx_blue   "); Serial.println(_lx_blue);
  Serial.print("     lx_total  "); Serial.println(_lx_total);
  Serial.print("     lx_beyond "); Serial.println(_lx_beyond);
  Serial.print("     light_sat "); Serial.println(_light_sat);
}
bool     AtlasRGB::getContinuous() { return _continuous;}
uint8_t  AtlasRGB::getMode()      { return _rgb_mode; }
uint16_t AtlasRGB::getRed()       { return atoi(_red); }
uint16_t AtlasRGB::getBlue()      { return atoi(_blue); }
uint16_t AtlasRGB::getGreen()     { return atoi(_green); }
uint16_t AtlasRGB::getLxRed()     { return atoi(_lx_red); }
uint16_t AtlasRGB::getLxBlue()    { return atoi(_lx_blue); }
uint16_t AtlasRGB::getLxGreen()   { return atoi(_lx_green); }
uint16_t AtlasRGB::getLxTotal()   { return atoi(_lx_total); }
uint16_t AtlasRGB::getLxBeyond()  { return atoi(_lx_beyond); }
bool     AtlasRGB::getSat()       { return _light_sat; }
uint32_t AtlasRGB::getMillisTime() { return _millis_time; }