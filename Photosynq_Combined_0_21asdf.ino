// CHANGE fluorescence to fluorescence_phi2
// CHANGE dirk to ecs_dirk

/*

NEXT DAC UPDATES:
- make all 

 so it's creating an array of 1500 (or printing it at least) for spad when it runs 1 or 5 averages (doesn't matter), but it seems to work fine for fluorescence and dirk - fix!!!
 --- then work on the calibrations... get them in line...
 --- add actual calibration value (subtracted amount) to the json
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
 
 
 - changed json structure to {..."sample":[[{...},{...}...],[{...}...]...]}, moved common variables to the part before "sample" except the calibration values
 - changed 'repeats' to 'averages' so 'averages' are all automatically internally averaged before being sent off to the app.  If averages == 1, no data is saved on the device (that means there's no limit to the number of data points).- now there is no limit to the number of measurement lights in an array (previously limited to 4)
 - all environmental variables are also averaged within a protocol, and can be entered using the 'environmental':[...] token in the incoming JSON.  This allows environmental variables to be taken at the beginning or end of the measurement
 - All information relating to the spectroscopic measurements can now be transferred to the device through a JSON
 - calibration only works on samples with the same pulse sizes (gain) and light intensities!!!
 - an additional calibration for ndvi/spad has been added.  Both calibrations have 3 phases (reflective, dull, blank) and it's not necessary

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



[{"act_background_light":13,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":3,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]},{"act_background_light":0,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":3,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]},{"act_background_light":13,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":3,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

orange and green compare
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}
,{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":1,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"measlights":[[12,20,12,20]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}
,{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[16],[16],[16],[16]],"act":[2,1,2,2]}
,{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":1,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"measlights":[[12,20,12,20]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[16],[16],[16],[16]],"act":[2,1,2,2]}]

[{"measurements":2,"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":1,"pulsesize":10,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]}
,{"measurements":2,"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1]],"averages":2,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":10,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

tcs_to_act

[{"environmental":[["relative_humidity",0],["temperature",0],["light_intensity",0],["co2",0]],"tcs_to_act":100,"protocol_name":"baseline_sample","protocols_delay":5,"act_background_light":20,"actintensity1":5,"actintensity2":5,"averages":1,"wait":0,"cal_true":2,"analog_averages":1,"pulsesize":10,"pulsedistance":3000,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]},{"tcs_to_act":100,"environmental":[["relative_humidity",0],["temperature",0],["light_intensity",0],["co2",0]],"protocols_delay":5,"act_background_light":20,"protocol_name":"fluorescence","baselines":[1,1,1,1],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":10,"pulsedistance":10000,"actintensity1":5,"actintensity2":50,"measintensity":7,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[0,1,0,0]}]

[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1],["co2",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":25,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]


just fluorescence, no env or baseline
[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":10,"pulsedistance":10000,"actintensity1":5,"actintensity2":50,"measintensity":7,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]


[{"protocol_name":"env_all","environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":15,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[1],"detectors":[[34]],"measlights":[[15]],"act":[2]}]



[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]


[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":100,"pulsedistance":10000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

act_background_light

!! FOR JESSE !!

BLANK WAIT
[{"protocol_name":"blank","averages":2,"wait":5,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":100,"pulses":[2000000],"detectors":[[34]],"measlights":[[13]], "act_light":[13],"act":[1]}]

NDVI PLUS FLUORESCENCE
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}
,{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":1,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"measlights":[[12,20,12,20]]}]

FLUORESCENCE
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

CALIBRATIONS
[{"protocol_name":"calibration","baselines":[0,0,0,0],"averages":3,"wait":10,"cal_true":1,"measurements_delay":10,"analog_averages":12,"pulsesize":50,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":3,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"measlights":[[14,15,16,20,14,15,16,20]]}
,{"protocol_name":"calibration_spad_ndvi","measurements":1,"baselines":[0,0,0,0],"measurements_delay":10,"averages":3,"wait":10,"cal_true":1,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"measlights":[[14,15,16,20,14,15,16,20]]}]



!! FOR JESSE 2 MEASUREMENTS !!

!! GOOD KEEP
[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]
[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]}]
[{"protocol_name":"calibration","baselines":[0,0,0,0],"averages":3,"wait":10,"cal_true":1,"analog_averages":12,"pulsesize":50,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":3,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"measlights":[[14,15,16,20,14,15,16,20]]}]

!! GOOD KEEP
[{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":10,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"measlights":[[12,20,12,20]]}]
[{"protocol_name":"calibration_spad_ndvi","measurements":1,"baselines":[0,0,0,0],"measurements_delay":10,"averages":3,"wait":10,"cal_true":1,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"measlights":[[14,15,16,20,14,15,16,20]]}]
 
!! GOOD KEEP
[{"protocol_name":"dirk","baselines":[0,0,0,0],"averages":5,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":5,"pulsedistance":3000,"actintensity1":255,"actintensity2":255,"measintensity":60,"calintensity":0,"pulses":[100,100,100],"detectors":[[35],[35],[35]],"measlights":[[16],[16],[16]],"act":[1,2,1]}]
[{"protocol_name":"firk","baselines":[0,0,0,0],"averages":5,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":5,"pulsedistance":3000,"actintensity1":255,"actintensity2":255,"measintensity":60,"calintensity":0,"pulses":[100,100,100],"detectors":[[35],[35],[35]],"measlights":[[16],[16],[16]],"act":[2,1,2]}]



 
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

//#define DEBUG 1  // uncomment to add full debug features
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


//////////////////////DEVICE ID FIRMWARE VERSION////////////////////////
// DEVICE ID SHOULD BE UNIQUE FOR EACH DEVICE:  Next device name should have device ID == 2
int device_id = 1;
const char* firmware_version = "Alfie Arabidopsis";

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
  0,15,16,20};                                                       // this is a lookup table for the measurement lights which are listed in 'array'.  Looking up 0 returns 0.
int averages = 1;
int pwr_off_state = 0;
int pwr_off_lights_state = 0;

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
float valMultiplier = 0.1;
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
    Wire.begin(I2C_MASTER,0x00,I2C_PINS_18_19,I2C_PULLUP_EXT,I2C_RATE_1200);
//  tmp006.begin();                                                              // this opens wire.begin as well, and initializes tmp006, tcs light sensor, and htu21D.  by default, contactless leaf temp measurement takes 4 seconds to complete
//  tcs.begin();
  dac.vdd(5000); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout

/*
    if(!TCS3471.detect())
        {
            Serial.println("Something is not right. Check your wiring, sensor needs at least VCC, GND, SCL and SDA.");
            while (1);
        }
*/
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
//  pinMode(MEASURINGLIGHT_PWM, OUTPUT); 
//  pinMode(ACTINICLIGHT_INTENSITY2, OUTPUT);
//  pinMode(ACTINICLIGHT_INTENSITY1, OUTPUT);
  pinMode(ACTINICLIGHT_INTENSITY_SWITCH, OUTPUT);
