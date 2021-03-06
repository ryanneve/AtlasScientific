# Arduino/C++ Library for Atlas Scientific sensors. #

## Currently supports the following sensors: ##

* NEW! - EZO RGB
* EZO DO
* EZO EC
* EZO ORP
* EZO PH
* RGB sensor

## Functionality: ##

* Circuit can be instantiated on any Serial port. Works with multiplexed ports
* (almost) All commands supported.
* Baud rate can be changed


## To be done: ##

* Put in proper Arduino Library format. See https://github.com/arduino/Arduino/wiki/Arduino-IDE-1.5:-Library-specification
* Rename Atlas.* to atlasscientific.*
* Test on non-Mega2560 arduino
* Examples
* Not tested on ORP sensor (don't have one)
* keywords.txt for Arduino IDE
* Temperature logger. Will probablt just wait for EZO version due out soon.
* Need to finish calibrate() methods for DO,EC,ORP, PH
* Need to add I2C functionality for EZO instruments. Some hooks provided.

One possible example would be a terminal program allowing user to pick UART and baud rate, then issue commands using methods. (partially done)

## Links:##

[Atlas Scientific](http://www.atlas-scientific.com/)

[ENV-TMP-D datasheet](http://www.atlas-scientific.com/_files/_datasheets/_probe/ENV-TEMP-D.pdf)