Protocols-Arduino
=================

Current Protocol File: Arduino\_Combined\_03
(other files are just for testing and messing around)

This repository contains the arduino protocols used on the Teensy 3.0 in the Photosynq handheld measurement device.

Currently, there is only a single arduino .ino file which contains all of the callable protocols.  Protocols are called by sending a 3 serial bytes '000' - '999' over bluetooth to the Teensy.  The Teensy is listening for a 3 digit number when on.  Each different number initiates a different protocol and the data is returned from the Teensy via the bluetooth Serial ( '000' calls a calibration routine, '001' returns temperature, relative humidity, and CO2 values, etc.).

The information from the Photosynq device is organized as a JSON.  When received by the app, it adds time and GPS data to the JSON, then the app passes the JSON along to the website where it stored in a database.

FOR EXAMPLE: 1) the photosynq app pairs the smartphone (via bluetooth) with a photosynq device, 2) send the device '001' over bluetooth serial, 3) device runs the subroutine '001' as defined in the Arduino code, 4) device sends back the data in the form of a JSON, 5) App displays the data, adds GPS and time stamp, and ships JSON off to the website where it is stored. 

INSTALLING ARDUINO IDE, TEENSYDUINO AND APPROPRIATE LIBRARIES TO CONTRIBUTE/FORK PROTOCOLS-ARDUINO

Go to Arduino.cc and follow the directions for installing the base Arduino IDE:
http://arduino.cc/en/Main/Software.

The Photosynq device uses the Teensy 3.0, an arduino compatable microcontroller which uses an ARM processor and 2 16 bit analog inputs - both important for making the fluorescence measurements effectively.  In order to use Teensy, you need to also download Teensyduino - a software add-on to the Arduino IDE.  This is required to be able to upload programs to your Photosynq device (or any Teensy).  So go download and install it here:
http://www.pjrc.com/teensy/td_download.html
(NOTE: installing teensyduino in Linux is a little trickier - make sure to follow the directions completely!)

Currently, the main Photosynq program does not require any additional libraries.  However, we may add sleep.h in the future for low power modes, so you may want to go get that and put it in your 'libraries' folder wherever you installed the Arduino IDE.

You should now be able to plug your Photosynq device into a computer using a standard micro USB to USB cable.  Open the Arduino IDE and go to Tools --> Board.  Select "Teensy 3.0" from the list.  

Now you're ready to go!  If you want to create a new protocol, you can write your code in a separate program to work out the kinks, then add it to the Current Protocol File above with an associated 3 byte call code (a number not currently defined between '000' - '999').   

Feel free to contact me at gbathree at gmail dot com with questions.