//  pinMode(CALIBRATINGLIGHT_PWM, OUTPUT);
  pinMode(SAMPLE_AND_HOLD, OUTPUT);
  pinMode(PWR_OFF, OUTPUT);
  pinMode(PWR_OFF_LIGHTS, OUTPUT);
  pinMode(BATT_LEVEL, INPUT); 
  digitalWriteFast(PWR_OFF, LOW);                                               // pull high to power off, pull low to keep power.
  digitalWriteFast(PWR_OFF_LIGHTS, HIGH);                                               // pull high to power off, pull low to keep power.
  pinMode(13, OUTPUT);
  analogReadAveraging(1);                                                       // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u) - this gets changed later by each protocol
  pinMode(DETECTOR1, EXTERNAL);                                                 // use external reference for the detectors
  pinMode(DETECTOR2, EXTERNAL);
  analogReadRes(ANALOGRESOLUTION);                                              // set at top of file, should be 16 bit
  analogresolutionvalue = pow(2,ANALOGRESOLUTION);                              // calculate the max analogread value of the resolution setting
  analogWriteFrequency(3, 187500);                                              // Pins 3 and 5 are each on timer 0 and 1, respectively.  This will automatically convert all other pwm pins to the same frequency.
  analogWriteFrequency(5, 187500);
  pinMode(DAC_ON, OUTPUT);
  digitalWriteFast(DAC_ON, LOW);                                               // pull high to power off, pull low to keep power.

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
  if (pwr_off_state == 0) {
    Serial.println("\"pwr_off\":\"HIGH\"");
    Serial1.println("\"pwr_off\":\"HIGH\"");
    digitalWriteFast(PWR_OFF, HIGH);
    pwr_off_state = 1;
  }
  else if (pwr_off_state == 1) {
    Serial.println("\"pwr_off\":\"LOW\"");
    Serial1.println("\"pwr_off\":\"LOW\"");
    digitalWriteFast(PWR_OFF, LOW);    
    pwr_off_state = 0;
  }
}

void pwr_off_lights() {
  if (pwr_off_lights_state == 0) {
    Serial.println("\"pwr_off_lights\":\"HIGH\"");
    Serial1.println("\"pwr_off_lights\":\"HIGH\"");
    digitalWriteFast(PWR_OFF_LIGHTS, HIGH);
    pwr_off_lights_state = 1;
  }
  else if (pwr_off_lights_state == 1) {
    Serial.println("\"pwr_off_lights\":\"LOW\"");
    Serial1.println("\"pwr_off_lights\":\"LOW\"");
    digitalWriteFast(PWR_OFF_LIGHTS, LOW);    
    pwr_off_lights_state = 0;
  }
}

void batt_level() {                                                      // 1024 counts == 5V of battery voltage.  We'll output the values for 6 (1229 counts) and 12 (1458 counts) volts.
  Serial.print("\"batt_level_6V\": \"");
  Serial1.print("\"batt_level_6V\": \"");
  float level6 = (float) analogRead(BATT_LEVEL)/(205*6);
  Serial.print(level6*100);
  Serial1.print(level6*100);
  Serial.print("%\"");
  Serial1.print("%\"");
  Serial.print(",\"batt_level_12V\": \"");
  Serial1.print(",\"batt_level_12V\": \"");
  float level12 = (float) analogRead(BATT_LEVEL)/(205*12);
  Serial.print(level12*100);
  Serial1.print(level12*100);
  Serial.print("%\",");
  Serial1.print("%\",");
}

//////////////////////// MAIN LOOP /////////////////////////
void loop() {
  
  delay(500);                                                                   // 
  int measurements = 1;                                                   // the number of times to repeat the entire measurement (all protocols)
  unsigned long measurements_delay = 0;                                    // number of milliseconds to wait between measurements  
  volatile unsigned long meas_number = 0;                                       // counter to cycle through measurement lights 1 - 4 during the run

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
    json2[i] = "";                                          // reset all json2 char's to zero (ie reset all protocols)
  }
  
