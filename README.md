Protocols-Arduino
=================

This repository contains the arduino protocols used to program the Teensy 3.1 on the MultispeQ.  It receives communication over USB or bluetooth as a JSON (the "Protocol JSON"), parses the JSON and inteprets what to do (flash lights, measure CO2, etc.) and takes the measurement as specified, then sends back the data also as a JSON (the "Data JSON").

***INSTALLING ARDUINO IDE, TEENSYDUINO AND APPROPRIATE LIBRARIES TO CONTRIBUTE/FORK PROTOCOLS-ARDUINO***

1. Go to Arduino.cc and follow the directions for installing the base Arduino IDE:
http://arduino.cc/en/Main/Software.

2. The Photosynq device uses the Teensy 3.1, an arduino compatable microcontroller which uses an ARM processor and 2 16 bit analog inputs - both important for making the fluorescence measurements effectively.  In order to use Teensy, you need to also download Teensyduino - a software add-on to the Arduino IDE.  This is required to be able to upload programs to your Photosynq device (or any Teensy).  So go download and install it here:
http://www.pjrc.com/teensy/td_download.html
(NOTE: installing teensyduino in Linux is a little trickier - make sure to follow the directions completely!)

3. Open additional libraries zip file (https://github.com/Photosynq/Protocols-Arduino/blob/master/additional%20libraries.zip?raw=true) for required libraries.  Unzip the files into the 'libraries' folder located in your arduino install location (in windows that's the c drive NOT in the my documents folder).  Some modifications have been made to these libraries, so you must REPLACE any existing libraries of the same name if you have them.  I would suggest saving the old versions of these before replacing, as other programs may not work after the replace.

4. You should now be able to plug your Photosynq device into a computer using a standard micro USB to USB cable.  

5. Download the most current protocol file and file folder (see top of this readme for the name) and place in your arduino sketchbook folder.  Make sure you place both the folder and the .ino file of the same name (for example, photosynq-21.ino and folder named photosynq-21) in the sketchbook folder!

6. Open the arduino software.  Open the Arduino IDE and go to Tools --> Board.  Select "Teensy 3.1" from the list.  Load the protocol by going to file-->sketchbook, and once loaded press the upload button - this flashes the MultispeQ with the new firmware.  Then go to tools --> serial port and select the port which contains the teensy.  Once that is checked, open the Arduino Serial Monitor (tools --> serial monitor).  You can copy and paste protocols through the serial port to run a measurement, or enter one of the following options for testing:


* 1000:     // print "MultispeQ Ready" to USB and Bluetooth
* 1001:     // power off completely (if USB connected, only batteries
* 1002:     // configure bluetooth name and baud rate
* 1003:     // power down lights (TL1963) only
* 1004:     // show battery level and associated values
* 1005:     // print all calibration data
* 1006*:    // add calibration values for tcs light sensor to actual PAR values     
* 1007:     // view device info
* 1008*:    // add calibration values for offset  
* 1011**:   // add calibration values for the lights     
* 1012**:   // add factory calibration values for of lights    
* 1013:     // view and set device info      
* 1014**:   // add calibration values for the baseline  
* 1015**:   // add calibration values for the spad blanks
* 1016:     // user defined calibrations, set by specific lights/detectors
* 1017:     // reset all EEPROM values to zero
* 1018:     // reset only the calibration arrays to zero (leave tcs calibration, offset, device info)
* 1019**:   // add user defined calibration values (2) (factory devices use this for SPAD factor)
* 1020:     // add user defined calibration values (2)
* 1021:     // add user defined calibration values (2)
* 1022:     // add user defined calibration values (2)
* 1023:     // add user defined calibration values (2)
* 1024:     // add user defined calibration values (2)
* 1025:     // add user defined calibration values (2)
* 1027:     // reboot teensy (used for loading hex files)
* 15 - measuring light 1 (main board)");
* 16 - measuring light 2 (main board)");
* 11 - measuring light 3 (add on board)");
* 12 - measuring light 4 (add on board)");
* 20 - actinic light 1 (main board)");
* 2 - actinic light 2 (add on board)");
* 14 - calibrating light 1 (main board)");
* 10 - calibrating light 2 (add on board)");
* 34 - detector 1 (main board)");
* 35 - detector 2 (add on board)");
* 101 - light detector testing (press any key to exit after entering))");
* 105 - light detector testing RAW SIGNAL (press any key to exit after entering))");
* 102 - CO2 testing (press any key to exit after entering))");
* 103 - relative humidity (press any key to exit after entering))");
* 104 - temperature (press any key to exit after entering))");
* 106 - contactless temperature testing (press any key to exit after entering - EXPERIMENTAL not implemented yet))");
* 99 followed by a light pin (15,16,11,12,20,2,10,14) - outputs the PAR value and matches ambient PAR to chosen lighten pin

*requires new menu items in chrome app
**requires wizard with new protocols and macros to generate calibration values

FINAL NOTE: Teensy 3.1 sometimes has Serial connection problems in Windows.  If you are using windows and you cannot get the teensy to appear on list of serial devices in Arduino or you keep losing the connection, try unplugging the device, waiting at least 5 seconds, then power the device on with power button, plug the usb back in.  Hopefully that works.  Otherwise, you could try switching USB locations or sometimes the cables themselves are bad.


_Feel free to contact me at gbathree at gmail dot com with questions._
