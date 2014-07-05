// CHANGE fluorescence to fluorescence_phi2
// CHANGE dirk to ecs_dirk

/*

Most recent updates (24):
- added bluetooth reprogramming
- added light_tests_all at 100+

Most recent updates (25):{
- adding fixed offset due to detector 
- added 1009+ which allows user to save the conversion factor from tcs to actinic light
- added 99+ which copies incoming tcs value and converts it to actinic value
- added 98+ which allows user to enter a dac value for the actinic, and sets actinic dac to that value
- added calibrations for each light, so you can convert the DAC value to uE... in the long run this will be helpful so users can know for their measuring lights, actinic, etc.
  what the actual uE levels are, and they can identify when the LEDs themselves have changed.

Next to do:
- fix the rest of the calibrations... probably what's causing our f'ed up spad values and NDVI
- device ID - for some reason after you set it, it  says it's zero for manufacturing date, until you restart in which case it states correctly
- consolidate the user input function for the JSON [{}] and the user_input_dbl - they basically do the same thing, just set the cached value differently for each...
- create lookup tables for the memory eeprom saves, and for calibration values (like we do with lights [0,15,16,20] baseline [0,0,1,0]...).  It seems like the offset and other calibrations are similar enough we could make a single
way of saving them and recalling them, with the information passed to the subroutine sufficient to define it's location and name for recall.


NEXT DAC UPDATES:
- fix light tests and others which need the dac-on pulled down
- check baseline and calibrations... change values for calibration

 so it's creating an array of 1500 (or printing it at least) for spad when it runs 1 or 5 averages (doesn't matter), but it seems to work fine for fluorescence and dirk - fix!!!
 --- then work on the calibrations... get them in line...
 --- add actual calibration value (subtracted amount) tnewso the json
 --- should we make a new calibration for the LEDs themselves, based on reflectance from a known piece?  
 --- seriously clean up the debug statements
 --- add additional gain control - clean up old gain control... it's not really working write I don't think...
 Lessons learned from Mexico - 
 --- gain is chopping off too much from Fs - causing Fs to be too low
 --- update the calibration so there's more time between runs... 
 --- make sure we're able to run a fluorescence transient
 --- add bluetooth programmer into the code as a number, also add data to each unit save in EEPROM

 
 future improvements:
- make every output a JSON
- fix act_background_light
- auto gain
- make gain an array to adjust by each stuff, can we make anything else into an array?
- auto-light level for actinic
- make the calibrations more efficient (right now they both run 3 high, low, blank but that's not necessary.
 
 Experiments:
 calibration consistency - change the temperature of the device and measure the same leaf over and over... what's the repeatability?  How much change due to temperature?
 create actual PAR - find the ri 
 
 

so we're going to create a new input variable... called actinic_background and actinic_background_delay.


 */

/*  

setup - 
get light on table, get actual par meter, place in location
measure light value using light intensity meter
measure using par meter, note value
now turn off lights, place the device on the par meter, turn actinic so it moves through pwm settings
run actinic, and note when par meter value is equal to previous value
now, change light position and repeat - create calibration graph.



[{"act_background_light":13,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":3,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]},{"act_background_light":0,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":3,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]},{"act_background_light":13,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":3,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

orange and green compare
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"meas_lights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}
,{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":1,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"meas_lights":[[12,20,12,20]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}
,{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"meas_lights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[16],[16],[16],[16]],"act":[2,1,2,2]}
,{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":1,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"meas_lights":[[12,20,12,20]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[16],[16],[16],[16]],"act":[2,1,2,2]}]

[{"measurements":2,"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":1,"pulsesize":10,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"meas_lights":[[14]]}
,{"measurements":2,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1]],"averages":2,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":10,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

tcs_to_act

[{"environmental":[["relative_humidity",0],["temperature",0],["light_intensity",0],["co2",0]],"tcs_to_act":100,"protocol_name":"baseline_sample","protocols_delay":5,"act_background_light":20,"actintensity1":5,"actintensity2":5,"averages":1,"wait":0,"cal_true":2,"analog_averages":1,"pulsesize":10,"pulsedistance":3000,"calintensity":255,"pulses":[400],"detectors":[[34]],"meas_lights":[[14]]},{"tcs_to_act":100,"environmental":[["relative_humidity",0],["temperature",0],["light_intensity",0],["co2",0]],"protocols_delay":5,"act_background_light":20,"protocol_name":"fluorescence","baselines":[1,1,1,1],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":10,"pulsedistance":10000,"actintensity1":5,"actintensity2":50,"measintensity":7,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[0,1,0,0]}]

[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1],["co2",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":25,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]


just fluorescence, no env or baseline
[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":10,"pulsedistance":10000,"actintensity1":5,"actintensity2":50,"measintensity":7,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]


[{"protocol_name":"env_all","environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":15,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[1],"detectors":[[34]],"meas_lights":[[15]],"act":[2]}]



[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]


[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":100,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

act_background_light

!! FOR JESSE !!

BLANK WAIT
[{"protocol_name":"blank","averages":2,"wait":5,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":100,"pulses":[2000000],"detectors":[[34]],"meas_lights":[[13]], "act_light":[13],"act":[1]}]

NDVI PLUS FLUORESCENCE
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"meas_lights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}
,{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":1,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"meas_lights":[[12,20,12,20]]}]

FLUORESCENCE
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"meas_lights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

CALIBRATIONS
[{"protocol_name":"calibration","baselines":[0,0,0,0],"averages":3,"wait":10,"cal_true":1,"measurements_delay":10,"analog_averages":12,"pulsesize":50,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":3,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"meas_lights":[[14,15,16,20,14,15,16,20]]}
,{"protocol_name":"calibration_spad_ndvi","measurements":1,"baselines":[0,0,0,0],"measurements_delay":10,"averages":3,"wait":10,"cal_true":1,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"meas_lights":[[14,15,16,20,14,15,16,20]]}]



!! FOR JESSE 2 MEASUREMENTS !!

!! GOOD KEEP
[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"meas_lights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"meas_lights":[[14]]}]
[{"protocol_name":"calibration","baselines":[0,0,0,0],"averages":3,"wait":10,"cal_true":1,"analog_averages":12,"pulsesize":50,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":3,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"meas_lights":[[14,15,16,20,14,15,16,20]]}]

!! GOOD KEEP
[{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":10,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"meas_lights":[[12,20,12,20]]}]
[{"protocol_name":"calibration_spad_ndvi","measurements":1,"baselines":[0,0,0,0],"measurements_delay":10,"averages":3,"wait":10,"cal_true":1,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"meas_lights":[[14,15,16,20,14,15,16,20]]}]
 
!! GOOD KEEP
[{"protocol_name":"dirk","baselines":[0,0,0,0],"averages":5,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":5,"pulsedistance":3000,"actintensity1":255,"actintensity2":255,"measintensity":60,"calintensity":0,"pulses":[100,100,100],"detectors":[[35],[35],[35]],"meas_lights":[[16],[16],[16]],"act":[1,2,1]}]
[{"protocol_name":"firk","baselines":[0,0,0,0],"averages":5,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":5,"pulsedistance":3000,"actintensity1":255,"actintensity2":255,"measintensity":60,"calintensity":0,"pulses":[100,100,100],"detectors":[[35],[35],[35]],"meas_lights":[[16],[16],[16]],"act":[2,1,2]}]



 
 Basic structure:
 
 highest level is samples
 Samples contains Measurements, with user-set delays between Measurements
 Measurements contains Protocols, with user-set delays bewteen Protocols
 
 Measurements may contain any set of protocols up to 15 max, but no protocols may be duplicates
 
 
 */
/*
/////////////////////LICENSE/////////////////////
 GPLv3 license
 by Greg Austic
 If you use this code, please be nice and attribute if possible and when reasonable!
 Libraries from Adafruit were used for TMP006 and TCS34725, as were code snippets from their examples for taking the measurements.  Lots of help from PJRC forum as well.  Thanks Adafruit and Paul and PJRC community!
 
 /////////////////////DESCRIPTION//////////////////////
 Using Arduino v1.0.5 w/ Teensyduino installed downloaded from http://www.pjrc.com/teensy/td_download.html, intended for use with Teensy 3.0 or 3.1
 
 This file is used for the MultispeQ handheld spectrophotometers which are used in the PhotosynQ platform.  It is capable of taking a wide variety of measurements, including 
 fluorescence, 810 and 940 dirk, electrochromic shift, SPAD (chlorophyll content), and environmental variables like CO2, ambient temperature, contactless object temperature,
 relative humidity, light intensity, and color temperature.  It then outputs the data in a JSON format through the USB (serial) and Bluetooth (serial1).  It is designed to work
 with the PhotosynQ app and website for data storage and analysis.
 
 The structure of the file is the user selects a 3 digit number (000 - 999) through either serial communication, and that number initiates a set of variables which define the flashing
 of lights and analog reads of the detector, called a protocol  A user may input multiple codes at once (for example 005001002) and those protocols will run one after the other.  There
 are two specialized protocols, one is calibration (999) which compares the amount of infrared light produced by the measuring LEDs with that produced by a infrared LED itself.  This is
 used to improve the accuracy and reduce false IR signal in the other protocols.  The data from this protocol is saved in the EEPROM and is accessible by later protocols.  
 The other specialized protocol is 998 called baseline, which calls the calibration data and calculates the appropriate baseline specific to the current actual sample.  The baseline
 protocol should be run while the sample intended to be measured is in place, just before an actual measurement takes place.  For example, running 998001 will run the baseline, followed
 by fluorescence.  The fluorescence data will now include the baseline calculated values and, in fact, the outputed data will already be baseline corrected (ie each data point will
 have the baseline value subtracted from the measured value).  This is especially relevant with samples which produces small signals, where the interfering IR from the LEDs themselves
 can in fact account for a signifcant amount of the perceived signal.
 
 //////////////////////  TERMINOLOGY: //////////////////////  
 A 'pulse' is a single pulse of an LED, usually a measuring LED.  Pulses last for 20 - 100us
 A 'cycle' is a set of X number of pulses.  During each cycle, the user can set an actinic or other light on (not pulsing, just on throughout the cycle)
 'averages' are the number of times you repeat the same measurement BUT the values are averaged and only a single value is outputted from the device
 'measuring lights' are LEDs which have the ability to very quickly and cleanly turn on and off.
 'actinic lights' are LEDs which turn on and off slightly less quickly (but still very quickly!) but usually have better heat sinks and more capacity to be on without burning out
 and with less burn in artifacts (change in LED intensity due to the LED heating up)
 'calibratinglights' are IR LEDs which are used to calculate the baseline to improve the quality of measurements by calculating the amount of interfering IR there is from the measuring
 light
 
 //////////////////////  FEATURES AND NOTES: //////////////////////  
 For calibration, removes the first set of measured values for all measuring lights (meas1 - meas4) when calculating the 'high' and 'low' calibration values to avoid any artifacts
 When taking measurements of a sample to calculate the baseline, the measured values are not printed to the Serial ports and the output is JSON compatible
 When performing a calibration, the user is given 10 seconds to flip from shiny to dull side (always start with shiny side).  The wait time between sides can be adjusted by changing the cal_wait variable
 Protocol 000 allows the user to test the lights and detectors, one by one.
 USB power alone is not sufficient!  You must also have an additional power supply which can quickly supply power, like a 9V lithium ion battery.  Power supply may be from 5V - 24V
 Calibration is performed to create an accurate reflectance in the sample using a 940nm LED.  You can find a full explanation of the calibration at https://opendesignengine.net/documents/14
 A new calibration should be performed when sample conditions have changed (a new sample is used, the same sample is used in a different position)
 Pin A10 and A11 are 16 bit enabed with some added coding in C - the other pins cannot achieve 16 bit resolution.
 
 ////////////////////// HARDWARE NOTES //////////////////////////
 
 The detector operates with an AC filter, so only pulsing light (<100us pulses) passes through the filter.  Permanent light sources (like the sun or any other constant light) is completely
 filtered out.  So in order to measure absorbance or fluorescence a pulsing light must be used to be detectedb by the detector.  Below are notes on the speed of the measuring lights
 and actinic lights used in the MultispeQ, and the noise level of the detector:
 
 Optical:
 RISE TIME, Measuring: 0.4us
 FALL TIME, Measuring: 0.2us
 RISE TIME Actinic (bright): 2us
 FALL TIME Actinic (bright): 2us
 RISE TIME Actinic (dim): 1.5us
 FALL TIME Actinic (dim): 1.5us
 
 Electrical:
 RISE TIME, Measuring: .4us
 FALL TIME, Measuring: .2us
 RISE TIME Actinic: 1.5us
 FALL TIME Actinic: 2us
 NOISE LEVEL, detector 1: ~300ppm or ~25 detector units from peak to peak
 OVERALL: Excellent results - definitely good enough for the spectroscopic measurements we plant to make (absorbance (ECS), transmittance, fluorescence (PMF), etc.)
 
 //////////////////////  DATASHEETS ////////////////////// 
 CO2 sensor hardware http://CO2meters.com/Documentation/Datasheets/DS-S8-PSP1081.pdf
 CO2 sensor communication http://CO2meters.com/Documentation/Datasheets/DS-S8-Modbus-rev_P01_1_00.pdf
 TMP006 contactless temperature sensor information http://www.ti.com/product/tmp006
 TCS34725 light color and intensity sensor http://www.adafruit.com/datasheets/TCS34725.pdf
 HTU21D temp/rel humidity sensor http://www.meas-spec.com/downloads/HTU21D.pdf
 
 //////////////////////  I2C Addresses ////////////////////// 
 TMP006 0x42
 TCS34725 0x29
 HTU21D 0x40
 
 ////////////////////// ISSUES: ////////////////////// 
 Figure out low power mode
 Simplify the code, possibly create library
 enable user definable number of measurement pulses (maybe use vector instead of array, see programmingcplusplus library)
 a universal baseline generator - it runs every light at every intensity on every detector and creates arrays for the outputted values.  Then, if you want to subtract a baseline
 reduce # of global variables
 */

//#define DEB0UG 1  // uncomment to add full debug features
//#define DEBUGSIMPLE 1  // uncomment to add partial debug features
//#define DAC 1 // uncomment for boards which do not use DAC for light intensity control

#include <Time.h>                                                             // enable real time clock library
//#include <Wire.h>
#include <EEPROM.h>
#include <DS1307RTC.h>                                                        // a basic DS1307 library that returns time as a time_t
#include <Adafruit_Sensor.h>
#include <Adafruit_TMP006.h>
#include <Adafruit_TCS34725.h>
#include <HTU21D.h>
#include <JsonParser.h>
#include <TCS3471.h>
#include "mcp4728.h"
#include <i2c_t3.h>
#include <stdlib.h>
#include <SoftwareSerial.h>
#include <algorithm>

//////////////////////DEVICE ID FIRMWARE VERSION////////////////////////
int device_id = 0;
float manufacture_date = 0;
float firmware_version = 0;

//////////////////////PIN DEFINITIONS AND TEENSY SETTINGS////////////////////////
#define ANALOGRESOLUTION 16
#define MEASURINGLIGHT1 15      // comes installed with 505 cyan z led with a 20 ohm resistor on it.  The working range is 0 - 255 (0 is high, 255 is low)
#define MEASURINGLIGHT2 16      // comes installed with 520 green z led with a 20 ohm resistor on it.  The working range is 0 - 255 (0 is high, 255 is low)
#define MEASURINGLIGHT3 11      // comes installed with 605 amber z led with a 20 ohm resistor on it.  The working range is high 0 to low 80 (around 80 - 90), with zero off safely at 120
#define MEASURINGLIGHT4 12      // comes installed with 3.3k ohm resistor with 940 LED from osram (SF 4045).  The working range is high 0 to low 95 (80 - 100) with zer off safely at 110
#define ACTINICLIGHT1 20
#define ACTINICLIGHT2 2
#define CALIBRATINGLIGHT1 14
#define CALIBRATINGLIGHT2 10
//#define MEASURINGLIGHT_PWM 23
//#define CALIBRATINGLIGHT_PWM 9
//#define ACTINICLIGHT_INTENSITY2 3
//#define ACTINICLIGHT_INTENSITY1 4
#define ACTINICLIGHT_INTENSITY_SWITCH 5
#define DETECTOR1 34
#define DETECTOR2 35
#define SAMPLE_AND_HOLD 6
#define PWR_OFF_LIGHTS 22           // teensy shutdown pin auto power off
#define PWR_OFF 21           // teensy shutdown pin auto power off for lights power supply (TL1963)
#define BATT_LEVEL 17               // measure battery level
#define DAC_ON 23

int _meas_light;															 // measuring light to be used during the interrupt
int act_state = 1;
int act2_state = 1;
int act_int_state = 0;
int act2_int_state = 0;
int analog_reads = 0;
int number_of_protocols = 0;
int serial_buffer_size = 5000;                                        // max size of the incoming jsons
int max_jsons = 15;                                                   // max number of protocols per measurement
float baseline_array [4] = {
  0,0,0,0};                                                           // values are defined below in cal_baseline