digitalWriteFast(SAMPLE_AND_HOLD,LOW);                                          // discharge sample and hold in case the cap has be
delay(10);
digitalWriteFast(SAMPLE_AND_HOLD,HIGH);

  while (Serial.peek() != 91 && Serial1.peek() != 91) {     // wait till we see a "[" to start the JSON of JSONS
//    Serial.println("I am here!");
//    delay(1000);
    if (Serial.peek() == '9' | Serial1.peek() == '9') {                              // look for 9 for confirmation that device is working
      Serial.read();                                      // flush the '9'
      Serial1.read();                                      // flush the '9'
      Serial.println("MultispeQ Ready");
      Serial1.println("MultispeQ Ready");
    }
    else if (Serial.peek() == '8' | Serial1.peek() == '8') {                      // run light tests
      Serial.read();
      Serial1.read();
      lighttests();
    }
    else if (Serial.peek() == '7' | Serial1.peek() == '7') {                      // teensy power down on pin 22
      Serial.read();
      Serial1.read();
      pwr_off_lights();
    }
    else if (Serial.peek() == '6' | Serial1.peek() == '6') {                      // teensy power down on pin 22
      Serial.read();
      Serial1.read();
      pwr_off();
    }
    else if (Serial.peek() == '5' | Serial1.peek() == '5') {                      // teensy power down on pin 22
      Serial.read();
      Serial1.read();
      batt_level();
    }
    else if (Serial.peek() == '4' | Serial1.peek() == '4') {                      // run light tests
      Serial.read();
      Serial1.read();
      lighttests_all();
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

      recall_all();                                        							// recall and save the data from the EEPROM
      if (baseline_flag == 1) {                           							 // calculate baseline values from saved EEPROM data only if the previous run was a sample baseline run (otherwise leave baseline == 0)
        cal_baseline();
      }

      free(json);                                                                        // make sure this is here! Free before resetting the size according to the serial input
      json = (char*)malloc((json2[q].length()+1)*sizeof(char));
      strcpy(json,json2[q].c_str());  
      json[json2[q].length()] = '\0';                                                    // Add closing character to char*
      hashTable = root.parseHashTable(json);
      if (!hashTable.success()) {
        Serial.println("{\"error\":\"JSON not recognized, or other failure with Json Parser\"}");
        Serial1.println("{\"error\":\"JSON not recognized, or other failure with Json Parser\"}");
        while (Serial.available()>0) {                                                   // Flush incoming serial of the failed command
          Serial.read();
        }
        return;
      }

      int cal_true =            hashTable.getLong("cal_true");                             // identify this as a calibration routine (0 = not a calibration routine, 1 = calibration routine, 2 = create baseline for sample)
      String protocol_name =    hashTable.getString("protocol_name");                      // used only for calibration routines ("cal_true" = 1 or = 2)
      averages =                hashTable.getLong("averages");                               // The number of times to average this protocol.  The spectroscopic and environmental data is averaged in the device and appears as a single measurement.
      if (averages == 0) {                                                                  // if averages don't exist, set it to 1 automatically.
        averages = 1;
      }
      int wait =                hashTable.getLong("wait");                                    // seconds wait time between 'averages'
      int protocols_delay =     hashTable.getLong("protocols_delay");                         // delay between protocols within a measurement
      measurements =            hashTable.getLong("measurements");                            // number of times to repeat a measurement, which is a set of protocols
      measurements_delay =      hashTable.getLong("measurements_delay");                      // delay between measurements in seconds
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
    
      int tcs_to_act =          hashTable.getLong("tcs_to_act");                         // sets the % of response from the tcs light sensor to act as actinic during the run (values 1 - 100).  If tcs_to_act is not defined (ie == 0), then the act_background_light intensity is set to actintensity1.
      int pulsesize =           hashTable.getLong("pulsesize");                         // Size of the measuring pulse (5 - 100us).  This also acts as gain control setting - shorter pulse, small signal. Longer pulse, larger signal.  
      int pulsedistance =       hashTable.getLong("pulsedistance");                       // distance between measuring pulses in us.  Minimum 1000 us.
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
      digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                               // put the switch to the actinic (low) position permanently
      JsonArray act_intensities =      hashTable.getArray("act_intensities");                         // write to input register of a DAC. channel 0 for low (actinic).  1 step = +3.69uE (271 == 1000uE, 135 == 500uE, 27 == 100uE)
      JsonArray meas_intensities =     hashTable.getArray("meas_intensities");                        // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
      JsonArray cal_intensities =      hashTable.getArray("cal_intensities");                        // write to input register of a DAC. channel 2 calibrating light.  0 (low) - 4095 (high).
      JsonArray act1 =           hashTable.getArray("act");                                // the state (0:preset 1, 1:preset 2, 2:off) of the actinic light in an array.  For example, [0,2,2] is actinic light on at preset 1, actinic light on at preset 2, actinic light off
      JsonArray act2 =          hashTable.getArray("act2");                                // the state (0:on, 1:on, 2:off) of the actinic 2 light in an array.  Preset value is defined by 'act' above.  For example, [0,1,2] is actinic 2 on, on, off.
      JsonArray alt1 =          hashTable.getArray("alt1");                                // the state (0:off, 1:on) of the alternate light 1 in an array.  For example, [0,1,1] is alternate light 1 off, on, on
      JsonArray alt2 =          hashTable.getArray("alt2");                                // the state (0:off, 1:on) of the alternate light 2 in an array.  For example, [0,1,1] is alternate light 2 off, on, on
      JsonArray detectors =     hashTable.getArray("detectors");                           // the Teensy pin # of the detectors used during those pulses, as an array of array.  For example, if pulses = [5,2] and detectors = [[34,35],[34,35]] .  
      JsonArray measlights =    hashTable.getArray("measlights");
      JsonArray baselines =     hashTable.getArray("baselines");                           // mark which cycles you want to enable the baseline subtraction
      JsonArray environmental = hashTable.getArray("environmental");
      total_cycles =            pulses.getLength()-1;                                      // (start counting at 0!)

      free(data_raw_average);
      int size_of_data_raw = 0;
      for (i=0;i<pulses.getLength();i++) {
        size_of_data_raw += pulses.getLong(i) * measlights.getArray(i).getLength();
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
      Serial.print("arrays in measlights:  ");
      Serial.println(measlights.getLength());

      Serial.println();
      Serial.print("length of measlights arrays:  ");
      for (i=0;i<measlights.getLength();i++) {
        Serial.print(measlights.getArray(i).getLength());
        Serial.print(", ");
      }
      Serial.println();
#endif

      Serial1.print("{\"protocol_name\": \"");
      Serial.print("{\"protocol_name\": \"");
      Serial1.print(protocol_name);
      Serial.print(protocol_name);
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
      Serial.print("],\"baseline_sample\": ");
      Serial1.print("],\"baseline_sample\": ");
      Serial.print(cal1_sample,2);
      Serial1.print(cal1_sample,2);
      Serial.print(" ,");
      Serial1.print(",");
      Serial1.print("\"averages\": "); 
      Serial.print("\"averages\": "); 
      Serial1.print(averages);  
      Serial.print(averages); 
      Serial1.print(","); 
      Serial.print(","); 

      // this should be an array, so I can reset it all......
      relative_humidity_average = 0;                                                                    // reset all of the environmental variables
      temperature_average = 0;
      objt_average = 0;
      co2_value_average = 0;
      lux_average = 0;
      r_average = 0;
      g_average = 0;
      b_average = 0;

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
              Serial1.print(lux_average);  
              Serial1.print(",");
              Serial.print(lux_average);  
              Serial.print(",");  
              Serial1.print("\"r\": ");
              Serial.print("\"r\": ");
              Serial1.print(r_average);  
              Serial1.print(",");
              Serial.print(r_average);  
              Serial.print(",");  
              Serial1.print("\"g\": ");
              Serial.print("\"g\": ");
              Serial1.print(g_average);  
              Serial1.print(",");
              Serial.print(g_average);  
              Serial.print(",");  
              Serial1.print("\"b\": ");
              Serial.print("\"b\": ");
              Serial1.print(b_average);  
              Serial1.print(",");
              Serial.print(b_average);  
              Serial.print(",");  
            }
          }
        }
      int actfull = 0;

