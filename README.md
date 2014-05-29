Protocols-Arduino
=================

***Most Current Protocol File: Arduino\_Combined\_21***

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
'9' print MultispeQ Ready
'8' run light tests and sensor tests
'7' power off lights
'6' power off
'5' check battery level
'4' run all light tests
'3' print calibration values

FINAL NOTE: Teensy 3.1 sometimes has Serial connection problems in Windows.  If you are using windows and you cannot get the teensy to appear on list of serial devices in Arduino or you keep losing the connection, try unplugging the device, waiting at least 5 seconds, then plugging it back in.  Hopefully that works.  Otherwise, you could try switching USB locations or sometimes the cables themselves are bad.


_Feel free to contact me at gbathree at gmail dot com with questions._
