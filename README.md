# Arduino/C++ Library for Atlas Scientific sensors. #

## Currently supports the following sensors: ##

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

* Examples
* Not tested on ORP sensor (don't have one)
* keywords.txt for Arduino IDE
* Temperature logger
* Need to finish calibrate() methods for DO,EC,ORP, PH
* Need to add I2C functionality for EZO instruments. Some hooks provided.
* Auto-set baud rate?

One possible example would be a terminal program allowing user to pick UART and baud rate, then issue commands using methods. (partially done)

## Links:##

[Atlas Scientific](http://www.atlas-scientific.com/)

[ENV-TMP-D datasheet](http://www.atlas-scientific.com/_files/_datasheets/_probe/ENV-TEMP-D.pdf)