/*
 	[{"act_light":20,"act":[0,2,0,1,0,2,0,1],"actintensity1":27,"actintensity2":1370,"measintensity":147,"calintensity":4095,"measurements":1,"measurements_delay":0,"pulses":[100,100,100,100,100,100,100,100],"measlights":[[15],[14],[15],[14],[15],[14],[15],[14]],"pulsesize":10,"pulsedistance":10000,"averages":1}]
[{"averages":1,"wait":0,"cal_true":0,"analog_averages":1,"act_light":20,"pulsesize":10,"pulsedistance":10000,"actintensity1":27,"actintensity2":1370,"measintensity":147,"calintensity":4095,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act_intensities":[100,1000,100,100],"act_lights":[20,20,20,20]}]
*/

      if (tcs_to_act > 0) {                                                           // if you're setting actinic intensity based on outside light as per the tcs light sensor.  The deep red actinic produces 103 uE per pwm step.
        _act_intensity = (lux_to_uE(lux_average)*tcs_to_act)/(3.69*100);
        actfull = (lux_to_uE(lux_average)/3.69);                                     // just to confirm that actintensity is in fact a fraction of lux_to_uE if tcs_to_act is not 100%.
      }
      
      analogReadAveraging(analog_averages);                                      // set analog averaging (ie ADC takes one signal per ~3u)
      dac.analogWrite(0,act_intensities.getLong(0));                            // Set to intensity at initial position in array.  Write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
      dac.analogWrite(1,1150);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                     // preset the switch to the actinic (low) preset position
      dac.analogWrite(2,meas_intensities.getLong(0));                                                       // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
      dac.analogWrite(3,cal_intensities.getLong(0));                                                       // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
      delay(1);                                                                 // give time for the lights to stabilize      
      
      digitalWriteFast(act_background_light, HIGH);                                    // turn on the actinic background light to the low preset position and keep it on until the end of the measurement

#ifdef DEBUGSIMPLE
      Serial.println();
      Serial.print("% of actinic intensity tcs_to_act:   ");
      Serial.println(tcs_to_act);
      Serial.print("lux in uE:                           ");
      Serial.println(lux_to_uE(lux_average));
      Serial.print("lux raw:   ");
      Serial.println(lux_average);
      Serial.print("actinic intensity at # of tcs:       ");
      Serial.println(actintensity1);
      Serial.print("actinic intensity at full:           ");
      Serial.println(actfull);