int baseline_lights [4] = {
  0,15,16,20};
int all_pins [13] = {0,15,16,11,12,2,20,14,10,34,35,36,37};
float calibration_slope [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_yint [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_slope_factory [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_yint_factory [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
int averages = 1;
int pwr_off_state = 0;
int pwr_off_lights_state = 0;
int pwr_off_ms = 120000;                                                // number of milliseconds before unit auto powers down.

//////////////////////Shared Variables///////////////////////////
volatile int off = 0, on = 0;
int analogresolutionvalue;
int i=0,j=0, k=0,z=0,y=0,q=0,x=0,p=0;                                         // Used as a counters
unsigned long starttimer0, starttimer1, starttimer2;
int protocol_number, protocols;
IntervalTimer timer0, timer1, timer2;
int cal_pulses = 400;                                               	      // number of total pulses in the calibration routine if it is a calibration routine
int json_open = 0;
volatile long data1=0, data2=0, data3=0, data4=0, data5=0, data6=0, data7=0, data8=0, data9=0, data10;
volatile long data11=0, data12=0, data13=0, data14=0, data15=0, data16=0, data17=0, data18=0, data19=0;
int act_background_light = 13;
double light_y_intercept = 0;
double light_slope = 3;
//double teensy20_actinic_tcs = 0;
float offset_34 = 0;                                                          // create detector offset variables
float offset_35 = 0;
float slope_34 = 0;
float yintercept_34 = 0;
float slope_35 = 0;
float yintercept_35 = 0;
char* bt_response = "OKOKlinvorV1.8OKsetPINOKsetnameOK115200"; // Expected response from bt module after programming is done.

///////////////////////Calibration Variables////////////////////////// 
float baseline = 0, baseline_flag = 0;
unsigned long cal1sum_high_d1 = 0;                                           // 'high' is value with reflective surface, 'low' is value with non-reflective surface, 'blank' is value with nothing in front of detector, d1 is detector 1 and d2 is detector 2
unsigned long cal1sum_low_d1 = 0;
unsigned long cal1sum_blank_d1 = 0;
unsigned long meas1sum_high_d1 = 0;
unsigned long meas1sum_low_d1 = 0;
unsigned long meas1sum_blank_d1 = 0;
unsigned long meas2sum_high_d1 = 0;
unsigned long meas2sum_low_d1 = 0;
unsigned long meas2sum_blank_d1 = 0;
unsigned long act1sum_high_d1 = 0;
unsigned long act1sum_low_d1 = 0;
unsigned long act1sum_blank_d1 = 0;
unsigned long cal1sum_high_d2 = 0;
unsigned long cal1sum_low_d2 = 0;
unsigned long cal1sum_blank_d2 = 0;
unsigned long meas1sum_high_d2 = 0;
unsigned long meas1sum_low_d2 = 0;
unsigned long meas1sum_blank_d2 = 0;
unsigned long meas2sum_high_d2 = 0;
unsigned long meas2sum_low_d2 = 0;
unsigned long meas2sum_blank_d2 = 0;
unsigned long act1sum_high_d2 = 0;
unsigned long act1sum_low_d2 = 0;
unsigned long act1sum_blank_d2 = 0;

unsigned long cal1sum_sample = 0;

float cal1_high_d1 = 0;
float cal1_low_d1 = 0;
float cal1_blank_d1 = 0;
float meas1_high_d1 = 0;
float meas1_low_d1 = 0;
float meas1_blank_d1 = 0;
float meas2_high_d1 = 0;
float meas2_low_d1 = 0;
float meas2_blank_d1 = 0;
float act1_high_d1 = 0;
float act1_low_d1 = 0;
float act1_blank_d1 = 0;

float cal1_high_d2 = 0;
float cal1_low_d2 = 0;
float cal1_blank_d2 = 0;
float meas1_high_d2 = 0;
float meas1_low_d2 = 0;
float meas1_blank_d2 = 0;
float meas2_high_d2 = 0;
float meas2_low_d2 = 0;
float meas2_blank_d2 = 0;
float act1_high_d2 = 0;
float act1_low_d2 = 0;
float act1_blank_d2 = 0;

float cal1_sample = 0;                                                  // average value from sample used to calculate baseline
int cal_wait = 15;                                                      // wait time during calibration for users to switch the calibration piece between averages

//////////////////////HTU21D Temp/Humidity variables///////////////////////////
#define temphumid_address 0x40                                           // HTU21d Temp/hum I2C sensor address
HTU21D htu;                                                              // create class object
unsigned int tempval;
unsigned int rhval;

//////////////////////S8 CO2 Variables///////////////////////////
byte readCO2[] = {
  0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};                             //Command packet to read CO2 (see app note)
byte response[] = {
  0,0,0,0,0,0,0};                                                        //create an array to store CO2 response
float valMultiplier = 1;
int CO2calibration = 17;                                                 // manual CO2 calibration pin (CO2 sensor has auto-calibration, so manual calibration is only necessary when you don't want to wait for autocalibration to occur - see datasheet for details 
//unsigned long co2_value;
//int co2_counter = 0;                                                     // counters for co2 evolution
//int co2_count = 0;                  
//volatile int co2_flag = 0;

////////////////////TMP006 variables - address is 1000010 (adr0 on SDA, adr1 on GND)//////////////////////
Adafruit_TMP006 tmp006(0x42);  // start with a diferent i2c address!  ADR1 is GND, ADR0 is SDA so address is set to 42
float tmp006_cal_S = 6.4;

////////////////////ENVIRONMENTAL variables averages (must be global) //////////////////////
float relative_humidity_average = 0;
float temperature_average = 0;
float objt_average = 0;
float co2_value_average = 0;
float lux_average = 0;
float r_average = 0;
float g_average = 0;
float b_average = 0;

float tryit [5];

// PUSH THIS AS LOCAL VARIABLE WHEN NEW DETECTOR IS FULLY IMPLEMENTED
int detector;                                                                 // detector used in the current pulse

//////////////////////TCS34725 variables/////////////////////////////////
void i2cWrite(byte address, byte count, byte* buffer);
void i2cRead(byte address, byte count, byte* buffer);
TCS3471 TCS3471(i2cWrite, i2cRead);

//////////////////////DAC VARIABLES////////////////////////
mcp4728 dac = mcp4728(0); // instantiate mcp4728 object, Device ID = 0



void setup() {
  Serial.begin(115200);                                                         // set baud rate for Serial, used for communication to computer via USB
  //  Serial.println("Serial works");
  Serial1.begin(115200);                                                        // set baud rate for Serial 1 used for bluetooth communication on pins 0,1
  //  Serial.println("Serial1 works");
  Serial3.begin(9600);                                                          // set baud rate for Serial 3 is used to communicate with the CO2 sensor
  //  Serial.println("Serial3 works");
  delay(500);

// Wire.begin();
    Wire.begin(I2C_MASTER,0x00,I2C_PINS_18_19,I2C_PULLUP_EXT,I2C_RATE_1200);      // using alternative wire library
//  tmp006.begin();                                                              // this opens wire.begin as well, and initializes tmp006, tcs light sensor, and htu21D.  by default, contactless leaf temp measurement takes 4 seconds to complete
//  tcs.begin();
  dac.vdd(5000); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout

    TCS3471.setWaitTime(200.0);
    TCS3471.setIntegrationTime(700.0);
    TCS3471.setGain(TCS3471_GAIN_1X);
    TCS3471.enable();
        
  pinMode(MEASURINGLIGHT1, OUTPUT);                                             // set appropriate pins to output
  pinMode(MEASURINGLIGHT2, OUTPUT);
  pinMode(MEASURINGLIGHT3, OUTPUT);
  pinMode(MEASURINGLIGHT4, OUTPUT);
  pinMode(ACTINICLIGHT1, OUTPUT);
  pinMode(ACTINICLIGHT2, OUTPUT);
  pinMode(CALIBRATINGLIGHT1, OUTPUT);
  pinMode(CALIBRATINGLIGHT2, OUTPUT);
  pinMode(ACTINICLIGHT_INTENSITY_SWITCH, OUTPUT);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);                     // preset the switch to the actinic (high) preset position, DAC channel 0
  pinMode(SAMPLE_AND_HOLD, OUTPUT);
  pinMode(PWR_OFF, OUTPUT);
  pinMode(PWR_OFF_LIGHTS, OUTPUT);
  pinMode(BATT_LEVEL, INPUT); 
  digitalWriteFast(PWR_OFF, LOW);                                               // pull high to power off, pull low to keep power.
  digitalWriteFast(PWR_OFF_LIGHTS, HIGH);                                               // pull high to power on, pull low to power down.
  pinMode(13, OUTPUT);
  analogReadAveraging(1);                                                       // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u) - this gets changed later by each protocol
  pinMode(DETECTOR1, EXTERNAL);                                                 // use external reference for the detectors
  pinMode(DETECTOR2, EXTERNAL);
  analogReadRes(ANALOGRESOLUTION);                                              // set at top of file, should be 16 bit
  analogresolutionvalue = pow(2,ANALOGRESOLUTION);                              // calculate the max analogread value of the resolution setting
  analogWriteFrequency(3, 187500);                                              // Pins 3 and 5 are each on timer 0 and 1, respectively.  This will automatically convert all other pwm pins to the same frequency.
  analogWriteFrequency(5, 187500);
  pinMode(DAC_ON, OUTPUT);
  digitalWriteFast(DAC_ON, HIGH);                                               // pull high to power off, pull low to keep power.
}

int print_success() {
  Serial.println("Success you win!");
}

/*
void actinic_calibration() {
  float light_cal_high = 0;
  float ligth_cal_low = 0; 
  int light_tcs_high = 0;
  int light_tcs_low = 0;
  char w;

  Serial.print("Calibration for the actinic light compared to the TCS color sensor on top of PhotosynQ.  Please place the device, with TCS sensor facing up, on a constant, bright light source, then press any key.  Afterwards, you'll be prompted to enter the same light intensity as measured by an independent light meter.  Then you'll repeat both measurements using a lower light intensity");
  while (Serial.available == 0) {
    delay(100);
  }                                                                              // wait for key press
  
    light_tcs_high = light_intensity();
    Serial.print("TCS value has been taken.  Now please enter the calibration value from an independent light sensor followed by a # .  For example: 12.412#");
    while (Serial.peek != 35) {                                                  // waiting for a # to indicate the end of the serial stream
      if (Serial.available>0) {
        w = Serial.read();
      }
    }
    light_cal_1 = parseFloat(w);
    
  Serial.print("Now lower the light level and move the TCS sensor into place and press any key to save the TCS value");
  while (Serial.available == 0) {
    delay(100);
  }                                                                              // wait for key press
  
    light_tcs_low = light_intensity();
    Serial.print("TCS value has been taken.  Now please enter the calibration value from an independent light sensor followed by a # .  For example: 12.412#");
    while (Serial.peek != 35) {                                                  // waiting for a # to indicate the end of the serial stream
      if (Serial.available>0) {
        w = Serial.read();
      }
    }
    light_cal_1 = parseFloat(w);



    Serial.print("That's it!  Now you can test out your results by placing the TCS in front of a light source and watching (or measuring) the light output from the LEDs.  Press any key at any time to exit.  ");

  while (Serial.available == 0) {
    
}
*/

/*
so... what's rgb say? - mark rbg
then what's actual say?
choose different light level, mark both
make linear relationship and save multiplier
*/

void pwr_off() { 
    digitalWriteFast(PWR_OFF, HIGH);
    delay(50);
    digitalWriteFast(PWR_OFF, LOW);
    Serial.println("{\"pwr_off\":\"HIGH\"}");
    Serial1.println("{\"pwr_off\":\"HIGH\"}");
    Serial.println();
    Serial1.println();
}

void pwr_off_lights() {
  Serial.print("{\"pwr_off_lights\": \"");
  Serial1.print("{\"pwr_off_lights\": \"");   
  if (pwr_off_lights_state == 0) {
    Serial.println("HIGH\"}");
    Serial1.println("HIGH\"}");
    digitalWriteFast(PWR_OFF_LIGHTS, HIGH);
    pwr_off_lights_state = 1;
  }
  else if (pwr_off_lights_state == 1) {
    Serial.println("LOW\"}");
    Serial1.println("LOW\"}");
    digitalWriteFast(PWR_OFF_LIGHTS, LOW);    
    pwr_off_lights_state = 0;
  }
    Serial.println();
    Serial1.println();
}

/*

Battery output:
batt_level:[bv0,bv1,bv2]

battery data intepretation:
if bv0 - bv1 < .1 - state: "USB Power Only"
else if bv1 < 5 && bv0 > 7 - state "Low quality batteries, replace with NiMh or Li"
else if bv1 < 5 - state "Replace batteries"
else if bv0 - bv1 > .1, then:
  if bv0 < 6.6 - state "Replace batteries"
  if 6.6 - 7.1 - state "Batteries low"
  if 7.1 - 7.6 - state "Batteries OK"
  if 7.6 + - state "Batteries good"  

Please print both the intepretation and the raw values (we'll want to watch them over time to get more info out of them).

*/

void batt_level() {
  dac.analogWrite(0,4095);                                        // if we are setting actinic equal to the light sensor, then do it!
  digitalWriteFast(DAC_ON, LOW);                                               // pull high to power off, pull low to keep power.
  delay(10);
  digitalWriteFast(DAC_ON, HIGH);                                               // pull high to power off, pull low to keep power.
  float bv0 = ((float) analogRead(BATT_LEVEL) / 2358);                // read battery voltage in normal low draw state
  digitalWriteFast(ACTINICLIGHT1, HIGH);                                               // pull high to power off, pull low to keep power.
  delay(10);
  float bv1 = ((float) analogRead(BATT_LEVEL) / 2358);               // read battery voltage in start of high draw state
  delay(110);
  float bv2 = ((float) analogRead(BATT_LEVEL) / 2358);               // read battery voltage after 100ms of high draw state
  digitalWriteFast(ACTINICLIGHT1, LOW);                                               // pull high to power off, pull low to keep power.
  Serial.print("{\"batt_level\": [");
  Serial1.print("{\"batt_level\": ["); 
  Serial.print(bv0);
  Serial1.print(bv0);
  Serial.print(",");
  Serial1.print(",");
  Serial.print(bv1);
  Serial1.print(bv1);
  Serial.print(",");
  Serial1.print(",");
  Serial.print(bv2);
  Serial1.print(bv2);
  Serial.println("]}");
  Serial1.println("]}");
  Serial.println();
  Serial1.println();
}

//////////////////////// MAIN LOOP /////////////////////////
void loop() {
  
  delay(500);                                                                   // 
  int measurements = 1;                                                   // the number of times to repeat the entire measurement (all protocols)
  unsigned long measurements_delay = 0;                                    // number of milliseconds to wait between measurements  
  volatile unsigned long meas_number = 0;                                       // counter to cycle through measurement lights 1 - 4 during the run
  unsigned long end1;
  unsigned long start1 = millis(); 

  // these variables could be pulled from the JSON... however, because pulling from JSON is slow, it's better to create a int to save them into at the beginning of a protocol run and use the int instead of the raw hashTable.getLong type call 
  int _act1_light;
  int _act2_light;
  int _alt1_light;
  int _alt2_light;
  int _act1_light_prev;
  int _act2_light_prev;
  int _alt1_light_prev;
  int _alt2_light_prev;
  int _act_intensity;
  int _meas_intensity;
  int _cal_intensity;

  int cycle = 0;                                                                // current cycle number (start counting at 0!)
  int pulse = 0;                                                                // current pulse number
  int total_cycles;	                       	                        	// Total number of cycles - note first cycle is cycle 0
  int alt1_state;
  int alt2_state;
  int meas_array_size = 0;                                                      // measures the number of measurement lights in the current cycle (example: for meas_lights = [[15,15,16],[15],[16,16,20]], the meas_array_size's are [3,1,3].  
  char* json = (char*)malloc(1);
  char w;
  char* name;
  JsonHashTable hashTable;
  JsonParser<400> root;
  number_of_protocols = 0;                                                      // reset number of protocols
  int end_flag = 0;
  int which_serial = 0;
  long* data_raw_average = (long*)malloc(1);
  char serial_buffer [serial_buffer_size];
  String json2 [max_jsons];
  memset(serial_buffer,0,serial_buffer_size);                                    // reset buffer to zero
  for (i=0;i<max_jsons;i++) {
    json2[i] = "";                                                              // reset all json2 char's to zero (ie reset all protocols)
  }
  recall_all();                                                                  // recall all data saved in eeprom
  
  digitalWriteFast(SAMPLE_AND_HOLD,LOW);                                          // discharge sample and hold in case the cap has be
  delay(10);
  digitalWriteFast(SAMPLE_AND_HOLD,HIGH);

  String choose = "0";
  while (Serial.peek() != '[' && Serial1.peek() != '[') {                      // wait till we see a "[" to start the JSON of JSONS
     choose = user_enter_str(50,1);
     if (choose == '[') {
       break;
     }                                                                         // if it's a '[' then skip the other stuff and get started with measurement
     else if (choose.toInt() > 0 && choose.toInt() < 200) {                    // if it's 0 - 200 then go see full light of device testing
       lighttests(choose.toInt());
     }
     else {
       switch (choose.toInt()) {
          case 1000:
          Serial.read();
          Serial1.read();
          Serial.println("MultispeQ Ready");
          Serial1.println("MultispeQ Ready");
          break;
          case 1001:
          Serial.read();
          Serial1.read();
          pwr_off();
          break;
          case 1002:
          Serial.read();
          Serial1.read();
          configure_bluetooth();
          break;
          case 1003:
          Serial.read();
          Serial1.read();
          pwr_off_lights();
          break;
          case 1004:
          Serial.read();
          Serial1.read();
          batt_level();
          break;
          case 1005:
          Serial.read();
          Serial1.read();
          recall_all();
          print_cal_vals();
          Serial.println();
          Serial1.println();
          break;
          case 1006:
          Serial.read();
          Serial1.read();
          calibrate_light_sensor();
          break;
          case 1007:
          Serial.read();
          Serial1.read();    
          set_device_info();
          break;
          case 1008:
          Serial.read();
          Serial1.read();    
          calibrate_offset();
          break;
/*
          case 1009:
          Serial.read();
          Serial1.read();    
          calibrate_teensy20_actinic_tcs();
          break;
*/
          case 1010:
          Serial.read();
          Serial1.read();    
          call_print_calibration(0);
          break;
          case 1011:                                                                    // calibrate the lights
          Serial.read();
          Serial1.read();    
          add_calibration(0);
          break;
          case 1012:                                                                    // factory calibration
          Serial.read();
          Serial1.read();    
          add_calibration(1);
          break;

/*
    Serial.println("enter value to test, followed by '+' or enter '0+' to exit:");
    Serial.println("test all lights (requires additional input)");
    Serial.println("15 - measuring light 1 (main board)");
    Serial.println("16 - measuring light 2 (main board)");
    Serial.println("11 - measuring light 3 (add on board)");
    Serial.println("12 - measuring light 4 (add on board)");
    Serial.println("20 - actinic light 1 (main board)");
    Serial.println("2 - actinic light 2 (add on board)");
    Serial.println("14 - calibrating light 1 (main board)");
    Serial.println("10 - calibrating light 2 (add on board)");
    Serial.println("34 - detector 1 (main board)");
    Serial.println("35 - detector 2 (add on board)");
    Serial.println("101 - light detector testing (press any key to exit after entering))");
    Serial.println("102 - CO2 testing (press any key to exit after entering))");
    Serial.println("103 - temperature testing (press any key to exit after entering))");
    Serial.println("104 - relative humidity testing (press any key to exit after entering))");
    Serial.println("105 - light detector testing RAW SIGNAL (press any key to exit after entering))");

    Serial1.println("enter value to test, followed by '+' or enter '0+' to exit:");
    Serial1.println("test all lights (requires additional input)");
    Serial1.println("15 - measuring light 1 (main board)");
    Serial1.println("16 - measuring light 2 (main board)");
    Serial1.println("11 - measuring light 3 (add on board)");
    Serial1.println("12 - measuring light 4 (add on board)");
    Serial1.println("20 - actinic light 1 (main board)");
    Serial1.println("2 - actinic light 2 (add on board)");
    Serial1.println("14 - calibrating light 1 (main board)");
    Serial1.println("10 - calibrating light 2 (add on board)");
    Serial1.println("34 - detector 1 (main board)");
    Serial1.println("35 - detector 2 (add on board)");
    Serial1.println("101 - light detector testing (press any key to exit after entering))");
    Serial1.println("102 - CO2 testing (press any key to exit after entering))");
    Serial1.println("103 - temperature testing (press any key to exit after entering))");
    Serial1.println("104 - relative humidity testing (press any key to exit after entering))");
    Serial1.println("105 - light detector testing RAW SIGNAL (press any key to exit after entering))");

*/
       }
    }
  }        
  if (Serial.peek() == 91) {
#ifdef DEBUGSIMPLE
    Serial.println("comp serial");
#endif
    which_serial = 1;
  }
  else if (Serial1.peek() == 91) {
#ifdef DEBUGSIMPLE
    Serial.println("bluetooth serial");
#endif
    which_serial = 2;
  } 

  Serial.read();                                       // flush the "["
  Serial1.read();                                       // flush the "["
  
  switch (which_serial) {
    case 0:
      Serial.println("error in choosing bluetooth or serial");
      Serial1.println("error in choosing bluetooth or serial");
      break;
    
      case 1:
      Serial.setTimeout(2000);                                          // make sure set timeout is 1 second
      Serial.readBytesUntil('!',serial_buffer,serial_buffer_size);
      for (i=0;i<5000;i++) {                                    // increments through each char in incoming transmission - if it's open curly, it saves all chars until closed curly.  Any other char is ignored.
          if (serial_buffer[i] == '{') {                               // wait until you see a open curly bracket
            while (serial_buffer[i] != '}') {                          // once you see it, save incoming data to json2 until you see closed curly bracket
              json2[number_of_protocols] +=serial_buffer[i];
              i++;
            }
            json2[number_of_protocols] +=serial_buffer[i];            // catch the last closed curly
            number_of_protocols++;
          }        
      }  
      for (i=0;i<number_of_protocols;i++) {
        if (json2[i] != '0') {
#ifdef DEBUGSIMPLE
          Serial.println(json2[i]);
#endif
        }
      }
      break;

      case 2:
      Serial1.setTimeout(1000);                                          // make sure set timeout is 1 second
      Serial1.readBytesUntil('!',serial_buffer,serial_buffer_size);
      for (i=0;i<5000;i++) {                                    // increments through each char in incoming transmission - if it's open curly, it saves all chars until closed curly.  Any other char is ignored.
          if (serial_buffer[i] == '{') {                               // wait until you see a open curly bracket
            while (serial_buffer[i] != '}') {                          // once you see it, save incoming data to json2 until you see closed curly bracket
              json2[number_of_protocols] +=serial_buffer[i];
              i++;
            }
            json2[number_of_protocols] +=serial_buffer[i];            // catch the last closed curly
            number_of_protocols++;
          }        
      }  
      for (i=0;i<number_of_protocols;i++) {
        if (json2[i] != '0') {
#ifdef DEBUGSIMPLE
          Serial.println(json2[i]);
#endif
        }
      }
      break;
  }
      
  Serial1.print("{\"device_id\": ");
  Serial.print("{\"device_id\": ");
  Serial1.print(device_id);
  Serial.print(device_id);
  Serial1.print(",\"firmware_version\": \"");                                          //Begin JSON file printed to bluetooth on Serial ports
  Serial.print(",\"firmware_version\": \"");
  Serial1.print(firmware_version);
  Serial.print(firmware_version);
  Serial.print("\",\"sample\": [");
  Serial1.print("\",\"sample\": [");

  for (y=0;y<measurements;y++) {                                                      // loop through the all measurements to create a measurement group

    Serial.print("[");                                                                        // print brackets to define single measurement
    Serial1.print("[");

    for (int q = 0;q<number_of_protocols;q++) {                                               // loop through all of the protocols to create a measurement

#ifdef DEBUGSIMPLE
      Serial.println(json);
#endif

      if (baseline_flag == 1) {                           							 // calculate baseline values from saved EEPROM data only if the previous run was a sample baseline run (otherwise leave baseline == 0)
        cal_baseline();
/*
        Serial.println();
        Serial.println(baseline_array[1]);
        Serial.print(",");
        Serial.println(baseline_array[2]);
        Serial.print(",");
        Serial.println(baseline_array[3]);
        Serial.print(",");
        Serial.print(meas1_low_d1);
        Serial.print(",");
        Serial.print(cal1_sample);
        Serial.print(",");
        Serial.print(cal1_low_d1);
        Serial.print(",");
        Serial.print(cal1_high_d1);
        Serial.print(",");
        Serial.print(meas1_high_d1);
*/
      }

      free(json);                                                                        // make sure this is here! Free before resetting the size according to the serial input
      json = (char*)malloc((json2[q].length()+1)*sizeof(char));
      strcpy(json,json2[q].c_str());  
      json[json2[q].length()] = '\0';                                                    // Add closing character to char*
      hashTable = root.parseHashTable(json);
      if (!hashTable.success()) {
        Serial.println();
        Serial1.println();
        Serial.println("{\"error\":\"JSON not recognized, or other failure with Json Parser\"}, the data JSON we received is: ");
        Serial1.println("{\"error\":\"JSON not recognized, or other failure with Json Parser\"}, the data JSON we received is: ");
        for (i=0;i<5000;i++) {
          Serial.println(json2[i]);
          Serial1.println(json2[i]);
        }
        serial_bt_flush();
        return;
      }
// What do we want to be able to do?
// read an analog value on a analog pin (analogRead())
// read a digital value on a digital pin (digitalRead())
// flip a digital pin up or down (digitalWrite, 0,1)
// set a pwm value on a pwm pin (analogWrite, 0 - 4096)
// set a constant analog out (3.1 only), (analogWrite, 0 - 4096)
// send a message via I2C, SPI, or Serial and receive data back

// When do we want to do it?
// at the beginning and/or end of a protocol
// 

      int cal_true =            hashTable.getLong("cal_true");                             // identify this as a calibration routine (0 = not a calibration routine, 1 = calibration routine, 2 = create baseline for sample)
      String protocol_name =    hashTable.getString("protocol_name");                      // used only for calibration routines ("cal_true" = 1 or = 2)
      String protocol_id =      hashTable.getString("protocol_id");                      // used only for calibration routines ("cal_true" = 1 or = 2)
      averages =                hashTable.getLong("averages");                               // The number of times to average this protocol.  The spectroscopic and environmental data is averaged in the device and appears as a single measurement.
      if (averages == 0) {                                                                  // if averages don't exist, set it to 1 automatically.
        averages = 1;
      }
//      int wait =                hashTable.getLong("wait");                                    // seconds wait time between 'averages'
      measurements =            hashTable.getLong("measurements");                            // number of times to repeat a measurement, which is a set of protocols
      measurements_delay =      hashTable.getLong("measurements_delay");                      // delay between measurements in seconds
      int protocols_delay =     hashTable.getLong("protocols_delay");                         // delay between protocols within a measurement
      int averages_delay =      hashTable.getLong("averages_delay");
      int analog_averages =     hashTable.getLong("analog_averages");                          // # of measurements per measurement pulse to be internally averaged (min 1 measurement per 6us pulselengthon) - LEAVE THIS AT 1 for now
      if (analog_averages == 0) {                                                                  // if averages don't exist, set it to 1 automatically.
        analog_averages = 1;
      }
      if (hashTable.getLong("act_background_light") == 0) {                                    // The Teensy pin # to associate with the background actinic light.  This light continues to be turned on EVEN BETWEEN PROTOCOLS AND MEASUREMENTS.  It is always Teensy pin 13 by default.
//        Serial.print("background light:  ");
//        Serial.println(hashTable.getLong("act_background_light"));
//        digitalWriteFast(act_background_light,LOW);                                          // turn off actinic background from previous protocol
        act_background_light =  13;                                                             // change to new background actinic light
    }
      else {
        act_background_light =  hashTable.getLong("act_background_light");
      }
      int act_background_light_intensity = hashTable.getLong("act_background_light_intensity");  // sets intensity of background actinic.  Choose this OR tcs_to_act.
      int tcs_to_act =          hashTable.getLong("tcs_to_act");                         // sets the % of response from the tcs light sensor to act as actinic during the run (values 1 - 100).  If tcs_to_act is not defined (ie == 0), then the act_background_light intensity is set to actintensity1.
      int pulsesize =           hashTable.getLong("pulsesize");                         // Size of the measuring pulse (5 - 100us).  This also acts as gain control setting - shorter pulse, small signal. Longer pulse, larger signal.  
      int pulsedistance =       hashTable.getLong("pulsedistance");                       // distance between measuring pulses in us.  Minimum 1000 us.
      int offset_off =             hashTable.getLong("offset_off");                           // turn off detector offsets (default == 0 which is on, set == 1 to turn offsets off)
// NOTE: it takes about 50us to set a DAC channel via I2C at 2.4Mz.  
//      int actintensity1 =       hashTable.getLong("actintensity1");                         // write to input register of a DAC. channel 0 for low (actinic).  1 step = +3.69uE (271 == 1000uE, 135 == 500uE, 27 == 100uE)
//      int actintensity2 =       hashTable.getLong("actintensity2");                        // write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  (274 == 1000uE, 548 == 2000uE, 1370 == 5000uE)
//      int measintensity =       hashTable.getLong("measintensity");                        // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
//      int calintensity =        hashTable.getLong("calintensity");                        // write to input register of a DAC. channel 2 calibrating light.  0 (low) - 4095 (high).
      JsonArray pulses =        hashTable.getArray("pulses");                             // the number of measuring pulses, as an array.  For example [50,10,50] means 50 pulses, followed by 10 pulses, follwed by 50 pulses.
// NOTE: for act, act2, alt1, and alt2, you can choose a measuring light.  If you do there are no preset states.  Select 2 for off, or 1 for on.
      JsonArray act1_lights =    hashTable.getArray("act1_lights");
      JsonArray act2_lights =    hashTable.getArray("act2_lights");
      JsonArray alt1_lights =    hashTable.getArray("alt1_lights");
      JsonArray alt2_lights =    hashTable.getArray("alt2_lights");
      JsonArray act_intensities =      hashTable.getArray("act_intensities");                         // write to input register of a DAC. channel 0 for low (actinic).  1 step = +3.69uE (271 == 1000uE, 135 == 500uE, 27 == 100uE)
      JsonArray meas_intensities =     hashTable.getArray("meas_intensities");                        // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
      JsonArray cal_intensities =      hashTable.getArray("cal_intensities");                        // write to input register of a DAC. channel 2 calibrating light.  0 (low) - 4095 (high).
      JsonArray detectors =     hashTable.getArray("detectors");                           // the Teensy pin # of the detectors used during those pulses, as an array of array.  For example, if pulses = [5,2] and detectors = [[34,35],[34,35]] .  
      JsonArray meas_lights =    hashTable.getArray("meas_lights");
      JsonArray baselines =     hashTable.getArray("baselines");                           // mark which cycles you want to enable the baseline subtraction
      JsonArray environmental = hashTable.getArray("environmental");
      total_cycles =            pulses.getLength()-1;                                      // (start counting at 0!)

      free(data_raw_average);
      int size_of_data_raw = 0;
      for (i=0;i<pulses.getLength();i++) {
        size_of_data_raw += pulses.getLong(i) * meas_lights.getArray(i).getLength();
      }
      data_raw_average = (long*)calloc(size_of_data_raw,sizeof(long));                   // get some memory space for data_raw_average, initialize all at zero.

#ifdef DEBUGSIMPLE
      Serial.println();
      Serial.print("size of data raw:  ");
      Serial.println(size_of_data_raw);

      Serial.println();
      Serial.print("all data in data_raw_average:  ");
      for (i=0;i<size_of_data_raw;i++) {
        Serial.print(data_raw_average[i]);
      }
      Serial.println();

      Serial.println();
      Serial.print("number of pulses:  ");
      Serial.println(pulses.getLength());

      Serial.println();
      Serial.print("arrays in meas_lights:  ");
      Serial.println(meas_lights.getLength());

      Serial.println();
      Serial.print("length of meas_lights arrays:  ");
      for (i=0;i<meas_lights.getLength();i++) {
        Serial.print(meas_lights.getArray(i).getLength());
        Serial.print(", ");
      }
      Serial.println();
#endif

      Serial1.print("{\"protocol_name\": \"");
      Serial.print("{\"protocol_name\": \"");
      Serial1.print(protocol_name);
      Serial.print(protocol_name);
      Serial1.print("\"");
      Serial.print("\"");
      Serial.print(",\"protocol_id\": \"");                                      // print the baseline values describing the amount of IR produced by the 2 measuring lights and the actinic light.  This baseline will be subtracted from the collected data if desired
      Serial1.print(",\"protocol_id\": \"");
      Serial.print(protocol_id);
      Serial1.print(protocol_id);
      Serial.print("\",\"baseline_values\": [");                                      // print the baseline values describing the amount of IR produced by the 2 measuring lights and the actinic light.  This baseline will be subtracted from the collected data if desired
      Serial1.print("\",\"baseline_values\": [");
      Serial.print(baseline_array[1]);
      Serial1.print(baseline_array[1]);
      Serial.print(",");
      Serial1.print(",");
      Serial.print(baseline_array[2]);
      Serial1.print(baseline_array[2]);
      Serial.print(",");
      Serial1.print(",");
      Serial.print(baseline_array[3]);
      Serial1.print(baseline_array[3]);
      Serial1.print("],\"chlorophyll_spad_calibration\": [");                         // print the maximum transmission values for the 650 and 940 LEDs used for calculate chlorophyll content measurement
      Serial.print("],\"chlorophyll_spad_calibration\": [");
      Serial.print(cal1_blank_d2);                                                  // 940nm
      Serial1.print(cal1_blank_d2);
      Serial.print(",");
      Serial1.print(",");   
      Serial.print(act1_blank_d2);                                                  // 650nm
      Serial1.print(act1_blank_d2);
      Serial.print("],");                                                  // 650nm
      Serial1.print("],");

      Serial1.print("\"averages\": "); 
      Serial.print("\"averages\": "); 
      Serial1.print(averages);  
      Serial.print(averages); 
      Serial1.print(","); 
      Serial.print(","); 
      print_sensor_calibration();                                               // print sensor calibration data

      // this should be an array, so I can reset it all......
      relative_humidity_average = 0;                                                                    // reset all of the environmental variables
      temperature_average = 0;
      objt_average = 0;
      co2_value_average = 0;
      lux_average = 0;
      r_average = 0;
      g_average = 0;
      b_average = 0;
      
      calculate_offset(pulsesize);                                                                    // calculate the offset, based on the pulsesize and the calibration values (ax+b)
      
      
#ifdef DEBUGSIMPLE
      Serial.println();
      Serial.print(offset_34);
      Serial.print(",");
      Serial.println(offset_35);
#endif
      for (x=0;x<averages;x++) {                                                       // Repeat the protocol this many times     
      
        /*
    options for relative humidity, temperature, contactless temperature. light_intensity,co2
         0 - take before spectroscopy measurements
         1 - take after spectroscopy measurements
         */

        for (int i=0;i<environmental.getLength();i++) {                                         // call environmental measurements
#ifdef DEBUGSIMPLE
          Serial.println("Here's the environmental measurements called:    ");
          Serial.print(environmental.getArray(i).getString(0));
          Serial.print(", ");
          Serial.println(environmental.getArray(i).getLong(1));
#endif

          if (environmental.getArray(i).getLong(1) == 0 \                                    
          && (String) environmental.getArray(i).getString(0) == "relative_humidity") {
            Relative_Humidity((int) environmental.getArray(i).getLong(1));                        // if this string is in the JSON and the 2nd component in the array is == 0 (meaning they want this measurement taken prior to the spectroscopic measurement), then call the associated measurement (and so on for all if statements in this for loop)
            if (x == averages-1) {                                                                // if it's the last measurement to average, then print the results
              Serial1.print("\"relative_humidity\": ");
              Serial.print("\"relative_humidity\": ");
              Serial1.print(relative_humidity_average);  
              Serial1.print(",");
              Serial.print(relative_humidity_average);  
              Serial.print(",");
            }
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "temperature") {
            Temperature((int) environmental.getArray(i).getLong(1));
            if (x == averages-1) {
              Serial1.print("\"temperature\": ");
              Serial.print("\"temperature\": ");
              Serial1.print(temperature_average);  
              Serial1.print(",");
              Serial.print(temperature_average);  
              Serial.print(",");     
            }  
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "contactless_temperature") {
            Contactless_Temperature( environmental.getArray(i).getLong(1));
            if (x == averages-1) {
              Serial1.print("\"contactless_temperature\": ");
              Serial.print("\"contactless_temperature\": ");
              Serial1.print(objt_average);  
              Serial1.print(",");
              Serial.print(objt_average);  
              Serial.print(",");
            }
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "co2") {
            Co2( environmental.getArray(i).getLong(1));
            if (x == averages-1) {                                                                // if it's the last measurement to average, then print the results
              Serial1.print("\"co2\": ");
              Serial.print("\"co2\": ");
              Serial1.print(co2_value_average);  
              Serial1.print(",");
              Serial.print(co2_value_average);  
              Serial.print(",");
            }
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "light_intensity") {
            Light_Intensity(environmental.getArray(i).getLong(1));
            if (x == averages-1) {
              Serial1.print("\"light_intensity\": ");
              Serial.print("\"light_intensity\": ");
              Serial1.print(lux_to_uE(lux_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(lux_average));  
              Serial.print(",");                
              Serial1.print("\"r\": ");
              Serial.print("\"r\": ");
              Serial1.print(lux_to_uE(r_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(r_average));  
              Serial.print(",");  
              Serial1.print("\"g\": ");
              Serial.print("\"g\": ");
              Serial1.print(lux_to_uE(g_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(g_average));  
              Serial.print(",");  
              Serial1.print("\"b\": ");
              Serial.print("\"b\": ");
              Serial1.print(lux_to_uE(b_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(b_average));  
              Serial.print(",");  
            }
          }
        }

      analogReadAveraging(analog_averages);                                      // set analog averaging (ie ADC takes one signal per ~3u)
 
      int actfull = 0;
      /**/
      int _tcs_to_act = 0;
      _tcs_to_act = (uE_to_intensity(act_background_light,lux_to_uE(lux_average))*tcs_to_act)/100;
      Serial.println();
      Serial.print("tcs to act: ");
      Serial.println(_tcs_to_act);
      Serial.print("ambient light in uE: ");
      Serial.println(lux_to_uE(lux_average));
      Serial.print("ue to intensity result:  ");
      Serial.println(uE_to_intensity(act_background_light,lux_to_uE(lux_average)));

      if (tcs_to_act == 0) {                                                    // if we're not setting actinic equal to the light sensor, then preset the analogwrite to the first value in the array
        dac.analogWrite(0,act_intensities.getLong(0));                            // Set to intensity at initial position in array.  Write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
        dac.analogWrite(3,meas_intensities.getLong(0));                           // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
      }
      else if (tcs_to_act > 0) {
        if (act_background_light == 2 | act_background_light == 20) {              // if the background light is an actinic light, then change actinic intensity
          dac.analogWrite(0,_tcs_to_act);
        }
        else if (act_background_light == 15 | act_background_light == 16 | act_background_light == 11 | act_background_light == 12) {               // if the background light is a measurement light, then change measurement intensity (requires that measuring light is actinic or calibrating!!)
          delay(1000);
          Serial.println("before 1");
          dac.analogWrite(3,_tcs_to_act);
          delay(1000);
          Serial.println("after 1");
        }
      }       
      dac.analogWrite(1,1150);                                                       // This should be unused, but set it to ~5000uE (about 1150 using a far red luxeon Z
      dac.analogWrite(2,cal_intensities.getLong(0));                            // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
      delay(1);                                                                 // give time for the lights to stabilize      
      digitalWriteFast(act_background_light, HIGH);                                    // turn on the actinic background light to the low preset position and keep it on until the end of the measurement    

        for (z=0;z<size_of_data_raw;z++) {                                            // cycle through all of the pulses from all cycles
          if (cycle == 0 && pulse == 0) {                                             // if it's the beginning of a measurement, then...                                                             // wait a few milliseconds so that the actinic pulse presets can stabilize
            Serial.flush();                                                          // flush any remaining serial output info before moving forward
            starttimer0 = micros();
            timer0.begin(pulse1,pulsedistance);                                       // Begin firsts pulse
            while (micros()-starttimer0 < pulsesize) {
            }                               // wait a full pulse size, then...                                                                                          
            timer1.begin(pulse2,pulsedistance);                                       // Begin second pulse
          }      

          meas_array_size = meas_lights.getArray(cycle).getLength();
          _meas_light = meas_lights.getArray(cycle).getLong(meas_number%meas_array_size);                                    // move to next measurement light
          detector = detectors.getArray(cycle).getLong(meas_number%meas_array_size);                                        // move to next detector

#ifdef DEBUGSIMPLE
          Serial.println();
          Serial.print(", ");
          Serial.print(cycle);
          Serial.print(", ");
          Serial.print(meas_number);
          Serial.print(", ");
          Serial.print(meas_array_size);
          Serial.print(", ");
          Serial.print(_meas_light);
          Serial.print(", ");
          Serial.print(detector);
          Serial.print(", ");
          Serial.println();
#endif      

          if (pulse == 0) {                                                                // if it's the first pulse of a cycle, then change act 1 and 2, alt1 and alt2 values as per array's set at beginning of the file

          _act1_light_prev = _act1_light;                                                        // pull all new values for lights and detectors from the JSON
          _act1_light = act1_lights.getLong(cycle);
          
          _act2_light_prev = _act2_light;
          _act2_light = act2_lights.getLong(cycle);
          
          _alt1_light_prev = _alt1_light;
          _alt1_light = alt1_lights.getLong(cycle);
          
          _alt2_light_prev = _alt2_light;
          _alt2_light = alt2_lights.getLong(cycle);

          _cal_intensity = cal_intensities.getLong(cycle);                                         // pull new intensities from the JSON, replace them with tcs values when appropriate
          if (_act_intensity == -1 && (act_background_light == 2 | act_background_light == 20)) {   // if act being converted to tcs, then...
              _act_intensity = _tcs_to_act;
          }
          else {
            _act_intensity = act_intensities.getLong(cycle);
           }
          if (_meas_intensity == -1 && (act_background_light == 15 | act_background_light == 16 | act_background_light == 11 | act_background_light == 12)) {   // if meas being converted to tcs, then...                                               // if -1 is the value in act_intensities for this round then default to the tcs light intensity value
            _meas_intensity = _tcs_to_act;
            if (lux_to_uE(lux_average) < 1) {
              _tcs_to_act = 1200;                                                                // HARDCODED ugh... necessary because otherwise 0 is max output for the measurement lights
            }
          }
          else {
            _meas_intensity = meas_intensities.getLong(cycle);
          }
          delay(1000);
          Serial.println("before 2");          
          dac.analogWrite(0,_act_intensity);           
          dac.analogWrite(3,_meas_intensity);
          dac.analogWrite(2,_cal_intensity); 
          delay(1000);
          Serial.println("after 2");          
          
//#ifdef DAC
          Serial.println("actinic, measurement, and calibration intensities");
          Serial.println(_act_intensity);
          Serial.println(_meas_intensity);
          Serial.println(_cal_intensity);
//#endif
        }
          
          while (on == 0 | off == 0) {                                        	     // if ALL pulses happened, then...
          }
          data1 = analogRead(detector);                                              // save the detector reading as data1, and subtract of the baseline (if there is no baseline then baseline is automatically set = 0)     
#ifdef DEBUGSIMPLE
          Serial.print(baseline);
          Serial.print(", ");
          Serial.println(data1);
#endif

          digitalWriteFast(SAMPLE_AND_HOLD, HIGH);						// turn off sample and hold, and turn on lights for next pulse set
          
          digitalWriteFast(_act1_light_prev, LOW);                                                // turn off previous lights, turn on the new ones on (if light setting is zero, then no light on
          digitalWriteFast(_act1_light, HIGH);
          digitalWriteFast(_act2_light_prev, LOW);
          digitalWriteFast(_act2_light, HIGH);
          digitalWriteFast(_alt1_light_prev, LOW);
          digitalWriteFast(_alt1_light, HIGH);
          digitalWriteFast(_alt2_light_prev, LOW);
          digitalWriteFast(_alt2_light, HIGH);

          digitalWriteFast(DAC_ON, LOW);                                               // pull high to power off, pull low to keep power.
          delayMicroseconds(10);                                                        // give DAC time to see the latch has been pulled
          digitalWriteFast(DAC_ON, HIGH);                                               // pull high to power off, pull low to keep power.

          for (i=0;i<baselines.getLength();i++) {                                      // check the current light and see if a baseline should be applied to it.
            if (baselines.getLong(i) == 1) {
              if (baseline_lights[i] == meas_lights.getArray(cycle).getLong(meas_number%meas_array_size)) {
                baseline = baseline_array[i];
                break;
              }
            }
          }
          float offset = 0;
          if (offset_off == 0) {
            switch (detector) {                                                          // apply offset to whicever detector is being used
              case 34:
                offset = offset_34;
                break;
              case 35:
                offset = offset_35;
                break;
            }
          }
          noInterrupts();                                                              // turn off interrupts because we're checking volatile variables set in the interrupts
          on = 0;                                                                      // reset pulse counters
          off = 0;  
          pulse++;                                                                     // progress the pulse counter and measurement number counter
          data_raw_average[meas_number] += data1 - baseline - offset;  
#ifdef DEBUG
          Serial.print("$pulse reset$");
          Serial.print(total_cycles);
          Serial.print(",");  
          Serial.print(cycle);
          Serial.print(",");
          Serial.print(pulses[cycle]);
          Serial.print(",");
          Serial.print(pulse);
          Serial.print(",");
          Serial.print(on);
          Serial.print(",");
          Serial.print(off);
          Serial.print(",");
          Serial.print(micros());
          Serial.print(",");
          Serial.print(meas_number % 4);
          Serial.print(",");
#endif
#ifdef DEBUGSIMPLE
          Serial.print(meas_number);
          Serial.print(", ");
          Serial.println(data_raw_average[meas_number]);
          Serial.print("!");
          Serial1.print("!");
          Serial.print(data1); 
          Serial1.print(data1);
          Serial.print("@"); 
          Serial1.print("@");
          Serial.print(baseline); 
          Serial1.print(baseline);
#endif
          if (cal_true == 1) {                                                                                 // if this is a normal calibration run, then save the average of each separate array.  If it's a sample calibration run, then sum all into cal1sum_sample.  Also, ignore the first sets of measurements (it's usually low due to an artifact)
            if (x == 0 && protocol_name == "calibration") {                                                    // if it's the first repeat (x = 0), save as 'high' values (tin), if it's the 2nd repeat (x = 1) save as 'low' values (tape)
              switch (meas_number%meas_array_size) {
              case 0:
                cal1sum_high_d1 += data1;                                                                      // Sum all of the data points for each light together for 'high', 'low', and 'blank' for detector 1 and detector 2 respectively (used to calculate the baseline calibration value)
                break;
              case 1:
                meas1sum_high_d1 += data1;
                break;
              case 2:
                meas2sum_high_d1 += data1;
                break;
              case 3:
                act1sum_high_d1 += data1;
                break;
              case 4:
                cal1sum_high_d2 += data1;
                break;
              case 5:
                meas1sum_high_d2 += data1;
                break;
              case 6:
                meas2sum_high_d2 += data1;
                break;
              case 7:
                act1sum_high_d2 += data1;
                break;
              }
            }          
            else if (x == 1 && protocol_name == "calibration") {
              switch (meas_number%meas_array_size) {
              case 0:
                cal1sum_low_d1 += data1;                                                                        // Sum all of the data points for each light together for 'high', 'low', and 'blank' for detector 1 and detector 2 respectively (used to calculate the baseline calibration value)
                break;
              case 1:
                meas1sum_low_d1 += data1;
                break;
              case 2:
                meas2sum_low_d1 += data1;
                break;
              case 3:
                act1sum_low_d1 += data1;
                break;
              case 4:
                cal1sum_low_d2 += data1;
                break;
              case 5:
                meas1sum_low_d2 += data1;
                break;
              case 6:
                meas2sum_low_d2 += data1;
                break;
              case 7:
                act1sum_low_d2 += data1;
                break;
              }
            }
          else if (x == 2 && protocol_name == "calibration_spad_ndvi") {
              switch (meas_number%meas_array_size) {
              case 0:
                cal1sum_blank_d1 += data1;                                                                      // Sum all of the data points for each light together for 'high', 'low', and 'blank' for detector 1 and detector 2 respectively (used to calculate the baseline calibration value)
                break;
              case 1:
                meas1sum_blank_d1 += data1;
                break;
              case 2:
                meas2sum_blank_d1 += data1;
                break;
              case 3:
                act1sum_blank_d1 += data1;
                break;
              case 4:
                cal1sum_blank_d2 += data1;
                break;
              case 5:
                meas1sum_blank_d2 += data1;
                break;
              case 6:
               meas2sum_blank_d2 += data1;
                break;
              case 7:
                act1sum_blank_d2 += data1;
                break;
              }
            }
          }
          else if (cal_true == 2) {                               // If we are calculating the baseline, then sum all of the values together.  Again, ignore the first sets of measurements which often contain an artifact (abnormally high or low values)
            cal1sum_sample += data1;
          }
          interrupts();                                                              // done with volatile variables, turn interrupts back on
          meas_number++;                                                              // progress measurement number counters

          if (pulse == pulses.getLong(cycle)*meas_lights.getArray(cycle).getLength()) {                                             // if it's the last pulse of a cycle...
            pulse = 0;
            noInterrupts();
            on = 0;                                                                  // ...reset pulse counters
            off = 0;  
            interrupts();          
            cycle++;                                                                 // ...move to next cycle
#ifdef DEBUG
            Serial.print("!cycle reset!");
#endif
          }
        }        

        if (_act1_light != act_background_light) {                                  // turn off all lights unless they are the actinic background light  
          digitalWriteFast(_act1_light, LOW);
        }
        if (_act2_light != act_background_light) {
        digitalWriteFast(_act2_light, LOW);
        }
        if (_alt1_light != act_background_light) {
        digitalWriteFast(_alt1_light, LOW);
        }
        if (_alt2_light != act_background_light) {
        digitalWriteFast(_alt2_light, LOW);
        }
        
        if (act_background_light_intensity > 0) {                                  // if act_background_light_intensity is defined, then set background to it
          if (act_background_light == 2 | act_background_light == 20) {   // if act being converted to tcs, then...  
            dac.analogWrite(0,act_background_light_intensity);
          }
          else if (act_background_light == 15 | act_background_light == 16 | act_background_light == 11 | act_background_light == 12) {   // if meas being converted to tcs, then...                                               // if -1 is the value in act_intensities for this round then default to the tcs light intensity value
            delay(1000);
            Serial.println("before 3");          
            dac.analogWrite(3,act_background_light_intensity);          
            delay(1000);
            Serial.println("after 3");          
          }
          digitalWriteFast(act_background_light, HIGH);                                // turn on actinic background light in case it was off previously.
          digitalWriteFast(DAC_ON, LOW);        
          delayMicroseconds(10);
          digitalWriteFast(DAC_ON, HIGH);
        }
        else if (_tcs_to_act > 0) {                                                // if tcs_to_act is defined, then set background to it
          if (act_background_light == 2 | act_background_light == 20) {           // if act being converted to tcs, then...  
            dac.analogWrite(0,_tcs_to_act);
          }
          else if (act_background_light == 15 | act_background_light == 16 | act_background_light == 11 | act_background_light == 12) {   // if meas being converted to tcs, then...                                               // if -1 is the value in act_intensities for this round then default to the tcs light intensity value
            if (lux_to_uE(lux_average) < 1) {
              _tcs_to_act = 1200;                                                                // HARDCODED ugh... necessary because otherwise 0 is max output for the measurement lights
            }            
            delay(1000);
            Serial.println("before 4");          
            dac.analogWrite(3,_tcs_to_act);          
            delay(1000);
            Serial.println("before 4");          
            Serial.println(_tcs_to_act);
          }
          delay(1000);
          Serial.println("before 5");
          digitalWriteFast(act_background_light, HIGH);                                // turn on actinic background light in case it was off previously.
          delay(1000);
          Serial.println("after 5 before 6");
          digitalWriteFast(DAC_ON, LOW);        
          delayMicroseconds(10);
          digitalWriteFast(DAC_ON, HIGH);
          delay(1000);
          Serial.println("before 6");
#ifdef DAC
          Serial.println();
          Serial.println("_tcs_to_act is greater than zero!!");
          Serial.println(_tcs_to_act);
#endif
        }

#ifdef DEBUGSIMPLE
        Serial.print("actinic intensity now:         ");
        Serial.println(_act_intensity);
#endif      
        
        timer0.end();                                                                  // if it's the last cycle and last pulse, then... stop the timers
        timer1.end();
        cycle = 0;                                                                     // ...and reset counters
        pulse = 0;
        on = 0;
        off = 0;
        meas_number = 0;

        /*
    options for relative humidity, temperature, contactless temperature. light_intensity,co2
         0 - take before spectroscopy measurements
         1 - take after spectroscopy measurements
         */
         
        for (int i=0;i<environmental.getLength();i++) {                                             // call environmental measurements after the spectroscopic measurement
#ifdef DEBUGSIMPLE
          Serial.println("Here's the environmental measurements called:    ");
          Serial.print(environmental.getArray(i).getString(0));
          Serial.print(", ");
          Serial.println(environmental.getArray(i).getLong(1));
#endif

          if (environmental.getArray(i).getLong(1) == 1 \                                       
          && (String) environmental.getArray(i).getString(0) == "relative_humidity") {
            Relative_Humidity((int) environmental.getArray(i).getLong(1));                        // if this string is in the JSON and the 3rd component in the array is == 1 (meaning they want this measurement taken prior to the spectroscopic measurement), then call the associated measurement (and so on for all if statements in this for loop)
            if (x == averages-1) {                                                                // if it's the last measurement to average, then print the results
              Serial1.print("\"relative_humidity\": ");
              Serial.print("\"relative_humidity\": ");
              Serial1.print(relative_humidity_average);  
              Serial1.print(",");
              Serial.print(relative_humidity_average);  
              Serial.print(",");
            }
          }
          if (environmental.getArray(i).getLong(1) == 1 \
        && (String) environmental.getArray(i).getString(0) == "temperature") {
            Temperature((int) environmental.getArray(i).getLong(1));
            if (x == averages-1) {
              Serial1.print("\"temperature\": ");
              Serial.print("\"temperature\": ");
              Serial1.print(temperature_average);  
              Serial1.print(",");
              Serial.print(temperature_average);  
              Serial.print(",");     
            }       
        }
          if (environmental.getArray(i).getLong(1) == 1 \
        && (String) environmental.getArray(i).getString(0) == "contactless_temperature") {
            Contactless_Temperature( environmental.getArray(i).getLong(1));
            if (x == averages-1) {
              Serial1.print("\"contactless_temperature\": ");
              Serial.print("\"contactless_temperature\": ");
              Serial1.print(objt_average);  
              Serial1.print(",");
              Serial.print(objt_average);  
              Serial.print(",");
            }
          }          
          if (environmental.getArray(i).getLong(1) == 1 \
        && (String) environmental.getArray(i).getString(0) == "co2") {
            Co2( environmental.getArray(i).getLong(1));
            if (x == averages-1) {
              Serial1.print("\"co2\": ");
              Serial.print("\"co2\": ");
              Serial1.print(co2_value_average);  
              Serial1.print(",");
              Serial.print(co2_value_average);  
              Serial.print(",");
            }
          }
          if (environmental.getArray(i).getLong(1) == 1 \
        && (String) environmental.getArray(i).getString(0) == "light_intensity") {
            Light_Intensity( environmental.getArray(i).getLong(1));
            if (x == averages-1) {
              Serial1.print("\"light_intensity\": ");
              Serial.print("\"light_intensity\": ");
              Serial1.print(lux_to_uE(lux_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(lux_average));  
              Serial.print(",");                
              Serial1.print("\"r\": ");
              Serial.print("\"r\": ");
              Serial1.print(lux_to_uE(r_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(r_average));  
              Serial.print(",");  
              Serial1.print("\"g\": ");
              Serial.print("\"g\": ");
              Serial1.print(lux_to_uE(g_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(g_average));  
              Serial.print(",");  
              Serial1.print("\"b\": ");
              Serial.print("\"b\": ");
              Serial1.print(lux_to_uE(b_average));  
              Serial1.print(",");
              Serial.print(lux_to_uE(b_average));  
              Serial.print(",");  
            }
          }
        } 

        if (cal_true == 1 && x == 2) {                                               // if it's a calibration run and it's finished both high and low sides (ie x == 2), then print and save calibration data to eeprom
          if (protocol_name == "calibration") {
#ifdef DEBUG
          Serial.print("\"calsum_values\": [");
          Serial.print(cal1sum_high_d1);
          Serial.print(",");
          Serial.print(cal1sum_low_d1);
          Serial.print(",");
          Serial.print(cal1sum_blank_d1);
          Serial.print(",");
          Serial.print(meas1sum_high_d1);
          Serial.print(",");
          Serial.print(meas1sum_low_d1);
          Serial.print(",");
          Serial.print(meas1sum_blank_d1);
          Serial.print(",");
          Serial.print(meas2sum_high_d2);
          Serial.print(",");
          Serial.print(meas2sum_low_d2);
          Serial.print(",");
          Serial.print(meas2sum_blank_d2);
          Serial.print(",");
          Serial.print(act1sum_high_d2);
          Serial.print(",");
          Serial.print(act1sum_low_d2);
          Serial.print(",");
          Serial.print(act1sum_blank_d2);
          Serial.print("],");
#endif
          Serial.print("\"cal_values\": [");
          Serial1.print("\"cal_values\": [");
          save_eeprom(cal1sum_high_d1,0);                                                      // save values for detector 1
          save_eeprom(cal1sum_low_d1,10);
          save_eeprom(meas1sum_high_d1,30);
          save_eeprom(meas1sum_low_d1,40);
          save_eeprom(meas2sum_high_d1,60);
          save_eeprom(meas2sum_low_d1,70);
          save_eeprom(act1sum_high_d1,90);
          save_eeprom(act1sum_low_d1,100);

          save_eeprom(cal1sum_high_d2,120);                                                      // save values for detector 2
          save_eeprom(cal1sum_low_d2,130);
          save_eeprom(meas1sum_high_d2,150);
          save_eeprom(meas1sum_low_d2,160);
          save_eeprom(meas2sum_high_d2,180);
          save_eeprom(meas2sum_low_d2,190);
          save_eeprom(act1sum_high_d2,210);
          save_eeprom(act1sum_low_d2,220);
          Serial.print("0],");
          Serial1.print("0],");   
        }
        if (protocol_name == "calibration_spad_ndvi") {
          Serial.print("\"cal_values\": [");
          Serial1.print("\"cal_values\": [");
          save_eeprom(cal1sum_blank_d1,20);
          save_eeprom(meas1sum_blank_d1,50);
          save_eeprom(meas2sum_blank_d1,80);
          save_eeprom(act1sum_blank_d1,110);

          save_eeprom(cal1sum_blank_d2,140);
          save_eeprom(meas1sum_blank_d2,170);
          save_eeprom(meas2sum_blank_d2,200);
          save_eeprom(act1sum_blank_d2,230);
          Serial.print("0],");
          Serial1.print("0],");   
        }
      }

        if (cal_true == 2) {
          cal1_sample = cal1sum_sample/(cal_pulses);                                        // when converting cal1_sum to a per-measurement value, we must take into account that we ignored the first 4 values when summing to avoid the artifacts (see above)
#ifdef DEBUGSIMPLE
          Serial.println("");
          Serial.print("\"baseline_sample\": ");
          Serial.print(cal1_sample,2);
          Serial.print(",");
          Serial.print("\"baseline_values\": [");
          Serial.print(baseline_array[1]);
          Serial.print(",");
          Serial.print(baseline_array[2]);
          Serial.print(",");
          Serial.print(baseline_array[3]);
          Serial.println("]");
          Serial.println("");
#endif
        }    
        calculations();
        if (x+1 < averages) {                                            // countdown to next average, unless it's the end of the very last run
          countdown(averages_delay);
        }
      }
      // print out the results (averaged if there are more than 1 average)    
      Serial1.print("\"data_raw\":[");
      Serial.print("\"data_raw\":[");
      for (i=0;i<size_of_data_raw;i++) {                                                  // print data_raw, divided by the number of averages
        Serial.print(data_raw_average[i]/averages);
        Serial1.print(data_raw_average[i]/averages);
        if (i != size_of_data_raw-1) {
          Serial.print(",");
          Serial1.print(",");
        }
      }
      Serial1.println("]}");                                                              // close out the data_raw and protocol
      Serial.println("]}");

      if (q < number_of_protocols-1) {                                                            // if it's not the last protocol in the measurement, add a comma
        Serial.print(",");
        Serial1.print(",");
        delay(protocols_delay*1000);
      }
      else if (q == number_of_protocols-1) {                                                            // if it's not the last protocol in the measurement, add a comma

        Serial.print("]");
        Serial1.print("]");
      }      

      cal1sum_high_d1 = 0;
      cal1sum_low_d1 = 0;
      cal1sum_blank_d1 = 0;
      meas1sum_high_d1 = 0;
      meas1sum_low_d1 = 0;
      meas1sum_blank_d1 = 0;
      meas2sum_high_d2 = 0;
      meas2sum_low_d2 = 0;
      meas2sum_blank_d2 = 0;
      act1sum_high_d2 = 0;
      act1sum_low_d2 = 0;
      act1sum_blank_d2 = 0;
      cal_true = 0;                                                                 // identify this as a calibration routine (=TRUE)
      averages = 1;   			                                        // number of times to repeat the entire run 
      averages_delay = 0;                  	                                                // seconds wait time between averages
      analog_averages = 1;                                                             // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
      _act1_light = 0;
      _act2_light = 0;
      _alt1_light = 0;
      _alt2_light = 0;
      detector = 0;
      pulsesize = 0;                                                                // measured in microseconds
      pulsedistance = 0;
      _act_intensity = 0;                                                            // intensity at LOW setting below
      _meas_intensity = 0;                                                            // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
      _cal_intensity = 0;
      cal1sum_sample = 0;
      baseline = 0;
      relative_humidity_average = 0;                                                // reset all environmental variables to zero
      temperature_average = 0;
      objt_average = 0;
      co2_value_average = 0;
      lux_average = 0;
      r_average = 0;
      g_average = 0;
      b_average = 0;

      if (protocol_name != "baseline_sample") {                                    // Only reset these variables, which are used to calculate the baseline, if the previous run was NOT a baseline measurement.
        baseline_array [1] = 0;
        baseline_array [2] = 0;
        baseline_array [3] = 0;
        baseline_flag = 0;
      }
      else {                                                                    // flag to indicate we ran a baseline run first - ensures that the next run we calculate the baseline data and use it
        baseline_flag = 1;
      }
    }
    serial_bt_flush();
    
    if (y < measurements-1) {                                                      // add commas between measurements
      Serial.print(",");
      Serial1.print(",");
      delay(measurements_delay*1000);                                                   // delay between measurements
    }
  }
  Serial.println("]}");
  Serial.println("");
  Serial1.println("]}");
  Serial1.println("");
  digitalWriteFast(act_background_light, LOW);                                    // turn off the actinic background light at the end of all measurements
  act_background_light = 13;                                                      // reset background light to teensy pin 13
}

void pulse1() {		                                                        // interrupt service routine which turns the measuring light on
  digitalWriteFast(SAMPLE_AND_HOLD, LOW);		            		 // turn on measuring light and/or actinic lights etc., tick counter
  digitalWriteFast(_meas_light, HIGH);						// turn on measuring light
  data1 = analogRead(detector);                                              // save the detector reading as data1
  on=1;
#ifdef DEBUG
  Serial.print("pulse on");
#endif
}

void pulse2() {    	                                                        // interrupt service routine which turns the measuring light off									// turn off measuring light, tick counter
  digitalWriteFast(_meas_light, LOW);
  off=1;
#ifdef DEBUG
  Serial.print("pulse off");
#endif
}

void lighttests_all() {

 // enter value to increment lights (suggested value is 40) followed by +
 // or press only +0 to exit
   /*
   DAC switch numbers are:
   actinic intensity switch low is DAC 1
   actinic intensity switch high is DAC 0
   intensity for measurement is DAC 3
   intensity for calibration is DAC 2
   */

  double increment = user_enter_dbl(60000);

  Serial.print("{\"response\": \"");
  Serial1.print("{\"response\": \""); 
 
  if (increment == 0) {
  //back to main menu
    goto skipit;
  }
  Serial.print(increment);  
  Serial1.print(increment);  
  Serial.print(",");  
  Serial1.print(",");  
  Serial.read();
  Serial.read();

  digitalWriteFast(ACTINICLIGHT1,HIGH);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(0);
  Serial.println("actinic intensity 1 (switch LOW)");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(0,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }

  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("actinic intensity 1 (switch HIGH)");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(1,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(ACTINICLIGHT1,LOW);

  digitalWriteFast(ACTINICLIGHT2,HIGH);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(0);
  Serial.println("actinic intensity 2 (switch LOW)");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(0,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("actinic intensity 2 (switch HIGH)");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(1,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(ACTINICLIGHT2,LOW);

  digitalWriteFast(CALIBRATINGLIGHT1,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(2);
  Serial.println("calibrating light 1");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(2,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(CALIBRATINGLIGHT1,LOW);
  digitalWriteFast(CALIBRATINGLIGHT2,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(2);
  Serial.println("calibrating light 2");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(2,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(CALIBRATINGLIGHT2,LOW);

  digitalWriteFast(MEASURINGLIGHT1,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("measuring light 1");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(3,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(MEASURINGLIGHT1,LOW);
  digitalWriteFast(MEASURINGLIGHT2,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("measuring light 2");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(3,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(MEASURINGLIGHT2,LOW);
  
  digitalWriteFast(MEASURINGLIGHT3,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("measuring light 3");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(3,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(MEASURINGLIGHT3,LOW);
  digitalWriteFast(MEASURINGLIGHT4,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("measuring light 4");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(3,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(MEASURINGLIGHT4,LOW);
  skipit:
  serial_bt_flush();
  Serial.println("\"}");
  Serial.println("");
  Serial1.println("\"}");
  Serial1.println("");
}
  
void lighttests(int _choose) {

  float sensor_value = 0;
  digitalWriteFast(DAC_ON, LOW);                                               // leave DAC on for light tests.

    dac.analogWrite(0,4095);                                                       // write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
    dac.analogWrite(1,4095);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
    digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                         // preset the switch to the actinic (low) preset position
    dac.analogWrite(2,4095);                                                       // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
    dac.analogWrite(3,4095);                                                       // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
    digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);

    serial_bt_flush();
    if (_choose == 98) {
      recall_all();
      int tcs_to_act = 100;                                                          // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
      int _tcs_to_act = 0;                                                           // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
      double dac_setting = 0;
      while (1) {
        dac_setting = user_enter_dbl(60000);
        if (dac_setting == -1) {
          goto end;
        }
        serial_bt_flush();
        dac.analogWrite(0,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
        if (dac_setting != 0) {
          digitalWriteFast(ACTINICLIGHT1, HIGH);
        }
        else {
          digitalWriteFast(ACTINICLIGHT1, LOW);
        }
      }
      end:
      serial_bt_flush();
      digitalWriteFast(ACTINICLIGHT1, LOW);
    }
    
    if (_choose == 99) {
      recall_all();
      int tcs_to_act = 100;                                                          // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
      int _tcs_to_act = 0;                                                          // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
      Serial.print("{\"act_to_tcs\":[");
      Serial1.print("{\"act_to_tcs\":[");
      dac.analogWrite(0,0);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      digitalWriteFast(ACTINICLIGHT1, HIGH);
      while (Serial.available()<1 && Serial1.available()<1) {
        sensor_value = lux_to_uE(Light_Intensity(1));
        _tcs_to_act = (lux_to_uE(sensor_value)*tcs_to_act)/(3.69*100);         // save the value from the tcs lights sensor as a velue between 0 - 4095 for the saturating light 1      
#ifdef DEBUGSIMPLE
        Serial.print(sensor_value);
        Serial1.print(sensor_value);
        Serial.print(",");
        Serial1.print(",");
        Serial.print(lux_to_uE(sensor_value));
        Serial1.print(lux_to_uE(sensor_value));
        Serial.print(",");
        Serial1.print(",");
        Serial.print(tcs_to_act);
        Serial1.print(tcs_to_act);
//        Serial.print(",");
//        Serial1.print(",");
//        Serial.print(teensy20_actinic_tcs);
//        Serial1.print(teensy20_actinic_tcs);
        Serial.println();
        Serial1.println();
#endif
        dac.analogWrite(0,_tcs_to_act);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
        Serial.print(sensor_value);
        Serial1.print(sensor_value);
        Serial.print(",");
        Serial1.print(",");
        Serial.print(_tcs_to_act);
        Serial1.print(_tcs_to_act);
        Serial.println(",");
        Serial1.println(",");        
        delay(1000);
      }
        serial_bt_flush();
        digitalWriteFast(ACTINICLIGHT1, LOW);      
    }
    serial_bt_flush(); 
    if (_choose == 100) {
      lighttests_all();
      }
    serial_bt_flush(); 
    if (_choose == 101) {
      Serial.print("{\"light_intensity\":[");
      Serial1.print("{\"light_intensity\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        sensor_value = lux_to_uE(Light_Intensity(1));
        Serial.print(sensor_value);
        Serial1.print(sensor_value);
        Serial.print(",");
        Serial1.print(",");
        delay(2000);
      }
    serial_bt_flush();
    }
    else if (_choose == 102) {                      // co2
      Serial.print("{\"Co2_value\":[");
      Serial1.print("{\"Co2_value\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        sensor_value = Co2(1);
        Serial.print(sensor_value);
        Serial.print(",");
        Serial1.print(sensor_value);
        Serial1.print(",");
        delay(2000);
      }
    serial_bt_flush();
    }
    else if (_choose == 103) {                      // temperature
      Serial.print("{\"temperature\":[");
      Serial1.print("{\"temperature\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        sensor_value = Temperature(1);
        Serial.print(sensor_value);
        Serial.print(",");
        Serial1.print(sensor_value);
        Serial1.print(",");
        delay(2000);
      }
    serial_bt_flush();
    }
    else if (_choose == 104) {                      //relative humidity
      Serial.print("{\"relative_humidity\":[");
      Serial1.print("{\"relative_humidity\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        sensor_value = Relative_Humidity(1);
        Serial.print(sensor_value);
        Serial.print(",");
        Serial1.print(sensor_value);
        Serial1.print(",");
        delay(2000);
      }
    serial_bt_flush();
    }
    else if (_choose == 105) {
      Serial.print("{\"light_intensity\":[");
      Serial1.print("{\"light_intensity\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        sensor_value = Light_Intensity(1);
        Serial.print(sensor_value);
        Serial1.print(sensor_value);
        Serial.print(",");
        Serial1.print(",");
        delay(2000);
      }
    serial_bt_flush();
    }
    else if (_choose == 106) {
      Serial1.print("{\"contactless_temperature\": [");
      Serial.print("{\"contactless_temperature\": [");
      while (Serial.available()<1 && Serial1.available()<1) {
        sensor_value = Contactless_Temperature(1);
        Serial.print(sensor_value);
        Serial1.print(sensor_value);
        Serial.print(",");
        Serial1.print(",");
        delay(200);
      }
    serial_bt_flush();
    }
    
    if (_choose<34 && _choose>0) {
      Serial1.print("{[");
      Serial.print("{[");
      int dac_setting = 0;
      while (1) {
        dac_setting = user_enter_dbl(60000);
        if (dac_setting == -1) {
          goto last;
        }
        serial_bt_flush();
        dac.analogWrite(0,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
        dac.analogWrite(1,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
        dac.analogWrite(2,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
        dac.analogWrite(3,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
        digitalWriteFast(_choose, HIGH);      
        if (dac_setting != 0) {
          digitalWriteFast(_choose, HIGH);      
        }
        else {
          digitalWriteFast(_choose, LOW);      
        }
      }
      last:
      serial_bt_flush();
      digitalWriteFast(_choose, LOW);      
    }
    else if (_choose>33 && _choose <38) {
      int reading = 0;
      int pulsedistance = 0;
      while (1) {
        Serial.println();
        Serial1.println();
        pulsedistance = user_enter_dbl(60000);
        if (pulsedistance == -1) {
          goto final;
        }
        digitalWriteFast(SAMPLE_AND_HOLD, LOW);
        delayMicroseconds(pulsedistance);
        reading = analogRead(_choose);
        digitalWriteFast(SAMPLE_AND_HOLD, HIGH);
        Serial.print(reading);
        Serial1.print(reading);
        Serial.print(",");
        Serial1.print(",");
      }
    final:
    serial_bt_flush();
    }
  digitalWriteFast(DAC_ON, HIGH);                                               // turn DAC back off
  Serial.print(sensor_value);
  Serial1.print(sensor_value);
  Serial.println("]}");
  Serial1.println("]}");
  Serial.println();
  Serial1.println();
}

float Relative_Humidity(int var1) {
  if (var1 == 1 | var1 == 0) {
    float relative_humidity = htu.readHumidity();
#ifdef DEBUGSIMPLE
    Serial1.print("\"relative_humidity\": ");
    Serial.print("\"relative_humidity\": ");
    Serial1.print(relative_humidity);  
    Serial1.print(",");
    Serial.print(relative_humidity);  
    Serial.print(",");
#endif
    relative_humidity_average += relative_humidity / averages;
    return relative_humidity;
  }
}

float Temperature(int var1) {
  if (var1 == 1 | var1 == 0) {
    float temperature = htu.readTemperature();
#ifdef DEBUGSIMPLE
    Serial1.print("\"temperature\": ");
    Serial.print("\"temperature\": ");
    Serial1.print(temperature);  
    Serial1.print(",");
    Serial.print(temperature);  
    Serial.print(",");
#endif
    temperature_average += temperature / averages;
    return temperature;
  }
}

int Light_Intensity(int var1) {
  if (var1 == 1 | var1 == 0 | var1 == 3) {
    word lux, r, g, b;
	lux = TCS3471.readCData();                  // take 3 measurements, outputs in format - = 65535 or whatever 16 bits is.
	r = TCS3471.readRData();
	g = TCS3471.readGData();
	b = TCS3471.readBData();
#ifdef DEBUGSIMPLE
    Serial1.print("\"light_intensity\": ");
    Serial1.print(lux, DEC);
    Serial1.print(",");
    Serial1.print("\"red\": ");
    Serial1.print(r, DEC);
    Serial1.print(",");
    Serial1.print("\"green\": ");
    Serial1.print(g, DEC);
    Serial1.print(",");
    Serial1.print("\"blue\": ");
    Serial1.print(b, DEC);
    Serial1.print(",");
    //  Serial1.print("\cyan\": ");
    //  Serial1.print(c, DEC);
    Serial.print("\"light_intensity\": ");
    Serial.print(lux, DEC);
    Serial.print(",");
    Serial.print("\"red\": ");
    Serial.print(r, DEC);
    Serial.print(",");
    Serial.print("\"green\": ");
    Serial.print(g, DEC);
    Serial.print(",");
    Serial.print("\"blue\": ");
    Serial.print(b, DEC);
    Serial.print(",");
    //  Serial.print("\cyan\": ");
    //  Serial.print(c, DEC);
#endif
    if (var1 == 1 | var1 == 0) {
      lux_average += lux / averages;
      r_average += r / averages;
      g_average += g / averages;
      b_average += b / averages;
    }
    return lux;
  }
}

void save_eeprom_dbl(double saved_val, int loc) {                                                        // save the calibration value to the EEPROM, and print the value to USB and bluetooth Serial
  char str [10] = {0};
  dtostrf(saved_val,10,10,str);
  for (i=0;i<10;i++) {
    EEPROM.write(loc+i,str[i]);
  }
}

float call_eeprom_dbl(int loc) {                                                                       // call calibration values from the EEPROM, and print the values to USB Serial only
  char str [10] = {0};
  float called_val;
  for (i=0;i<10;i++) {
    str[i] = EEPROM.read(loc+i);
  }
  String string = (String) str;
  called_val = atof(string.c_str());  
  return called_val;
}  

void print_sensor_calibration() {
  Serial.print("\"light_slope\":");
  Serial.print(light_slope);
  Serial.print(",");
  Serial.print("\"light_y_intercept\":");
  Serial.print(light_y_intercept);
  Serial.print(",");
}

String user_enter_str(long timeout,int _pwr_off) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  char serial_buffer [32] = {0};
  String serial_string;
  serial_bt_flush();
  long start1 = millis();
  long end1 = millis();
#ifdef DEBUG
  Serial.println();
  Serial.print(Serial.available());
  Serial.print(",");
  Serial.println(Serial1.available());
#endif  
  while (Serial.available() == 0 && Serial1.available() == 0) {
    if (_pwr_off == 1) {
      end1 = millis();
      if ((end1 - start1) > pwr_off_ms) {
        pwr_off();
        goto skip;
      }
    }
  }
  skip:
    if (Serial.available()>0) {                                                          // if it's from USB, then read it.
      if (Serial.peek() != '[') {
        Serial.readBytesUntil('+',serial_buffer, 32);
        serial_string = serial_buffer;
        serial_bt_flush();
      }
      else {}
    }
    else if (Serial1.available()>0) {                                                    // if it's from bluetooth, then read it instead.
      if (Serial1.peek() != '[') {
        Serial1.readBytesUntil('+',serial_buffer, 32);
        serial_string = serial_buffer;
        serial_bt_flush();
      }
      else {}
    }
  return serial_string;
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

long user_enter_long(long timeout) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  long val;  
  char serial_buffer [32] = {0};
  String serial_string;
  long start1 = millis();
  long end1 = millis();
  while (Serial.available() == 0 && Serial1.available() == 0) {
    end1 = millis();
    if ((end1 - start1) > timeout) {
      goto skip;
    }
  }                        // wait until some info comes in over USB or bluetooth...
  skip:
   if (Serial1.available()>0) {                                                    // if it's from bluetooth, then read it instead.
    Serial1.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atol(serial_string.c_str());  
  }
  else if (Serial.available()>0) {                                                          // if it's from USB, then read it.
    Serial.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atol(serial_string.c_str());  
  }
  return val;
  while (Serial.available()>0) {                                                     // flush any remaining serial data before returning
    Serial.read();
  }
  while (Serial1.available()>0) {
    Serial1.read();
  }
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

double user_enter_dbl(long timeout) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  double val;
  char serial_buffer [32] = {0};
  String serial_string;
  long start1 = millis();
  long end1 = millis();
  while (Serial.available() == 0 && Serial1.available() == 0) {
    end1 = millis();
    if ((end1 - start1) > timeout) {
      goto skip;
    }
  }                        // wait until some info comes in over USB or bluetooth...
  skip:
  if (Serial.available()>0) {                                                          // if it's from USB, then read it.
    Serial.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atof(serial_string.c_str());  
  }
  else if (Serial1.available()>0) {                                                    // if it's from bluetooth, then read it instead.
    Serial1.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atof(serial_string.c_str());  
  }
  return val;
  while (Serial.available()>0) {                                                     // flush any remaining serial data before returning
    Serial.read();
  }
  while (Serial1.available()>0) {
    Serial1.read();
  }
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

float lux_to_uE(float _lux_average) {                                                      // convert the raw signal value to uE, based on a calibration curve
//  int uE = (_lux_average-996.668)/30.64773;           // units 1,2,6 with lamp
//  int uE = (_lux_average-1739.21)/35.84464;           // units 3 with lamp
//  int uE = (_lux_average-187.4785)/15.03928;           // units 5  with lamp
//  int uE = (_lux_average-.35)/.02135;           // units 8 and 4 in chambers
//  int uE = (_lux_average-.3692)/.0408;           // units 7 in chambers
//  int uE = (_lux_average+65)/6.30;           // units 8 and 4 outside
//  int uE = (_lux_average+250.35)/15.796;           // units 7 outside
//  int uE = (_lux_average-10.16)/2.527;           // units 12 outside
//  int uE = (_lux_average-52.43)/4.837;           // units 13 outside
  int uE = (_lux_average-light_y_intercept)/light_slope;           // units 13 outside
#ifdef DEBUGSIMPLE
  Serial.print(_lux_average);
  Serial.print(",");
  Serial.print(light_y_intercept);
  Serial.print(",");
  Serial.print(light_slope);
  Serial.print(",");
  Serial.println(uE);
#endif
  return uE;
}

/*
so how about:
uE_to_intensity(pin, uE) // this takes a pin and a uE (user entered) and outputs that value.  So user can output a specific light level.
change lux_to_uE to -->
tcs_to_intensity(pin, _lux_average) // this takes a _lux value, converts to uE based on the calibration for the specified pin.
int all_pins [13] = {0,15,16,11,12,2,20,14,10,34,35,36,37};
float calibration_slope [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_yint [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_slope_factory [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_yint_factory [13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};  
*/

int uE_to_intensity(int _pin, int _uE) {
  float _slope = 0;
  float _yint = 0;
  unsigned int _intensity = 0;
  for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // loop through all_pins
    if (all_pins[i] == _pin) {                                                                        // when you find the pin your looking for
      _slope = calibration_slope[i];                                                                  // go get the calibration slope and yintercept
      _yint = calibration_yint[i];
      break;
    }
  }
  _intensity = (_uE-_yint)/_slope;                                                                    // calculate the resulting intensity DAC value
//#ifdef DEBUGSIMPLE
  Serial.println(_uE);
  Serial.print(_uE);
  Serial.print(",");
  Serial.print(_yint);
  Serial.print(",");
  Serial.print(_slope);
  Serial.print(",");
  Serial.print("intensity setting DAC: ");
  Serial.println(_intensity);
//#endif
  return _intensity;
}

void save_calibration_yint (int _pin,float _yint_val,int _factory) {
  for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // loop through all_pins
    if (all_pins[i] == _pin) {                                                                        // when you find the pin your looking for
      if (_factory == 0) {
        save_eeprom_dbl(_yint_val,550+i*10);                                                                   // then in the same index location in the calibrations array, save the inputted value.      
      }
      else if (_factory == 1) {
        save_eeprom_dbl(_yint_val,950+i*10);                                                                   // then in the same index location in the calibrations array, save the inputted value.
      }
    }
  }
}

void save_calibration_slope (int _pin,float _slope_val,int _factory) {
  for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // loop through all_pins
    if (all_pins[i] == _pin) {                                                                        // when you find the pin your looking for
      if (_factory == 0) {
        save_eeprom_dbl(_slope_val,350+i*10);                                                                   // then in the same index location in the calibrations array, save the inputted value.
      }
      else if (_factory == 1) {
        save_eeprom_dbl(_slope_val,750+i*10);                                                                   // then in the same index location in the calibrations array, save the inputted value.
      }
    }
  }
}

void call_print_calibration (int no_print) {
  for (int i=0;i<(sizeof(all_pins)/sizeof(int));i++) {                                                      // recall the calibration arrays
    calibration_slope [i] = call_eeprom_dbl(350+i*10);
    calibration_yint [i] = call_eeprom_dbl(550+i*10);
    calibration_slope_factory [i] = call_eeprom_dbl(750+i*10);
    calibration_yint_factory [i] = call_eeprom_dbl(950+i*10);
  }
  if (no_print == 0) {
    Serial.print("\"all_pins\":[");
    for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // loop through all_pins and print
      Serial.print(all_pins[i]);
      if (i != sizeof(all_pins)/sizeof(int)-1) {        
        Serial.print(",");    
      }
    }
    Serial.println("]},");
    Serial.print("\"calibration_slope\":[");
    for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // recall the calibration arrays
      Serial.print(calibration_slope[i]);
      if (i != sizeof(all_pins)/sizeof(int)-1) {        
        Serial.print(",");    
      }
    }
    Serial.println("]},");
    Serial.print("\"calibration_yint\":[");
    for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // recall the calibration arrays
      Serial.print(calibration_yint[i]);
      if (i != sizeof(all_pins)/sizeof(int)-1) {        
        Serial.print(",");    
      }
    }
    Serial.println("]},");
    Serial.print("\"calibration_slope_factory\":[");
    for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // recall the calibration arrays
      Serial.print(calibration_slope_factory[i]);
      if (i != sizeof(all_pins)/sizeof(int)-1) {        
        Serial.print(",");    
      }
    }
    Serial.println("]}");
    Serial.print("\"calibration_yint_factory\":[");
    for (i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // recall the calibration arrays
      Serial.print(calibration_yint_factory[i]);
      if (i != sizeof(all_pins)/sizeof(int)-1) {        
        Serial.print(",");    
      }
    }
    Serial.println("]}");
    Serial.println("");
  }
}

void add_calibration (int _factory) {                                                                 // here you can save one of the calibration values.  This may be a regular calibration, or factory calibration (which saves both factory and regular values)
  call_print_calibration(0);                                                                            // call and print calibration info from eeprom
  int pin = 0;
  int location = 0;
  float slope_val = 0;
  float yint_val = 0;
  while (1) {
    pin = user_enter_dbl(60000);                                                                      // define the pin to add a calibration value to
    Serial.println(pin);
    if (pin == -1) {                                                                                    // if user enters -1, exit from this calibration
      goto final;
    }
    slope_val = user_enter_dbl(60000);                                                                  // now enter that calibration value
    Serial.println(slope_val,4);
    if (slope_val == -1) {
      goto final;
    }
    yint_val = user_enter_dbl(60000);                                                                  // now enter that calibration value
    Serial.println(yint_val,4);
    if (yint_val == -1) {
      goto final;
    }                                                                                            // THIS IS STUPID... not sure why but for some reason you have to split up the eeprom saves or they just won't work... at leats this works...
    save_calibration_yint(pin,yint_val,0);                                                      // save the value in the calibration string which corresponding pin index in all_pins
    save_calibration_slope(pin,slope_val,0);                                                      // save the value in the calibration string which corresponding pin index in all_pins
    if (_factory == 1) {      
      save_calibration_yint(pin,yint_val,1);                                                      // save the value in the calibration string which corresponding pin index in all_pins
      save_calibration_slope(pin,slope_val,1);                                                      // save the value in the calibration string which corresponding pin index in all_pins
    }
    skipit:
  delay(1);
  }
  final:
  serial_bt_flush();
  call_print_calibration(0);
}

void calculate_offset(int _pin, float _x, float _yint) {
}





void calculate_offset(int _pulsesize) {                                                                    // calculate the offset, based on the pulsesize and the calibration values offset = a*'pulsesize'+b
  offset_34 = slope_34*_pulsesize+yintercept_34;
  offset_35 = slope_35*_pulsesize+yintercept_35;
}

void print_offset(int _open) {
  if (_open == 0) {
    Serial.print("{");  
    Serial1.print("{");  
  }
  Serial.print("\"slope_34\":");
  Serial.print(slope_34);
  Serial.print(",");
  Serial.print("\"yintercept_34\":");
  Serial.print(yintercept_34);
  Serial.print(",");
  Serial.print("\"slope_35\":");
  Serial.print(slope_35);
  Serial.print(",");
  Serial.print("\"yintercept_35\":");
  Serial.print(yintercept_35);

  Serial1.print("\"slope_34\":");
  Serial1.print(slope_34);
  Serial1.print(",");
  Serial1.print("\"yintercept_34\":");
  Serial1.print(yintercept_34);
  Serial1.print(",");
  Serial1.print("\"slope_35\":");
  Serial1.print(slope_35);
  Serial1.print(",");
  Serial1.print("\"yintercept_35\":");
  Serial1.print(yintercept_35);
  if (_open == 1) {
    Serial.print(",");
    Serial1.print(",");
  }
  else {
    Serial.print("}");
    Serial1.print("}");
  }
}

void calibrate_offset() {

  recall_all();                                                        // recall saved offset values
  print_offset(0);                                                          // display them as JSON

  int pass = user_enter_dbl(1000);
  if (pass == 1) {                                                      // user enters the slope for detector 34, then y intercept for 34, then slope for 35, then y intercept for 35
    Serial.print("{\"output\":\"");
    Serial1.print("{\"output\":\"");
    float slope_34 = user_enter_dbl(60000);  
    Serial.print(slope_34,4);
    Serial1.print(slope_34,4);
    save_eeprom_dbl(slope_34,300);                                                 
    Serial.print("success!");
    Serial1.print("success!");
    serial_bt_flush();

    float yintercept_34 = user_enter_dbl(60000);  
    Serial.print(yintercept_34,4);
    Serial1.print(yintercept_34,4);
    save_eeprom_dbl(yintercept_34,310);                                                 
    Serial.print("success!");
    Serial1.print("success!");
    serial_bt_flush();

    float slope_35 = user_enter_dbl(60000);  
    Serial.print(slope_35,4);
    Serial1.print(slope_35,4);
    save_eeprom_dbl(slope_35,320);                                                 
    Serial.print("success");
    Serial1.print("success");
    serial_bt_flush();

    float yintercept_35 = user_enter_dbl(60000);  
    Serial.print(yintercept_35,4);
    Serial1.print(yintercept_35,4);
    save_eeprom_dbl(yintercept_35,330);                                                 
    Serial.print("success\"},");
    Serial1.print("success\"},");
    serial_bt_flush();
    
    recall_all();                                                        // recall saved offset values
    print_offset(0);                                                          // display them as JSON  
    Serial.println();
    Serial1.println();
  }
}

/*
void print_teensy20_actinic_tcs() {
  Serial.print("\"teensy20_actinic_tcs\":");
  Serial.print(teensy20_actinic_tcs);
  Serial.print(",");
}

void calibrate_teensy20_actinic_tcs() {

  recall_all();                                                        // recall saved offset values
  Serial.print("{");
  Serial1.print("{");
  print_teensy20_actinic_tcs();                                                          // display them as JSON
  Serial.print("\"");
  Serial1.print("\"");
  
  int pass = user_enter_dbl(1000);
  if (pass == 1) {
    float teensy20_actinic_tcs = user_enter_dbl(60000);  
    Serial.print(teensy20_actinic_tcs,4);
    Serial1.print(teensy20_actinic_tcs,4);
    save_eeprom_dbl(teensy20_actinic_tcs,340);                                                 
    Serial.print("success\",");
    Serial1.print("success\",");
    serial_bt_flush();
    recall_all();                                                                          // recall saved offset values
    print_teensy20_actinic_tcs();                                                          // display them as JSON
  }
    Serial.println("0}");
    Serial1.println("0}");
    Serial.println();
    Serial1.println();
}
*/

void calibrate_light_sensor() {

  recall_all();
  
  Serial.print("{\"sensor_calibration\": [");
  Serial1.print("{\"sensor_calibration\": [");
  Serial.print(light_slope,3);
  Serial1.print(",");
  Serial1.print(light_slope,3);
  Serial1.print(",");
  Serial.print(light_y_intercept,3);
  Serial.print(",");
  Serial1.print(light_y_intercept,3);
  Serial.print("]}");
  Serial1.print("]}");

  int pass = user_enter_dbl(1000);
  if (pass == 1) {
  
  // Welcome to the tcs34715 light sensor calibration
  // First manually check the light intensity of the device against a known PAR light meter, under a light similar to that you'll be using when taking samples (probably the sun)
  // From this, you should get a linear relationship between the two, in the form tcs_intensity = cal_intensity*a + b where tcs_intensity is the intensity as measured by the MultispeQ and cal_intensity is that measured by the other light meter
  // Please enter the 'a' value of that equation, followed by '+'
  
    double _light_slope = user_enter_dbl(60000);  
    Serial.print(_light_slope,6);
    Serial1.print(_light_slope,6);
    save_eeprom_dbl(_light_slope,250);                                                 
    Serial.print("success");
    Serial1.print("success");
    serial_bt_flush();

    double _light_y_intercept = user_enter_dbl(60000);  
    Serial.print(_light_y_intercept,6);
    Serial1.print(_light_y_intercept,6);
    save_eeprom_dbl(_light_y_intercept,260);                                                 
    Serial.print("success\",");
    Serial1.print("success\",");
    serial_bt_flush();
  
    recall_all();
    print_sensor_calibration();
  
    Serial.println("0}");                  // close out JSON
    Serial1.println("0}");
    Serial.println();
    Serial1.println();
}
}

float Contactless_Temperature(int var1) {
  if (var1 == 1 | var1 == 0) {
    float objt = readObjTempC_mod();
    float diet = tmp006.readDieTempC();
    delay(4000); // 4 seconds per reading for 16 samples per reading but shortened to make faster samples
#ifdef AVERAGES
    Serial.print("\"object_temperature\": ");
    Serial.print(objt);
    Serial.print(",");
    Serial1.print("\"object_temperature\": "); 
    Serial1.print(objt);
    Serial1.print(",");    
    Serial.print("\"board_temperature\": "); 
    Serial.print(diet);
    Serial.print(",");
    Serial1.print("\"board_temperature\": "); 
    Serial1.print(diet);
    Serial1.print(",");
#endif
    objt_average += objt / averages;
    return objt;
  }
  else if (var1 == 3) {
    Serial.println("Place a piece of black electrical tape, or other high emissivity item in the leaf clip and permanently clamp the clip with a rubber band.  Place the entire unit in a cooler out of the sun for 15 minutes to let the temeprature come to equilibrium in the space.  Once finished, press any key and the calibration will continue.  It will take anywhere between 30 seconds and 10 minutes, depending how far out of calibration you were initially");
    int tmp006_up = 0;
    int tmp006_down = 0;
    float tmp006_walk = 2;
    while (Serial.available()<1) {
    }                                         // wait for button press
    while (tmp006_walk > .05 && tmp006_up < 100 && tmp006_down < 100) {     // if it's walked 100 steps in any direction or if it's zoned in on the value (ie walk is <.1) then save the final value to EEPROM
      float objt = Contactless_Temperature(1);
      delay(10);
      float temperature = Temperature(1);
      if (objt<temperature) {                                        // if the temperature using contactless temp sensor (tmp006) is less than the on-board temperature (htu21d)
        tmp006_cal_S -= tmp006_walk;                                  // walk the calibration value down slightly
        tmp006_up++;
      }
      else if (objt>temperature) {                                    // if the temperature using contactless temp sensor (tmp006) is less than the on-board temperature (htu21d)
        tmp006_cal_S += tmp006_walk;                                   // walk the calibration value down slightly
        tmp006_down++;
      }
      Serial.print(tmp006_cal_S);
      Serial.print(",");
      Serial.print(tmp006_walk);
      Serial.print(",");
      Serial.print(tmp006_up);
      Serial.print(",");
      Serial.print(tmp006_down);
      Serial.println();
      delay(10);
      if (tmp006_up>2 && tmp006_down>2) {                             // if it's walked back and forth a few times, then make the walk steps smaller (therefore more accurate)
        tmp006_up = 0;
        tmp006_down = 0;
        tmp006_walk /= 5;
      }
    }
    save_eeprom(tmp006_cal_S,240);
    Serial.println("Finished calibration - new values saved!");
  }
}

unsigned long Co2(int var1) {
  if (var1 == 1 | var1 == 0) {
    requestCo2(readCO2,2000);
    unsigned long co2_value = getCo2(response);
#ifdef DEBUGSIMPLE
    Serial1.print("\"co2_content\": ");
    Serial.print("\"co2_content\": ");
    Serial1.print(co2_value);  
    Serial1.print(",");
    Serial.print(co2_value);  
    Serial.print(",");
#endif
    delay(100);
    co2_value_average += co2_value / averages;
    return co2_value;
  }
  else if (var1 == 3) {
    Serial.print("place detector in fresh air (not in house or building) for 30 seconds, then press any button. Make sure sensor environment is steady and calm!");
    Serial1.print("place detector in fresh air (not in house or building) for 30 seconds, then press any button.  Make sure sensor environment is steady and calm!");
    while (Serial1.available()<1 && Serial.available()<1) {
    }
    Serial.print("please wait about 6 seconds");
    Serial1.print("please wait about 6 seconds");
    digitalWriteFast(CO2calibration, HIGH);
    delay(6000);
    digitalWriteFast(CO2calibration, LOW);
    Serial.print("place detector in 0% CO2 air for 30 seconds, then press any button.  Make sure sensor environment is steady and calm!");
    Serial1.print("place detector in 0% CO2 air for 30 seconds, then press any button.  Make sure sensor environment is steady and calm!");
    while (Serial1.available()<2 && Serial.available()<2) {
    }
    Serial.print("please wait about 20 seconds");
    Serial1.print("please wait about 20 seconds");
    digitalWriteFast(CO2calibration, HIGH);
    delay(20000);
    digitalWriteFast(CO2calibration, LOW);
    Serial.print("calibration complete!");
    Serial1.print("calibration complete!");
  }
  else if (var1 == 4) {
    requestCo2(readCO2,2000);
    unsigned long co2_value = getCo2(response);
    Serial1.print("[{\"co2_content\": ");
    Serial.print("\"co2_content\": ");
    Serial1.print(co2_value);  
    Serial1.print("}]");
    Serial.print(co2_value);  
    Serial.print("}]");
    return co2_value;
    delay(2000);
  } 
}

void requestCo2(byte packet[],int timeout) {
  int error = 0;
  long start1 = millis();
  long end1 = millis();
  while(!Serial3.available()) { //keep sending request until we start to get a response
    Serial3.write(readCO2,7);
    delay(50);
    end1 = millis();
    if (end1 - start1 > timeout) {
      Serial.print("\"no co2 sensor found\"");
      Serial1.print("\"no co2 sensor found\"");
      Serial.print(",");
      Serial1.print(",");
      int error = 1;
      break;
    }
  }
  switch (error) {
    case 0:
    while(Serial3.available() < 7 ) //Wait to get a 7 byte response
    {
      timeout++;  
      if(timeout > 10) {   //if it takes to long there was probably an error
  #ifdef DEBUGSIMPLE
        Serial.println("I timed out!");
  #endif
        while(Serial3.available())  //flush whatever we have
            Serial3.read();
        break;                        //exit and try again
      }
      delay(50);
    }
    for (int i=0; i < 7; i++) {
      response[i] = Serial3.read();    
  #ifdef DEBUGSIMPLE
      Serial.println("I got it!");
  #endif
    }  
    break;
    case 1:
    break;
  }
}

unsigned long getCo2(byte packet[]) {
  int high = packet[3];                        //high byte for value is 4th byte in packet in the packet
  int low = packet[4];                         //low byte for value is 5th byte in the packet
  unsigned long val = high*256 + low;                //Combine high byte and low byte with this formula to get value
  return val* valMultiplier;
}

int numDigits(int number) {
  int digits = 0;
  if (number < 0) digits = 1; // remove this line if '-' counts as a digit
  while (number) {
    number /= 10;
    digits++;
  }
  return digits;
}

void save_eeprom(int saved_val, int loc) {                                                        // save the calibration value to the EEPROM, and print the value to USB and bluetooth Serial
  char str [10];
  itoa(saved_val, str, 10);
  for (i=0;i<10;i++) {
    EEPROM.write(loc+i,str[i]);
    char temp = EEPROM.read(loc+i);
  }
  Serial.print(((float) atoi(str) / ((cal_pulses-1))));                                          // when converting called_val to a per-measurement value, we have to divide by 1 less than cal_pulses/4 because we skipped the first set of values to avoid artifacts
  Serial1.print(((float) atoi(str) / ((cal_pulses-1))));                                          // when converting called_val to a per-measurement value, we have to divide by 1 less than cal_pulses/4 because we skipped the first set of values to avoid artifacts
  Serial.print(",");
  Serial1.print(",");
}

float call_eeprom(int loc) {                                                                       // call calibration values from the EEPROM, and print the values to USB Serial only
  char temp [10];
  float called_val;
  for (i=0;i<10;i++) {
    temp[i] = EEPROM.read(loc+i);
  }
  called_val = ((float) atoi(temp) / ((cal_pulses-1)));                                          // when converting called_val to a per-measurement value, we have to divide by 1 less than cal_pulses/4 because we skipped the first set of values to avoid artifacts
#ifdef DEBUGSIMPLE
  Serial.print(called_val);
  Serial.print(", ");
#endif
  return called_val;
}  

void recall_all() {                                                                  // call all baseline calibration data for spectroscopic methods
#ifdef DEBUGSIMPLE
  Serial.print("Calibration values called from EEPROM: HIGH, DETECTOR 1: ");
#endif
  cal1_high_d1 = call_eeprom(0);                                           
  meas1_high_d1 = call_eeprom(30);
  meas2_high_d1 = call_eeprom(60);
  act1_high_d1 = call_eeprom(90);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: LOW, DETECTOR 1: ");
#endif  
  cal1_low_d1 = call_eeprom(10);                                    
  meas1_low_d1 = call_eeprom(40);
  meas2_low_d1 = call_eeprom(70);
  act1_low_d1 = call_eeprom(100);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: BLANK, DETECTOR 1: ");
#endif
  cal1_blank_d1 = call_eeprom(20);
  meas1_blank_d1 = call_eeprom(50);                            
  meas2_blank_d1 = call_eeprom(80);
  act1_blank_d1 = call_eeprom(110);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: HIGH, DETECTOR 2: ");
#endif
  cal1_high_d2 = call_eeprom(120);                         
  meas1_high_d2 = call_eeprom(150);
  meas2_high_d2 = call_eeprom(180);
  act1_high_d2 = call_eeprom(210);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: LOW, DETECTOR 2: ");
#endif
  cal1_low_d2 = call_eeprom(130);               
  meas1_low_d2 = call_eeprom(160);
  meas2_low_d2 = call_eeprom(190);
  act1_low_d2 = call_eeprom(220);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: BLANK, DETECTOR 2: ");
#endif
  cal1_blank_d2 = call_eeprom(140);      
  meas1_blank_d2 = call_eeprom(170);
  meas2_blank_d2 = call_eeprom(200);
  act1_blank_d2 = call_eeprom(230);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Offset values for detector 1(34) and detector 2 (35)");
#endif
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("call uE per 12 bit division (4096) on LED on Teensy pin 20 used for conversion of ambient light to actinic");
#endif  
  tmp006_cal_S = call_eeprom_dbl(240);
  light_slope = call_eeprom_dbl(250);                                                 
  light_y_intercept = call_eeprom_dbl(260);  
  device_id = call_eeprom_dbl(270);
  firmware_version = call_eeprom_dbl(280);
  manufacture_date = call_eeprom_dbl(290);
  slope_34 = call_eeprom_dbl(300);                                                                      // call saved calibration inforamtion for detector offsets pin 34 (A10) infrared and 35 (A11) visible
  yintercept_34 = call_eeprom_dbl(310);
  slope_35 = call_eeprom_dbl(320);
  yintercept_35 = call_eeprom_dbl(330);
  call_print_calibration(1);                                                                       // recall all light calibration data converting lights to uE
}

void cal_baseline() {
  baseline_array [1] = (meas1_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(meas1_high_d1-meas1_low_d1));        // baseline for light 15 (measurement light)
  baseline_array [2] = (meas2_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(meas2_high_d1-meas2_low_d1));        // baseline for light 16 (measurement light)
  baseline_array [3] = (act1_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(act1_high_d1-act1_low_d1));           // baseline for light 20 (actinic)
}

int countdown(int _wait) {                                                                       // count down the number of seconds defined by wait
  for (z=0;z<_wait;z++) {
#ifdef DEBUG
    Serial.print(_wait);
    Serial.print(",");
    Serial.print(z);
#endif
#ifdef DEBUGSIMPLE
    Serial.print("Time remaining: ");
    Serial.print(_wait-z);
    Serial.print(", ");
    Serial.println(Serial.available());    
#endif
    delay(1000);
    if (Serial.peek() == 49) {                                                                      // THIS ISN'T IMPLEMENTED - can't really skip for some reason the incomign serial buffer keeps filling up
#ifdef DEBUG
      Serial.println("Ok - skipping wait!");
#endif
      delay(5);                                                                                    // if multiple buttons were pressed, make sure they all get into the serial cache...
      z = _wait;
      while (Serial.available()>0) {                                                                   //flush the buffer in case multiple buttons were pressed
        Serial.read();
      }
    }
  }
}

void calculations() {
}

float getFloatFromSerialMonitor(){                                               // getfloat function from Gabrielle Miller on cerebralmeltdown.com - thanks!
  char inData[20];  
  float f=0;    
  int x=0;  
  while (x<1){  
    String str;   
    if (Serial.available()) {
      delay(100);
      int i=0;
      while (Serial.available() > 0) {
        char  inByte = Serial.read();
        str=str+inByte;
        inData[i]=inByte;
        i+=1;
        x=2;
      }
      f = atof(inData);
      memset(inData, 0, sizeof(inData));  
    }
  }//END WHILE X<1  
  return f; 
}

double readObjTempC_mod(void) {
  double Tdie = tmp006.readRawDieTemperature();
  double Vobj = tmp006.readRawVoltage();
  Vobj *= 156.25;  // 156.25 nV per LSB
  Vobj /= 1000; // nV -> uV
  Vobj /= 1000; // uV -> mV
  Vobj /= 1000; // mV -> V
  Tdie *= 0.03125; // convert to celsius
  Tdie += 273.15; // convert to kelvin

#ifdef DEBUG
  Serial.print("Vobj = "); 
  Serial.print(Vobj * 1000000); 
  Serial.println("uV");
  Serial.print("Tdie = "); 
  Serial.print(Tdie); 
  Serial.println(" C");
  Serial.print("tmp006 calibration value: ");
  Serial.println(tmp006_cal_S);
#endif

  double tdie_tref = Tdie - TMP006_TREF;
  double S = (1 + TMP006_A1*tdie_tref + 
    TMP006_A2*tdie_tref*tdie_tref);
  S *= tmp006_cal_S;
  S /= 10000000;
  S /= 10000000;

  double Vos = TMP006_B0 + TMP006_B1*tdie_tref + 
    TMP006_B2*tdie_tref*tdie_tref;

  double fVobj = (Vobj - Vos) + TMP006_C2*(Vobj-Vos)*(Vobj-Vos);

  double Tobj = sqrt(sqrt(Tdie * Tdie * Tdie * Tdie + fVobj/S));

  Tobj -= 273.15; // Kelvin -> *C
  return Tobj;
}

void i2cWrite(byte address, byte count, byte* buffer)
{
    Wire.beginTransmission(address);
    byte i;
    for (i = 0; i < count; i++)
    {
#if ARDUINO >= 100
        Wire.write(buffer[i]);
#else
        Wire.send(buffer[i]);
#endif
    }
    Wire.endTransmission();
}

void i2cRead(byte address, byte count, byte* buffer)
{
    Wire.requestFrom(address, count);
    byte i;
    for (i = 0; i < count; i++)
    {
#if ARDUINO >= 100
        buffer[i] = Wire.read();
#else
        buffer[i] = Wire.receive();
#endif
    }
}

void print_cal_vals() {
          Serial.print("{\"cal_values\": [");
          Serial.print(cal1sum_high_d1);                                                      // save values for detector 1
          Serial.print(",");
          Serial.print(cal1sum_low_d1);
          Serial.print(",");
          Serial.print(meas1sum_high_d1);
          Serial.print(",");
          Serial.print(meas1sum_low_d1);
          Serial.print(",");
          Serial.print(meas2sum_high_d1);
          Serial.print(",");
          Serial.print(meas2sum_low_d1);
          Serial.print(",");
          Serial.print(act1sum_high_d1);
          Serial.print(",");
          Serial.print(act1sum_low_d1);
          Serial.print(",");

          Serial.print(cal1sum_high_d2);                                                      // save values for detector 2
          Serial.print(",");
          Serial.print(cal1sum_low_d2);
          Serial.print(",");
          Serial.print(meas1sum_high_d2);
          Serial.print(",");
          Serial.print(meas1sum_low_d2);
          Serial.print(",");
          Serial.print(meas2sum_high_d2);
          Serial.print(",");
          Serial.print(meas2sum_low_d2);
          Serial.print(",");
          Serial.print(act1sum_high_d2);
          Serial.print(",");
          Serial.print(act1sum_low_d2);
          Serial.println("],");

          Serial1.print("{\"cal_values\": [");
          Serial1.print(cal1sum_high_d1);                                                      // save values for detector 1
          Serial1.print(",");
          Serial1.print(cal1sum_low_d1);
          Serial1.print(",");
          Serial1.print(meas1sum_high_d1);
          Serial1.print(",");
          Serial1.print(meas1sum_low_d1);
          Serial1.print(",");
          Serial1.print(meas2sum_high_d1);
          Serial1.print(",");
          Serial1.print(meas2sum_low_d1);
          Serial1.print(",");
          Serial1.print(act1sum_high_d1);
          Serial1.print(",");
          Serial1.print(act1sum_low_d1);
          Serial1.print(",");

          Serial1.print(cal1sum_high_d2);                                                      // save values for detector 2
          Serial1.print(",");
          Serial1.print(cal1sum_low_d2);
          Serial1.print(",");
          Serial1.print(meas1sum_high_d2);
          Serial1.print(",");
          Serial1.print(meas1sum_low_d2);
          Serial1.print(",");
          Serial1.print(meas2sum_high_d2);
          Serial1.print(",");
          Serial1.print(meas2sum_low_d2);
          Serial1.print(",");
          Serial1.print(act1sum_high_d2);
          Serial1.print(",");
          Serial1.print(act1sum_low_d2);
          Serial1.println("],");

 
          Serial.print("\"cal_values_spad\": [");
          Serial.print(cal1sum_blank_d1);
          Serial.print(",");
          Serial.print(meas1sum_blank_d1);
          Serial.print(",");
          Serial.print(meas2sum_blank_d1);
          Serial.print(",");
          Serial.print(act1sum_blank_d1);
          Serial.print(",");
          Serial.print(cal1sum_blank_d2);
          Serial.print(",");
          Serial.print(meas1sum_blank_d2);
          Serial.print(",");
          Serial.print(meas2sum_blank_d2);
          Serial.print(",");
          Serial.print(act1sum_blank_d2);
          Serial.println("]}");
          Serial.println();

          Serial1.print("\"cal_valuesspad\": [");
          Serial1.print(cal1sum_blank_d1);
          Serial1.print(",");
          Serial1.print(meas1sum_blank_d1);
          Serial1.print(",");
          Serial1.print(meas2sum_blank_d1);
          Serial1.print(",");
          Serial1.print(act1sum_blank_d1);
          Serial1.print(",");
          Serial1.print(cal1sum_blank_d2);
          Serial1.print(",");
          Serial1.print(meas1sum_blank_d2);
          Serial1.print(",");
          Serial1.print(meas2sum_blank_d2);
          Serial1.print(",");
          Serial1.print(act1sum_blank_d2);
          Serial1.println("]}");
          Serial1.println();
}

void set_device_info() {
  serial_bt_flush();
  Serial.print("{\"device_id\": ");
  Serial.print(device_id);
  Serial.println(",");
  Serial.print("\"firmware_version\": ");
  Serial.print(firmware_version);
  Serial.println(",");
  Serial.print("\"manufacture_date\": ");
  Serial.print(manufacture_date);
  Serial.println("}");
  Serial.println();
  
  Serial1.print("{\"device_id\": ");
  Serial1.print(device_id);
  Serial1.println(",");
  Serial1.print("\"firmware_version\": ");
  Serial1.print(firmware_version);
  Serial1.println(",");
  Serial1.print("\"manufacture_date\": ");
  Serial1.print(manufacture_date);
  Serial1.println("}");
  Serial1.println();
  
  int pass = user_enter_dbl(1000);
  if (pass == 1) {
    Serial.print("\"device_info\":\"[");  
    Serial1.print("\"device_info\":\"[");  
    Serial.read();
    Serial1.read();
// please enter new device ID (integers only) followed by '+'
    device_id = user_enter_dbl(60000);
    save_eeprom_dbl((int) device_id,270);
    Serial.print(device_id);
    Serial1.print(device_id);
    Serial.print(",");
    Serial1.print(",");
// please enter new firmware version (int or double) followed by '+'
    firmware_version = user_enter_dbl(60000);
    save_eeprom_dbl(firmware_version,280);
    Serial.print(firmware_version,2);
    Serial1.print(firmware_version,2);
    Serial.print(",");
    Serial1.print(",");
/**/
// please enter new date of manufacture (yyyymm) followed by '+'   
    manufacture_date = user_enter_dbl(60000);
    save_eeprom_dbl(manufacture_date,290);
    Serial.print(manufacture_date,1);
    Serial1.print(manufacture_date,1);
    Serial.print(",");
    Serial1.print(",");

/*
    manufacture_date = user_enter_long(60000);
    Serial.println("made it!");
    save_eeprom_dbl(manufacture_date,290);
    Serial.print(manufacture_date);
    Serial1.print(manufacture_date);
*/
    Serial.println("]}");
    Serial1.println("]}");
    Serial.println();
    Serial1.println();
  }
  Serial.read();
  Serial1.read();
}

void serial_bt_flush() {
  while (Serial.available()>0) {                                                   // Flush incoming serial of the failed command
    Serial.read();
  }
  while (Serial1.available()>0) {                                                   // Flush incoming serial of the failed command
    Serial1.read();
  }
}

void configure_bluetooth () {

// Bluetooth Programming Sketch for Arduino v1.2
// By: Ryan Hunt <admin@nayr.net>
// License: CC-BY-SA
//
// Standalone Bluetooth Programer for setting up inexpnecive bluetooth modules running linvor firmware.
// This Sketch expects a bt device to be plugged in upon start. 
// You can open Serial Monitor to watch the progress or wait for the LED to blink rapidly to signal programing is complete.
// If programming fails it will enter command where you can try to do it manually through the Arduino Serial Monitor.
// When programming is complete it will send a test message across the line, you can see the message by pairing and connecting
// with a terminal application. (screen for linux/osx, hyperterm for windows)
//
// Hookup bt-RX to PIN 11, bt-TX to PIN 10, 5v and GND to your bluetooth module.
//
// Defaults are for OpenPilot Use, For more information visit: http://wiki.openpilot.org/display/Doc/Serial+Bluetooth+Telemetry

  Serial.print("{\"response\": \"");
  Serial1.print("{\"response\": \"");
// Enter bluetooth device name as seen by other bluetooth devices (20 char max), followed by '+'.
  String name = user_enter_str(60000,0);  
// Enter current bluetooth device baud rate, followed by '+' (if new jy-mcu,it's probably 9600, if it's already had firmware installed by 115200)
  long baud_now = user_enter_dbl(60000);  
// PLEASE NOTE - the pairing key has been set automatically to '1234'.

  int pin =         1234;                    // Pairing Code for Module, 4 digits only.. (0000-9999)
  int led =         13;                      // Pin of Blinking LED, default should be fine.
  char* testMsg =   "PhotosynQ changing bluetooth name!!"; //
  int x;
  int wait =        1000;                    // How long to wait between commands (1s), dont change this.
  
  pinMode(led, OUTPUT);
  Serial.begin(115200);                      // Speed of Debug Console

// Configuring bluetooth module for use with PhotosynQ, please wait.
  Serial1.begin(baud_now);                        // Speed of your bluetooth module, 9600 is default from factory.
  digitalWrite(led, HIGH);                 // Turn on LED to signal programming has started
  delay(wait);
  Serial1.print("AT");
  delay(wait);
  Serial1.print("AT+VERSION");
  delay(wait);
  Serial.print("Setting PIN : ");          // Set PIN
  Serial.println(pin);
  Serial1.print("AT+PIN"); 
  Serial1.print(pin); 
  delay(wait);
  Serial.print("Setting NAME: ");          // Set NAME
  Serial.println(name);
  Serial1.print("AT+NAME");
  Serial1.print(name); 
  delay(wait);
  Serial.println("Setting BAUD: 115200");   // Set baudrate to 115200
  Serial1.print("AT+BAUD8");                   
  delay(wait);

  if (verifyresults()) {                   // Check configuration
    Serial.println("Configuration verified");
  } 
  digitalWrite(led, LOW);                 // Turn off LED to show failure.
  Serial.println("\"}");                  // close out JSON
  Serial1.println("\"}");
  Serial.println();
  Serial1.println();
}

int verifyresults() {                      // This function grabs the response from the bt Module and compares it for validity.
  int makeSerialStringPosition;
  int inByte;
  char serialReadString[50];

  inByte = Serial1.read();
  makeSerialStringPosition=0;
  if (inByte > 0) {                                                // If we see data (inByte > 0)
    delay(100);                                                    // Allow serial data time to collect 
    while (makeSerialStringPosition < 38){                         // Stop reading once the string should be gathered (37 chars long)
      serialReadString[makeSerialStringPosition] = inByte;         // Save the data in a character array
      makeSerialStringPosition++;                                  // Increment position in array
      inByte = Serial1.read();                                          // Read next byte
    }
    serialReadString[38] = (char) 0;                               // Null the last character
    if(strncmp(serialReadString,bt_response,37) == 0) {               // Compare results
      return(1);                                                    // Results Match, return true..
    }
    Serial.print("VERIFICATION FAILED!!!, EXPECTED: ");           // Debug Messages
    Serial.println(bt_response);
    Serial.print("VERIFICATION FAILED!!!, RETURNED: ");           // Debug Messages
    Serial.println(serialReadString);
    return(0);                                                    // Results FAILED, return false..
  } 
  else { // In case we haven't received anything back from module
    Serial.println("VERIFICATION FAILED!!!, No answer from the bt module ");        // Debug Messages
    Serial.println("Check your connections and/or baud rate");
    return(0);                                                                      // Results FAILED, return false..
  }
}

