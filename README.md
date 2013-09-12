Protocols-Arduino
=================

***Current Protocol File: Arduino\_Combined\_03***

(other files are just for testing and messing around)

This repository contains the arduino protocols used to program the Teensy 3.0 'brain' of the Photosynq handheld measurement device.

Currently, there is only a single arduino .ino file which contains all of the callable protocols.  Protocols are called by sending a 3 serial bytes '000' - '999' over bluetooth to the Teensy.  The Teensy is listening for a 3 digit number when on.  Each different number initiates a different protocol and the data is returned from the Teensy via the bluetooth Serial ( '000' calls a calibration routine, '001' returns temperature, relative humidity, and CO2 values, etc.).

The information from the Photosynq device is organized as a JSON.  When received by the app, it adds time and GPS data to the JSON, then the app passes the JSON along to the website where it stored in a database.

A JSON is an easy, cross-platform way to share data and it looks like this (in case you don't know, I didn't):

{"subject": "apple", "raw": [337,336,348,351,348,363,370,363,362,363,357,369,373,368,382,389,365,365,361,387,361,379,383
,366,374,464,496,523,535,544,542,545,544,538,549,554,567,557,563,567,566,579,561,573,557,558,576,572,565,574,566,517,518
,515,509,497,492,472,492,485,468,467,461,464,461,462,466,456,471,467,442,470,455,464,465,468,471,461,462,467,473,440,466
,471,466,447,460,452,459,452,469,459,473,454,447,467,445,444,455,453], "Photosynthetic\_efficiency\_(Phi(II))": 
0.397,"Relative\_chlorophyll\_content": 335.09, "ProjectID": "Fruit - Alive or Dead?",  "UserID": "gbathree", "Temperature":33.89,"Relative\_Humidity": 32.43,"CO2\_content": 1050}

If you want to create a new protocol and you are adding data to the JSON in your code (that's the only way to get the data from your app from the device to the web so you probably will want to), you can check to make sure you didn't screw it up using a JSON validator, like this: http://jsonlint.com/ .  

Also, if you're creating a new protocol I'd suggest that you write your code in a separate program to work out the kinks, then add it to the Current Protocol File above with an associated 3 byte call code (a number not currently defined between '000' - '999').  The Current Protocol File is big and messy, it would be easier that way.

***FOR EXAMPLE***

1) the photosynq app pairs the smartphone (via bluetooth) with a photosynq device, 2) send the device '001' over bluetooth serial, 3) device runs the subroutine '001' as defined in the Arduino code, 4) device sends back the data in the form of a JSON, 5) App displays the data, adds GPS and time stamp, and ships JSON off to the website where it is stored. 

***INSTALLING ARDUINO IDE, TEENSYDUINO AND APPROPRIATE LIBRARIES TO CONTRIBUTE/FORK PROTOCOLS-ARDUINO***

Go to Arduino.cc and follow the directions for installing the base Arduino IDE:
http://arduino.cc/en/Main/Software.

The Photosynq device uses the Teensy 3.0, an arduino compatable microcontroller which uses an ARM processor and 2 16 bit analog inputs - both important for making the fluorescence measurements effectively.  In order to use Teensy, you need to also download Teensyduino - a software add-on to the Arduino IDE.  This is required to be able to upload programs to your Photosynq device (or any Teensy).  So go download and install it here:
http://www.pjrc.com/teensy/td_download.html
(NOTE: installing teensyduino in Linux is a little trickier - make sure to follow the directions completely!)

Currently, the main Photosynq program does not require any additional libraries.  However, we may add sleep.h in the future for low power modes, so you may want to go get that and put it in your 'libraries' folder wherever you installed the Arduino IDE.

You should now be able to plug your Photosynq device into a computer using a standard micro USB to USB cable.  Open the Arduino IDE and go to Tools --> Board.  Select "Teensy 3.0" from the list.  

Now you're ready to go!

_Feel free to contact me at gbathree at gmail dot com with questions._