#endif

        for (z=0;z<size_of_data_raw;z++) {                                            // cycle through all of the pulses from all cycles
          if (cycle == 0 && pulse == 0) {                                             // if it's the beginning of a measurement, then...                                                             // wait a few milliseconds so that the actinic pulse presets can stabilize
            Serial.flush();                                                          // flush any remaining serial output info before moving forward
            starttimer0 = micros();
            timer0.begin(pulse1,pulsedistance);                                       // Begin firsts pulse
            while (micros()-starttimer0 < pulsesize) {
            }                               // wait a full pulse size, then...                                                                                          
            timer1.begin(pulse2,pulsedistance);                                       // Begin second pulse
          }      

          meas_array_size = measlights.getArray(cycle).getLength();
          _meas_light = measlights.getArray(cycle).getLong(meas_number%meas_array_size);                                    // move to next measurement light
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

//

//TAKE IT FROM HERE!!




          if (pulse == 0) {                                                                // if it's the first pulse of a cycle, then change act 1 and 2, alt1 and alt2 values as per array's set at beginning of the file

          _act1_light_prev = _act1_light;                                                        // pull all new values for lights and detectors from the JSON
          _act1_light = act1_lights.getLong(cycle);
          
          _act2_light_prev = _act2_light;
          _act2_light = act2_lights.getLong(cycle);
          
          _alt1_light_prev = _alt1_light;
          _alt1_light = alt1_lights.getLong(cycle);
          
          _alt2_light_prev = _alt2_light;
          _alt2_light = alt2_lights.getLong(cycle);
                    
          _act_intensity = act_intensities.getLong(cycle);                                        // pull new intensities from the JSON
          _meas_intensity = meas_intensities.getLong(cycle);
          _cal_intensity = cal_intensities.getLong(cycle);
                    
          digitalWriteFast(_act1_light_prev, LOW);                                                // turn off previous lights, turn on the new ones
          digitalWriteFast(_act1_light, HIGH);
          digitalWriteFast(_act2_light_prev, LOW);
          digitalWriteFast(_act2_light, HIGH);
          digitalWriteFast(_alt1_light_prev, LOW);
          digitalWriteFast(_alt1_light, HIGH);
          digitalWriteFast(_alt2_light_prev, LOW);
          digitalWriteFast(_alt2_light, HIGH);

//          dac.analogWrite(0,actintensity2);                                                       // write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
          dac.analogWrite(1,_act_intensity);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
          digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                     // preset the switch to the actinic (low) preset position
          dac.analogWrite(2,_meas_intensity);                                                       // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
          dac.analogWrite(3,_cal_intensity);                                                       // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
          }
          
#ifdef DEBUGSIMPLE
          Serial.print("   ");
          Serial.print(act_state);
          Serial.print("   ");
          Serial.print(act_int_state);
          Serial.print("   ");
          Serial.print(alt1_state);
          Serial.print("    ");
          Serial.print(alt2_state);
          Serial.print("    ");
          Serial.println(act2_state);
#endif
          while (on == 0 | off == 0) {                                        	     // if ALL pulses happened, then...
          }
          data1 = analogRead(detector);                                              // save the detector reading as data1, and subtract of the baseline (if there is no baseline then baseline is automatically set = 0)     
#ifdef DEBUGSIMPLE
          Serial.print(baseline);
          Serial.print(", ");
          Serial.println(data1);
#endif

/*
          digitalWriteFast(SAMPLE_AND_HOLD, HIGH);						// turn on measuring light and/or actinic lights etc., tick counter
          digitalWriteFast(act_light, act_state);                                               // turn on actinic and other lights
          digitalWriteFast(act2_light, act2_state);
          digitalWriteFast(alt1_light, alt1_state);    
          digitalWriteFast(alt2_light, alt2_state);
*/
          for (i=0;i<baselines.getLength();i++) {                                      // check the current light and see if a baseline should be applied to it.
            if (baselines.getLong(i) == 1) {
              if (baseline_lights[i] == measlights.getArray(cycle).getLong(meas_number%meas_array_size)) {
                baseline = baseline_array[i];
                break;
              }
            }
          }
          noInterrupts();                                                              // turn off interrupts because we're checking volatile variables set in the interrupts
          on = 0;                                                                      // reset pulse counters
          off = 0;  
          pulse++;                                                                     // progress the pulse counter and measurement number counter
          data_raw_average[meas_number] += data1 - baseline;  
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

          if (pulse == pulses.getLong(cycle)*measlights.getArray(cycle).getLength()) {                                             // if it's the last pulse of a cycle...
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
/*    
        if (act_light == act_background_light) {
          digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                     // switch back to actinic position (low)
        }
        else {
          digitalWriteFast(act_light, LOW);
        }
        if (act2_light == act_background_light) {
          digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                     // switch back to actinic position (low)
        }
        else {
          digitalWriteFast(act2_light, LOW);
        }
        digitalWriteFast(alt1_light, LOW);
        digitalWriteFast(alt2_light, LOW);
/*        
#ifdef DEBUGSIMPLE
        Serial.print("actinic intensity now:         ");
        Serial.println(actintensity1);
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

/*
            if (x == averages-1) {                                                                // if it's the last measurement to average, then print the results
              Serial1.print("\"relative_humidity\": ");
              Serial.print("\"relative_humidity\": ");
              Serial1.print(relative_humidity_average);  
              Serial1.print(",");
              Serial.print(relative_humidity_average);  
              Serial.print(",");
            }
            if (x == averages-1) {
              Serial1.print("\"temperature\": ");
              Serial.print("\"temperature\": ");
              Serial1.print(temperature_average);  
              Serial1.print(",");
              Serial.print(temperature_average);  
              Serial.print(",");     
            }       
            if (x == averages-1) {
              Serial1.print("\"contactless_temperature\": ");
              Serial.print("\"contactless_temperature\": ");
              Serial1.print(objt_average);  
              Serial1.print(",");
              Serial.print(objt_average);  
              Serial.print(",");
            }
            if (x == averages-1) {
              Serial1.print("\"co2\": ");
              Serial.print("\"co2\": ");
              Serial1.print(co2_value_average);  
              Serial1.print(",");
              Serial.print(co2_value_average);  
              Serial.print(",");
            }
            if (x == averages-1) {
              Serial1.print("\"light_intensity\": ");
              Serial.print("\"light_intensity\": ");
              Serial1.print(lux_average);  
              Serial1.print(",");
              Serial.print(lux_average);  
              Serial.print(",");  
              Serial1.print("\"r\": ");
              Serial.print("\"r\": ");
              Serial1.print(r_average);  
              Serial1.print(",");
              Serial.print(r_average);  
              Serial.print(",");  
              Serial1.print("\"g\": ");
              Serial.print("\"g\": ");
              Serial1.print(g_average);  
              Serial1.print(",");
              Serial.print(g_average);  
              Serial.print(",");  
              Serial1.print("\"b\": ");
              Serial.print("\"b\": ");
              Serial1.print(b_average);  
              Serial1.print(",");
              Serial.print(b_average);  
              Serial.print(",");  
            }
*/



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
              Serial1.print(lux_average);  
              Serial1.print(",");
              Serial.print(lux_average);  
              Serial.print(",");  
              Serial1.print("\"r\": ");
              Serial.print("\"r\": ");
              Serial1.print(r_average);  
              Serial1.print(",");
              Serial.print(r_average);  
              Serial.print(",");  
              Serial1.print("\"g\": ");
              Serial.print("\"g\": ");
              Serial1.print(g_average);  
              Serial1.print(",");
              Serial.print(g_average);  
              Serial.print(",");  
              Serial1.print("\"b\": ");
              Serial.print("\"b\": ");
              Serial1.print(b_average);  
              Serial1.print(",");
              Serial.print(b_average);  
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
          cal1_sample = cal1sum_sample/(cal_pulses-4);                                        // when converting cal1_sum to a per-measurement value, we must take into account that we ignored the first 4 values when summing to avoid the artifacts (see above)
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
          countdown(wait);
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
      wait = 0;                  	                                                // seconds wait time between averages
      analog_averages = 1;                                                             // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
      _act1_light = 0;
      _act2_light = 0;
      _alt1_light = 0;
      _alt2_light = 0;
      detector = 0;
      pulsesize = 0;                                                                // measured in microseconds
      pulsedistance = 0;
      _act_intensity = 0;                                                            // intensity at LOW setting below
//      act_intensity2 = 0;                                                            // intensity at HIGH setting below
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
        //    for (i=0;i<sizeof(baseline_array)/sizeof(float);i++) {
        //      baseline_array[i] = 0;
        //    }
        baseline_flag = 0;
      }
      else {                                                                    // flag to indicate we ran a baseline run first - ensures that the next run we calculate the baseline data and use it
        baseline_flag = 1;
      }
    }
    while (Serial.available()>0) {                                                   // Flush incoming serial of any remaining information
      Serial.read();
    }

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

  Serial.setTimeout(600000);                                                            // set timeout for readBytesUntil below
  
  Serial.println("enter value to increment lights (suggested value is 40) followed by +");
  Serial.println("or press only +0 to exit");
  char serial_buffer [50];
  int increment = 0;
  
   /*
   DAC switch numbers are:
   actinic intensity switch low is DAC 1
   actinic intensity switch high is DAC 0
   intensity for measurement is DAC 3
   intensity for calibration is DAC 2
   */

  Serial.readBytesUntil('+',serial_buffer,50);
  increment = atoi(serial_buffer);  
  Serial.println(increment);  
  if (increment == 0) {
    Serial.println("back to main menu");
    goto skipit;
  }
  Serial.println(increment);  
  Serial.read();
  Serial.read();

  digitalWriteFast(ACTINICLIGHT1,HIGH);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);
  Serial.println();
  Serial.print("DAC number:   ");  Serial.println(0);
  Serial.println("actinic intensity 1 (switch LOW)");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(0,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }

  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
  Serial.println();
  Serial.print("DAC number:   ");  Serial.println(1);
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
  Serial.print("DAC number:   ");  Serial.println(0);
  Serial.println("actinic intensity 2 (switch LOW)");
  for (i=0;i<4095;i=i+increment) {
    dac.analogWrite(0,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
  Serial.println();
  Serial.print("DAC number:   ");  Serial.println(1);
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
  Serial.print("DAC number:   ");  Serial.println(2);
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
  Serial.print("DAC number:   ");  Serial.println(2);
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
  Serial.print("DAC number:   ");  Serial.println(1);
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
  Serial.print("DAC number:   ");  Serial.println(1);
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
  Serial.print("DAC number:   ");  Serial.println(1);
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
  Serial.setTimeout(1000);                                                            // put settimeout back to zero
  while (Serial.available()>0 | Serial1.available()>0) {                                                            // flush incoming serial
    Serial.read();
    Serial1.read();
  } 
}

void lighttests() {
  while (Serial.available()>0 | Serial1.available()>0) {                                                            // flush incoming serial
    Serial.read();
    Serial1.read();
  } 
  int choose = 0;
      dac.analogWrite(0,4095);                                                       // write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
      dac.analogWrite(1,4095);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
//      analogWrite(ACTINICLIGHT_INTENSITY2, actintensity2);                      // this is high (saturating)
//      analogWrite(ACTINICLIGHT_INTENSITY1, actintensity1);                      // set intensities for low (actinic)
      digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                     // preset the switch to the actinic (low) preset position
      dac.analogWrite(2,4095);                                                       // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
      dac.analogWrite(3,4095);                                                       // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
//      analogWrite(MEASURINGLIGHT_PWM, measintensity);
//      analogWrite(CALIBRATINGLIGHT_PWM, calintensity);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);

  while (choose!=999) {

    Serial.println("enter value to test:");
    Serial.println("015 - measuring light 1 (main board)");
    Serial.println("016 - measuring light 2 (main board)");
    Serial.println("011 - measuring light 3 (add on board)");
    Serial.println("012 - measuring light 4 (add on board)");
    Serial.println("020 - actinic light 1 (main board)");
    Serial.println("002 - actinic light 2 (add on board)");
    Serial.println("014 - calibrating light 1 (main board)");
    Serial.println("010 - calibrating light 2 (add on board)");
    Serial.println("A10 - detector 1 (main board)");
    Serial.println("A11 - detector 2 (add on board)");
    Serial.println("101 - light detector testing (press any key to exit after entering))");
    Serial.println("102 - CO2 testing (press any key to exit after entering))");
    Serial.println("103 - temperature testing (press any key to exit after entering))");
    Serial.println("104 - relative humidity testing (press any key to exit after entering))");
    Serial.println("or enter 999 to exit");  

    Serial1.println("enter value to test:");
    Serial1.println("015 - measuring light 1 (main board)");
    Serial1.println("016 - measuring light 2 (main board)");
    Serial1.println("011 - measuring light 3 (add on board)");
    Serial1.println("012 - measuring light 4 (add on board)");
    Serial1.println("020 - actinic light 1 (main board)");
    Serial1.println("002 - actinic light 2 (add on board)");
    Serial1.println("014 - calibrating light 1 (main board)");
    Serial1.println("010 - calibrating light 2 (add on board)");
    Serial1.println("A10 - detector 1 (main board)");
    Serial1.println("A11 - detector 2 (add on board)");
    Serial1.println("101 - light detector testing (press any key to exit after entering))");
    Serial1.println("102 - CO2 testing (press any key to exit after entering))");
    Serial1.println("103 - temperature testing (press any key to exit after entering))");
    Serial1.println("104 - relative humidity testing (press any key to exit after entering))");
    Serial1.println("or enter 999 to exit");  

    while (Serial.available()<3 && Serial1.available()<3) {
    }
    choose = calc_Protocol();
    Serial.println(choose);
    Serial1.println(choose);

    if (choose == 101) {
      Serial.print("{\"light_intensity\":[");
      Serial1.print("{\"light_intensity\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        Serial.print(Light_Intensity(1));
        Serial1.print(Light_Intensity(1));
        Serial.print(",");
        Serial1.print(",");
        delay(2000);
      }
      while (Serial.available()>0) {
        Serial.read();
      }
      while (Serial1.available()>0) {
        Serial1.read();
      }
    }
    else if (choose == 102) {                      // co2
      Serial.print("{\"Co2_value\":[");
      Serial1.print("{\"Co2_value\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        Serial.print(Co2(1));
        Serial.print(",");
        Serial1.print(Co2(1));
        Serial1.print(",");
        delay(2000);
      }
      while (Serial.available()>0) {
        Serial.read();
      }
      while (Serial1.available()>0) {
        Serial1.read();
      }
    }
    else if (choose == 103) {                      // temperature
      Serial.print("{\"temperature\":[");
      Serial1.print("{\"temperature\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        Serial.print(Temperature(1));
        Serial.print(",");
        Serial1.print(Temperature(1));
        Serial1.print(",");
        delay(2000);
      }
      while (Serial.available()>0) {
        Serial.read();
      }
      while (Serial1.available()>0) {
        Serial1.read();
      }
    }
    else if (choose == 104) {                      //relative humidity
      Serial.print("{\"relative_humidity\":[");
      Serial1.print("{\"relative_humidity\":[");
      while (Serial.available()<1 && Serial1.available()<1) {
        Serial.print(Relative_Humidity(1));
        Serial.print(",");
        Serial1.print(Relative_Humidity(1));
        Serial1.print(",");
        delay(2000);
      }
      while (Serial.available()>0) {
        Serial.read();
      }
      while (Serial1.available()>0) {
        Serial1.read();
      }
    }
    Serial.println("0]}");
    Serial1.println("0]}");
      while (Serial.available()>0) {
        Serial.read();
      }
      while (Serial1.available()>0) {
        Serial1.read();
      }
    if (choose<30) {
      Serial.println("First actinic intensty switch high, then actinic intensity switch low");
      Serial1.println("First actinic intensty switch high, then actinic intensity switch low");
      delay(1000);
      for (y=0;y<2;y++) {
        for (x=0;x<4025;x=x+10) {
          Serial.println(x);
          Serial1.println(x);
          dac.analogWrite(0,x);                                                       // write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
          dac.analogWrite(1,x);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
          dac.analogWrite(2,x);                                                       // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
          dac.analogWrite(3,x);                                                       // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
          if (y==0) {
            digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);
          }
          else {
            digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
          }
          delay(2);
          digitalWriteFast(choose, HIGH);
          delay(2);
          digitalWriteFast(choose, LOW);
        }
        for (x=4025;x>0;x=x-10) {
          Serial.println(x);
          Serial1.println(x);
          dac.analogWrite(0,x);                                                       // write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
          dac.analogWrite(1,x);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
          dac.analogWrite(2,x);                                                       // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
          dac.analogWrite(3,x);                                                       // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
          if (y==0) {
            digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);
          }
          else {
            digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
          }
          delay(2);
          digitalWriteFast(choose, HIGH);
          delay(2);
          digitalWriteFast(choose, LOW);
        }
      }
    }
    else {
      int reading = 0;
      switch (choose) {
      case 1710:
        for (x=0;x<1000;x++) {
          digitalWriteFast(SAMPLE_AND_HOLD, LOW);
          delayMicroseconds(15);
          reading = analogRead(A10);
          digitalWriteFast(SAMPLE_AND_HOLD, HIGH);
          Serial.println(reading);
          Serial1.println(reading);
          delay(10);
        }
        break;

      case 1711:
        for (x=0;x<1000;x++) {
          digitalWriteFast(SAMPLE_AND_HOLD, LOW);
          delayMicroseconds(15);
          reading = analogRead(A11);
          digitalWriteFast(SAMPLE_AND_HOLD, HIGH);
          Serial.println(reading);
          Serial1.println(reading);
          delay(10);
        }
        break;
      }
    }
  }
}
/*
    if (choose<30) {
      while (1==1) {
        while (Serial.available() > 0) {
          Serial.read();
        }
        Serial.println("Choose a value to set the pwm (3 digits, for example '001'), press 'e' key to exit");
        if (Serial.peek() == 101) {
          break;
        }
        Serial.println(Serial.available());
        while (Serial.available()<3) {
        }  
        choose = calc_Protocol();  
        analogWrite(MEASURINGLIGHT_PWM, x);
        analogWrite(CALIBRATINGLIGHT_PWM, x);
        analogWrite(ACTINICLIGHT_INTENSITY1, x);
        analogWrite(ACTINICLIGHT_INTENSITY2, x);
        digitalWriteFast(choose, HIGH);
      }
    }
*/

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
/*	lux += TCS3471.readCData();
	r += TCS3471.readRData();
	g += TCS3471.readGData();
	b += TCS3471.readBData();
	lux += TCS3471.readCData();
	r += TCS3471.readRData();
	g += TCS3471.readGData();
	b += TCS3471.readBData();
      lux = lux/3;                                  // average them together
      r = r/3;
      g = g/3;
      b = b/3;
*/
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
//  else if (var1 == 3) {
    // PAR calibration based on known value... not yet implemented!
