Protocols-Arduino
=================

***See additional libraries zip file for required libraries (to be placed in the 'libraries' folder in your arduino install location).  Some modifications have been made to these libraries, so you must REPLACE any existing libraries of the same name if you have them.  I would suggest saving the old versions of these files, as other programs may not work after the replace***
***Most Current Protocol File: Arduino\_Combined\_21***

This repository contains the arduino protocols used to program the Teensy 3.1 on the MultispeQ.  It receives communication over USB or bluetooth as a JSON, parses the JSON and inteprets what to do (flash lights, measure CO2, etc.), takes the measurement as specified, then sends back the data also as a JSON.

***INSTALLING ARDUINO IDE, TEENSYDUINO AND APPROPRIATE LIBRARIES TO CONTRIBUTE/FORK PROTOCOLS-ARDUINO***

Go to Arduino.cc and follow the directions for installing the base Arduino IDE:
http://arduino.cc/en/Main/Software.

The Photosynq device uses the Teensy 3.1, an arduino compatable microcontroller which uses an ARM processor and 2 16 bit analog inputs - both important for making the fluorescence measurements effectively.  In order to use Teensy, you need to also download Teensyduino - a software add-on to the Arduino IDE.  This is required to be able to upload programs to your Photosynq device (or any Teensy).  So go download and install it here:
http://www.pjrc.com/teensy/td_download.html
(NOTE: installing teensyduino in Linux is a little trickier - make sure to follow the directions completely!)

You should now be able to plug your Photosynq device into a computer using a standard micro USB to USB cable.  Open the Arduino IDE and go to Tools --> Board.  Select "Teensy 3.1" from the list.  

Now you're ready to go!

_Feel free to contact me at gbathree at gmail dot com with questions._
