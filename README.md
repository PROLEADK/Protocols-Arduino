Protocols-Arduino
=================

***Most Current Protocol File: Arduino\_Combined\_28***

This repository contains the arduino protocols used to program the Teensy 3.1 on the MultispeQ.  It receives communication over USB or bluetooth as a JSON, parses the JSON and inteprets what to do (flash lights, measure CO2, etc.), takes the measurement as specified, then sends back the data also as a JSON.

***INSTALLING ARDUINO IDE, TEENSYDUINO AND APPROPRIATE LIBRARIES TO CONTRIBUTE/FORK PROTOCOLS-ARDUINO***

1. Go to Arduino.cc and follow the directions for installing the base Arduino IDE:
http://arduino.cc/en/Main/Software.

2. The Photosynq device uses the Teensy 3.1, an arduino compatable microcontroller which uses an ARM processor and 2 16 bit analog inputs - both important for making the fluorescence measurements effectively.  In order to use Teensy, you need to also download Teensyduino - a software add-on to the Arduino IDE.  This is required to be able to upload programs to your Photosynq device (or any Teensy).  So go download and install it here:
http://www.pjrc.com/teensy/td_download.html
(NOTE: installing teensyduino in Linux is a little trickier - make sure to follow the directions completely!)

3. Open additional libraries zip file (https://github.com/Photosynq/Protocols-Arduino/blob/master/additional%20libraries.zip?raw=true) for required libraries.  Unzip the files into the 'libraries' folder located in your arduino install location.  Some modifications have been made to these libraries, so you must REPLACE any existing libraries of the same name if you have them.  I would suggest saving the old versions of these before replacing, as other programs may not work after the replace.

4. You should now be able to plug your Photosynq device into a computer using a standard micro USB to USB cable.  

5. Download the most current protocol file and file folder (see top of this readme for the name) and place in your arduino sketchbook folder.  Make sure you place both the folder and the .ino file of the same name (for example, photosynq-21.ino and folder named photosynq-21) in the sketchbook folder!

6. Open the arduino software.  Open the Arduino IDE and go to Tools --> Board.  Select "Teensy 3.1" from the list.  Load the protocol by going to file-->sketchbook, and once loaded press the upload button - this flashes the MultispeQ with the new firmware.  Then go to tools --> serial port and select the port which contains the teensy.  Once that is checked, open the Arduino Serial Monitor (tools --> serial monitor).  You can copy and paste protocols through the serial port to run a measurement, or enter one of the following options for testing:


* case 1000:     // print "MultispeQ Ready" to USB and Bluetooth
* case 1001:     // power off completely (if USB connected, only batteries
* case 1002:     // configure bluetooth name and baud rate
* case 1003:     // power down lights (TL1963) only
* case 1004:     // show battery level and associated values
* case 1005:     // print all calibration data
* case 1006*:     // add calibration values for tcs light sensor to actual PAR values     
* case 1007:     // view device info
* case 1008*:     // add calibration values for offset  
* case 1011**:     // add calibration values for the lights     
* case 1012**:     // add factor calibration values for of lights    
* case 1013:     // view and set device info      
* case 1014**:     // add calibration values for the baseline  
* case 1015**:     // add calibration values for the spad blanks
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

*requires new menu items in chrome app
**requires wizard with new protocols and macros to generate calibration values

FINAL NOTE: Teensy 3.1 sometimes has Serial connection problems in Windows.  If you are using windows and you cannot get the teensy to appear on list of serial devices in Arduino or you keep losing the connection, try unplugging the device, waiting at least 5 seconds, then power the device on with power button, plug the usb back in.  Hopefully that works.  Otherwise, you could try switching USB locations or sometimes the cables themselves are bad.


_Feel free to contact me at gbathree at gmail dot com with questions._