//  }
}

/**/
float lux_to_uE(float _lux_average) {
//  int uE = (_lux_average-996.668)/30.64773;           // units 1,2,6 with lamp
//  int uE = (_lux_average-1739.21)/35.84464;           // units 3 with lamp
//  int uE = (_lux_average-187.4785)/15.03928;           // units 5  with lamp
//  int uE = (_lux_average-.35)/.02135;           // units 8 and 4 in chambers
//  int uE = (_lux_average-.3692)/.0408;           // units 7 in chambers
  int uE = (_lux_average+65)/6.30;           // units 8 and 4 outside
//  int uE = (_lux_average+250.35)/15.796;           // units 7 outside

  return uE;
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

/*
void Co2_flag() {
 co2_flag = 1;
 }
 */

unsigned long Co2(int var1) {
  if (var1 == 1 | var1 == 0) {
    requestCo2(readCO2);
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
    requestCo2(readCO2);
    unsigned long co2_value = getCo2(response);
    Serial1.print("[{\"co2_content\": ");
    Serial.print("\"co2_content\": ");
    Serial1.print(co2_value);  
    Serial1.print("}]");
    Serial.print(co2_value);  
    Serial.print("}]");
    return co2_value;
    delay(5000);
  }    
    
    // THIS STILL NEEDS WORK.  Entirely optional, but may be nice so that CO2 data is able to be viewed in the raw_data view and can generate macro outputs...
    /*  
     void Co2_evolution(int stop_at_max, int stop_at_min, int freq, int points_to_sum, int sum_stop_ppm) {
     
     int co2_sum = 10000;                                      // start high enough so the first measurement won't turn off the loop
     int co2_sum2 = 0;
     int co2_last = Co2(1);
     Serial1.print("\"data_raw\": [");
     Serial.print("\"data_raw\": [");
     
     timer0.begin(Co2_flag,freq*1000000);                                                                                                  // begin taking measurements
     while (co2_counter < stop_at_max && ((co2_sum > sum_stop_ppm) | (co2_sum < -1*sum_stop_ppm) | (co2_counter < stop_at_min))) {        // continue taking measurements while... counter is < max allowable measurements and > min allowable measurements and x measurements sum to <2ppm0
     Serial.print(co2_counter);
     Serial.print(",");  
     Serial.print(co2_counter%points_to_sum);
     Serial.print(",");  
     Serial.print(co2_counter < stop_at_max);
     Serial.print(co2_sum > sum_stop_ppm);
     Serial.print(co2_sum < -1*sum_stop_ppm);
     Serial.println(co2_counter < stop_at_min);
     Serial1.print(co2_counter);
     Serial1.print(",");  
     Serial1.print(co2_counter%points_to_sum);
     Serial1.print(",");  
     Serial1.print(co2_counter < stop_at_max);
     Serial1.print(co2_sum > sum_stop_ppm);
     Serial1.print(co2_sum < -1*sum_stop_ppm);
     Serial1.println(co2_counter < stop_at_min);
     while (co2_flag < 1) {
     }
     requestCo2(readCO2);  
     co2_value = getCo2(response);                                              // take measurement
     co2_sum2 += co2_value-co2_last;                                            // sum the difference between last value and current value
     co2_last = co2_value;                                                      // set last value to current value 
     co2_counter = co2_counter + 1;                                             // count up total measurements
     noInterrupts();
     co2_flag = 0;                                                              // reset co2 measurement flag
     interrupts();
     Serial.print(co2_value);
     Serial.print(",");
     Serial1.print(co2_value);
     Serial1.print(",");
     delay(1000);
     temp(1);
     relh(1);
     #ifdef DEBUGSIMPLE
     Serial.print(co2_sum2);
     Serial.print(",");   
     Serial.print(co2_sum);
     Serial.print(",");   
     Serial.print(co2_flag);
     Serial.print(",");
     Serial.print(co2_counter);
     #endif
     if (co2_counter%points_to_sum == points_to_sum-1) {
     co2_sum = co2_sum2;
     co2_sum2 = 0;
     }
     }
     #ifdef DEBUGSIMPLE
     Serial.print(co2_counter < stop_at_max);
     Serial.print(co2_sum > sum_stop_ppm);
     Serial.print(co2_sum < -1*sum_stop_ppm);
     Serial.println(co2_counter < stop_at_min); 
     #endif
     timer0.end();
     Serial.print("],"); 
     co2_flag = 0;
     co2_sum = 0;
     co2_counter = 0;
     Serial1.print("\"CO2_frequency\": ");
     Serial1.print(freq);
     Serial1.print(",");
     Serial.print("\"CO2_frequency\": ");
     Serial.print(freq);
     Serial.print(",");
  }
     */
}

void requestCo2(byte packet[]) {
  while(!Serial3.available()) { //keep sending request until we start to get a response
    Serial3.write(readCO2,7);
    delay(50);
  }
  int timeout=0;  //set a time out counter
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

void eeprom() {

  int joe = 395;
  char joe2 [10];
  itoa(joe, joe2, 10);

  int digs = numDigits(joe);
  for (i=0;i<digs;i++) {
    //    joe2[i] = (char) joe/pow(10,(i-1));
    //    joe = joe - joe2[i]*pow(10,(i-1));
    Serial1.println(joe2[i]);
  }
  Serial1.println("");
  Serial1.println(joe);
  //  joe2[5] = joe/100000;
  //  joe2[4] = joe/10000;
  // joe2[3] = joe/1000;
  // joe2[2] = joe/100;
  // joe2[1] = joe/10;
  Serial1.println(joe);
  Serial1.println(numDigits(joe));
  delay(50);
  joe = (char) joe;
  Serial1.println(joe);

  int jack;
  EEPROM.write(1,1);
  EEPROM.write(2,2);
  EEPROM.write(3,3);
  EEPROM.write(4,4);  
  jack = EEPROM.read(1);
  Serial1.println(jack);
  jack = EEPROM.read(2);
  Serial1.println(jack);
  jack = EEPROM.read(3);
  Serial1.println(jack);
  jack = EEPROM.read(4);
  Serial1.println(jack);
}

void save_eeprom(int saved_val, int loc) {                                                        // save the calibration value to the EEPROM, and print the value to USB and bluetooth Serial
  char str [10];
  itoa(saved_val, str, 10);
  for (i=0;i<10;i++) {
    EEPROM.write(loc+i,str[i]);
    char temp = EEPROM.read(loc+i);
  }
  Serial.print(((float) atoi(str) / ((cal_pulses-1)/4)));                                          // when converting called_val to a per-measurement value, we have to divide by 1 less than cal_pulses/4 because we skipped the first set of values to avoid artifacts
  Serial1.print(((float) atoi(str) / ((cal_pulses-1)/4)));                                          // when converting called_val to a per-measurement value, we have to divide by 1 less than cal_pulses/4 because we skipped the first set of values to avoid artifacts
  Serial.print(",");
  Serial1.print(",");
}

float call_eeprom(int loc) {                                                                       // call calibration values from the EEPROM, and print the values to USB Serial only
  char temp [10];
  float called_val;
  for (i=0;i<10;i++) {
    temp[i] = EEPROM.read(loc+i);
  }
  called_val = ((float) atoi(temp) / ((cal_pulses-1)/4));                                          // when converting called_val to a per-measurement value, we have to divide by 1 less than cal_pulses/4 because we skipped the first set of values to avoid artifacts
#ifdef DEBUGSIMPLE
  Serial.print(called_val);
  Serial.print(", ");
#endif
  return called_val;
}  

void recall_all() {
#ifdef DEBUGSIMPLE
  Serial.print("Calibration values called from EEPROM: HIGH, DETECTOR 1: ");
#endif
  cal1_high_d1 = call_eeprom(0);                                                     // save calibration data 
  meas1_high_d1 = call_eeprom(30);
  meas2_high_d1 = call_eeprom(60);
  act1_high_d1 = call_eeprom(90);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: LOW, DETECTOR 1: ");
#endif  
  cal1_low_d1 = call_eeprom(10);                                                      // save calibration data
  meas1_low_d1 = call_eeprom(40);
  meas2_low_d1 = call_eeprom(70);
  act1_low_d1 = call_eeprom(100);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: BLANK, DETECTOR 1: ");
#endif
  cal1_blank_d1 = call_eeprom(20);
  meas1_blank_d1 = call_eeprom(50);                                                    // save calibration data
  meas2_blank_d1 = call_eeprom(80);
  act1_blank_d1 = call_eeprom(110);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: HIGH, DETECTOR 2: ");
#endif
  cal1_high_d2 = call_eeprom(120);                                                 // save calibration data
  meas1_high_d2 = call_eeprom(150);
  meas2_high_d2 = call_eeprom(180);
  act1_high_d2 = call_eeprom(210);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: LOW, DETECTOR 2: ");
#endif
  cal1_low_d2 = call_eeprom(130);                                                    // save calibration data
  meas1_low_d2 = call_eeprom(160);
  meas2_low_d2 = call_eeprom(190);
  act1_low_d2 = call_eeprom(220);
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("Calibration values called from EEPROM: BLANK, DETECTOR 2: ");
#endif
  cal1_blank_d2 = call_eeprom(140);                                                 // save calibration data
  meas1_blank_d2 = call_eeprom(170);
  meas2_blank_d2 = call_eeprom(200);
  act1_blank_d2 = call_eeprom(230);
#ifdef DEBUGSIMPLE
#endif
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("TMP006 contactless temperature sensor calibration value from EEPROM: ");
#endif  
  //  tmp006_cal_S = call_eeprom(240)*((cal_pulses-1)/4);
}

void cal_baseline() {
  baseline_array [1] = (meas1_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(meas1_high_d1-meas1_low_d1));        // baseline for light 15 (measurement light)
  baseline_array [2] = (meas2_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(meas2_high_d1-meas2_low_d1));        // baseline for light 16 (measurement light)
  baseline_array [3] = (act1_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(act1_high_d1-act1_low_d1));           // baseline for light 20 (actinic)
}

int protocol_runtime(volatile int protocol_pulses[], volatile int protocol_pulsedistance[][2], volatile int protocol_total_cycles) {
  int total_time = 0;
  for (x=0;x<protocol_total_cycles;x++) {
    total_time += protocol_pulses[x]*(protocol_pulsedistance[x][0]+protocol_pulsedistance[x][1]);
  }
  return total_time;
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

int calc_Protocol() {
  int a = 0;
  int c = 0;
  for (i=0;i<3;i++){
    if (Serial1.available() == 0) {
      c = Serial.read()-48;
    }
    else {
      c = Serial1.read()-48;
    }
    a = (10 * a) + c;
  }
  return a;
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

