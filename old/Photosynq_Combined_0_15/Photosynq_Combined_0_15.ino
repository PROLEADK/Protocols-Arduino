// CHANGE fluorescence to fluorescence_phi2
// CHANGE dirk to ecs_dirk

/*

 so it's creating an array of 1500 (or printing it at least) for spad when it runs 1 or 5 averages (doesn't matter), but it seems to work fine for fluorescence and dirk - fix!!!
 --- then work on the calibrations... get them in line...
 --- should we make a new calibration for the LEDs themselves, based on reflectance from a known piece?  
 --- seriously clean up the debug statements
 --- add additional gain control - clean up old gain control... it's not really working write I don't think...
 Lessons learned from Mexico - 
 --- gain is chopping off too much from Fs - causing Fs to be too low

Must do:
- update the calibration so there's more time between runs... 
- make sure we're able to run a fluorescence transient
 
 Experiments:
 calibration consistency - change the temperature of the device and measure the same leaf over and over... what's the repeatability?  How much change due to temperature?
 
 
 
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

future improvements:
- auto gain
- make gain an array to adjust by each stuff, can we make anything else into an array?
- auto-light level for actinic
- make the calibrations more efficient (right now they both run 3 high, low, blank but that's not necessary.
 

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

[{"protocol_name":"baseline_sample","averages":1,"wait":0,"cal_true":2,"analog_averages":12,"pulsesize":25,"pulsedistance":3000,"actintensity1":1,"actintensity2":1,"measintensity":255,"calintensity":255,"pulses":[400],"detectors":[[34]],"measlights":[[14]]}
,{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]

[{"protocol_name":"fluorescence","baselines":[1,1,1,1],"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"act_light":20,"pulsesize":50,"pulsedistance":3000,"actintensity1":100,"actintensity2":100,"measintensity":3,"calintensity":255,"pulses":[50,50,50,50],"detectors":[[34],[34],[34],[34]],"measlights":[[15],[15],[15],[15]],"act":[2,1,2,2]}]



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
[{"protocol_name":"chlorophyll_spad_ndvi","baselines":[0,0,0,0],"measurements":1,"measurements_delay":1,"environmental":[["relative_humidity",1],["temperature",1],["light_intensity",1]],"averages":1,"wait":0,"cal_true":0,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,35,35,34]],"measlights":[[12,20,12,20]]}]
[{"protocol_name":"calibration_spad_ndvi","measurements":1,"baselines":[0,0,0,0],"measurements_delay":1000,"averages":3,"wait":10,"cal_true":1,"analog_averages":12,"pulsesize":6,"pulsedistance":3000,"actintensity1":5,"actintensity2":5,"measintensity":6,"calintensity":255,"pulses":[100],"detectors":[[34,34,34,34,35,35,35,35]],"measlights":[[14,15,16,20,14,15,16,20]]}]
 
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

#include <Time.h>                                                             // enable real time clock library
#include <Wire.h>
#include <EEPROM.h>
#include <DS1307RTC.h>                                                        // a basic DS1307 library that returns time as a time_t
#include <Adafruit_Sensor.h>
#include <Adafruit_TMP006.h>
#include <Adafruit_TCS34725.h>
#include <HTU21D.h>
#include <JsonParser.h>

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
#define MEASURINGLIGHT_PWM 23
#define CALIBRATINGLIGHT_PWM 9
#define ACTINICLIGHT_INTENSITY2 3
#define ACTINICLIGHT_INTENSITY1 4
#define ACTINICLIGHT_INTENSITY_SWITCH 5
#define DETECTOR1 34
#define DETECTOR2 35
#define SAMPLE_AND_HOLD 6

int _meas_light;															 // measuring light to be used during the interrupt
int act_state = 1;
int act_int_state = 0;
int analog_reads = 0;
int number_of_protocols = 0;
float baseline_array [4] = {
  0,0,0,0};                                                           // values are defined below in cal_baseline
int baseline_lights [4] = {
  0,15,16,20};                                                 // this is a lookup table for the measurement lights which are listed in 'array'.  Looking up 0 returns 0.
int averages = 0;

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
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

void setup() {
  Serial.begin(115200);                                                         // set baud rate for Serial, used for communication to computer via USB
  //  Serial.println("Serial works");
  Serial1.begin(115200);                                                        // set baud rate for Serial 1 used for bluetooth communication on pins 0,1
  //  Serial.println("Serial1 works");
  Serial3.begin(9600);                                                          // set baud rate for Serial 3 is used to communicate with the CO2 sensor
  //  Serial.println("Serial3 works");
  delay(500);
  tmp006.begin();                                                              // this opens wire.begin as well, and initializes tmp006, tcs light sensor, and htu21D.  by default, contactless leaf temp measurement takes 4 seconds to complete

  pinMode(MEASURINGLIGHT1, OUTPUT);                                             // set appropriate pins to output
  pinMode(MEASURINGLIGHT2, OUTPUT);
  pinMode(MEASURINGLIGHT3, OUTPUT);
  pinMode(MEASURINGLIGHT4, OUTPUT);
  pinMode(ACTINICLIGHT1, OUTPUT);
  pinMode(ACTINICLIGHT2, OUTPUT);
  pinMode(CALIBRATINGLIGHT1, OUTPUT);
  pinMode(CALIBRATINGLIGHT2, OUTPUT);
  pinMode(MEASURINGLIGHT_PWM, OUTPUT); 
  pinMode(ACTINICLIGHT_INTENSITY2, OUTPUT);
  pinMode(ACTINICLIGHT_INTENSITY1, OUTPUT);
  pinMode(ACTINICLIGHT_INTENSITY_SWITCH, OUTPUT);
  pinMode(CALIBRATINGLIGHT_PWM, OUTPUT);
  pinMode(SAMPLE_AND_HOLD, OUTPUT);
  pinMode(13, OUTPUT);
  analogReadAveraging(1);                                                       // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u) - this gets changed later by each protocol
  pinMode(DETECTOR1, EXTERNAL);                                                 // use external reference for the detectors
  pinMode(DETECTOR2, EXTERNAL);
  analogReadRes(ANALOGRESOLUTION);                                              // set at top of file, should be 16 bit
  analogresolutionvalue = pow(2,ANALOGRESOLUTION);                              // calculate the max analogread value of the resolution setting
  analogWriteFrequency(3, 187500);                                              // Pins 3 and 5 are each on timer 0 and 1, respectively.  This will automatically convert all other pwm pins to the same frequency.
  analogWriteFrequency(5, 187500);
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









//////////////////////// MAIN LOOP /////////////////////////
void loop() {

  delay(500);                                                                   // 
  int measurements = 1;                                                   // the number of times to repeat the entire measurement (all protocols)
  unsigned long measurements_delay = 0;                                    // number of milliseconds to wait between measurements
  volatile unsigned long meas_number = 0;                                       // counter to cycle through measurement lights 1 - 4 during the run

  // these variables could be pulled from the JSON... however, because pulling from JSON is slow, it's better to create a int to save them into at the beginning of a protocol run and use the int instead of the raw hashTable.getLong type call 
  int cycle = 0;                                                                // current cycle number (start counting at 0!)
  int pulse = 0;                                                                // current pulse number
  int total_cycles;	                       	                        	// Total number of cycles - note first cycle is cycle 0
  int alt1_state;
  int alt2_state;
  int red_state;
  int meas_array_size = 0;                                                      // measures the number of measurement lights in the current cycle (example: for meas_lights = [[15,15,16],[15],[16,16,20]], the meas_array_size's are [3,1,3].  
  String json2 [15];
  char* json = (char*)malloc(1);
  char w;
  char* name;
  JsonHashTable hashTable;
  JsonParser<250> root;
  number_of_protocols = 0;                                                      // reset number of protocols
  int end_flag = 0;
  long* data_raw_average = (long*)malloc(1);

// {firmware version: ......  sample: [[{data_raw:[3,4,2,3]},{data_raw:[...]...


  while (Serial.peek() != 91 && Serial1.peek() != 91) {                           // wait till we see a "[" to start the JSON of JSONS
    if (Serial.peek() == 57 | Serial1.peek() == 57) {                              // look for 9 for confirmation that device is working
      Serial.read();                                      // flush the '9'
      Serial1.read();                                      // flush the '9'
      Serial.println("MultispeQ Ready");
      Serial1.println("MultispeQ Ready");
    }
    else if (Serial.peek() == 56 | Serial1.peek() == 56) {                      // run light tests
      Serial.read();
      Serial1.read();
      lighttests();
    }
/*
    else if (Serial.peek() == 55) {
      Serial.read();
      actinic_calibration();
    }
*/
  }

Serial.read();                                       // flush the "["
Serial1.read();                                       // flush the "["

  int which_serial = 0;
  while (end_flag != 1) {                             // run this loop until we see the "]" to close the JSON of JSONs
  Serial.print(Serial.available());
  Serial.print(",  ");
  Serial.println(Serial1.available());
    while (Serial.peek()!=125 && Serial1.peek()!=125) {                      // wait until you receive a "curly brackets" indicating the close of a json
/*
      if (Serial.available()>0) {
        Serial.print("z");
        w = Serial.read();
        w = 0;
        json2[number_of_protocols] += w;
        which_serial = 1;
      }
*/
      if (Serial1.available()>0) {
        w = Serial1.read();
        json2[number_of_protocols] += w;
        which_serial = 2;
      }
    }
    switch (which_serial) {
      case 1:
        w = Serial.read();                                                  // catch the last "curly brackets"
        break;
      case 2:
        w = Serial1.read();
        break;
    }
    json2[number_of_protocols] += w;
    Serial.println(json2[number_of_protocols]);      
    number_of_protocols++;
#ifdef DEBUGSIMPLE                                                    // print out the incoming protocol JSONs
#endif
    if (Serial.peek() == 44 | Serial1.peek() == 44) {                          // if there's a comma, flush it out before starting the next JSON
      Serial.read();
      Serial1.read();
    }
    else if (Serial.peek() == 93 | Serial1.peek() == 93) {                      // if it's a ], that means that's the end of the string of JSONs, so send up the end_flag
      end_flag = 1;
    }
#ifdef DEBUGSIMPLE                                      // print the total number of protocols
    Serial.print("this is number_of_protocols     ");
    Serial.println(number_of_protocols);
#endif
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

      int cal_true =            hashTable.getLong("cal_true");                             // identify this as a calibration routine (0 = false, 1 = re-calibrate, 2 = calibrate to sample)
      String protocol_name =    hashTable.getString("protocol_name");
      averages =                hashTable.getLong("averages");                               // number of times to repeat this particular protocol
      int wait =                hashTable.getLong("wait");                                    // seconds wait time between 'averages'
      measurements =            hashTable.getLong("measurements");
      measurements_delay =      hashTable.getLong("measurements_delay");
      int analog_averages =     hashTable.getLong("analog_averages");                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
      int act_light =           hashTable.getLong("act_light");                               // any
      int alt1_light =          hashTable.getLong("alt1_light");                             // any
      int alt2_light =          hashTable.getLong("alt2_light");                               // any    
      int red_light =           hashTable.getLong("red_light");                                 // any
      if (hashTable.getLong("act_background_light") != 0) {
//        Serial.print("background light:  ");
//        Serial.println(hashTable.getLong("act_background_light"));
        digitalWriteFast(act_background_light,LOW);                                          // turn off actinic background from previous protocol
        act_background_light =hashTable.getLong("act_background_light");                      // change to new background actinic light
      }
      int pulsesize =           hashTable.getLong("pulsesize");                         // also acts as gain control setting - shorter pulse, small signal.  Longer pulse, larger signal
      int pulsedistance =       hashTable.getLong("pulsedistance");                       // distance between pulses
      int actintensity1 =       hashTable.getLong("actintensity1");                         // intensity at LOW setting below
      int actintensity2 =       hashTable.getLong("actintensity2");                        // intensity at HIGH setting below
      int measintensity =       hashTable.getLong("measintensity");                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
      int calintensity =        hashTable.getLong("calintensity");                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
      JsonArray pulses =        hashTable.getArray("pulses");                     
      JsonArray act =           hashTable.getArray("act");
      JsonArray red =           hashTable.getArray("red");
      JsonArray alt1 =          hashTable.getArray("alt1");
      JsonArray alt2 =          hashTable.getArray("alt2");
      JsonArray detectors =     hashTable.getArray("detectors");
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

      Serial1.print("{\"protocol_number\": ");
      Serial.print("{\"protocol_number\": ");
      Serial1.print(protocol_number);
      Serial.print(protocol_number);
      Serial1.print(",\"protocol_name\": \"");
      Serial.print(",\"protocol_name\": \"");
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

      analogReadAveraging(analog_averages);                                      // set analog averaging (ie ADC takes one signal per ~3u)
      analogWrite(ACTINICLIGHT_INTENSITY1, actintensity1);                      // set intensities for each of the lights - this is high (saturating)
      analogWrite(ACTINICLIGHT_INTENSITY2, actintensity2);                      // this is low (actinic)
      digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                     // preset the switch to the actinic (low) preset position
      analogWrite(MEASURINGLIGHT_PWM, measintensity);
      analogWrite(CALIBRATINGLIGHT_PWM, calintensity);
      delay(5);                                                                 // give time for the lights to stabilize

//      Light_Intensity(3);                                                       // run light intensity, but don't save the data as averages
      

      digitalWriteFast(act_background_light, HIGH);                                    // turn on the actinic background light to the low preset position and keep it on until the end of the measurement

      for (x=0;x<averages;x++) {                                                       // Repeat the protocol this many times     
      
        /*
    options for relative humidity, temperature, contactless temperature. light_intensity,co2
         0 - off
         1 - single measurement
         2 - calibration (calibration only for contactless temperature, light intensity, and co2
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
            Relative_Humidity((int) environmental.getArray(i).getLong(1));                        // if this string is in the JSON and the 3rd component in the array is == 0 (meaning they want this measurement taken prior to the spectroscopic measurement), then call the associated measurement (and so on for all if statements in this for loop)
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "temperature") {
            Temperature((int) environmental.getArray(i).getLong(1));
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "contactless_temperature") {
            Contactless_Temperature( environmental.getArray(i).getLong(1));
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "co2") {
            Co2( environmental.getArray(i).getLong(1));
          }
          if (environmental.getArray(i).getLong(1) == 0 \
        && (String) environmental.getArray(i).getString(0) == "light_intensity") {
            Light_Intensity( environmental.getArray(i).getLong(1));
          }
        }
        for (z=0;z<size_of_data_raw;z++) {                                                                    // cycle through all of the pulses from all cycles
          if (cycle == 0 && pulse == 0) {                                                                // if it's the beginning of a measurement, then...                                                             // wait a few milliseconds so that the actinic pulse presets can stabilize
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
          if (pulse == 0) {                                                                // if it's the first pulse of a cycle, then change sat, act, far red, alt1 and alt2 values as per array's set at beginning of the file
            if (act.getLong(cycle) == 2) {
              act_state = 0;
            }
            else {
              act_int_state = act.getLong(cycle);
              act_state = 1;
            }
          }
          alt1_state = alt1.getLong(cycle);
          alt2_state = alt2.getLong(cycle);
          red_state = red.getLong(cycle);
          digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH,act_int_state);

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
          Serial.println(red_state);
#endif
          while (on == 0 | off == 0) {                                        	     // if ALL pulses happened, then...
          }
          data1 = analogRead(detector);                                              // save the detector reading as data1, and subtract of the baseline (if there is no baseline then baseline is automatically set = 0)     
#ifdef DEBUGSIMPLE
          Serial.print(baseline);
          Serial.print(", ");
          Serial.println(data1);
#endif
          digitalWriteFast(SAMPLE_AND_HOLD, HIGH);																// turn on measuring light and/or actinic lights etc., tick counter
          digitalWriteFast(act_light, act_state);                                               // turn on actinic and other lights
          digitalWriteFast(red_light, red_state);
          digitalWriteFast(alt1_light, alt1_state);    
          digitalWriteFast(alt2_light, alt2_state);

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
        digitalWriteFast(act_light, LOW);                                              // make sure all lights are turned off
        digitalWriteFast(red_light, LOW);
        digitalWriteFast(alt1_light, LOW);
        digitalWriteFast(alt2_light, LOW);
        digitalWriteFast(act_background_light, HIGH);                                  // EXCEPT keep on the actinic background light between all protocols and measurements
        timer0.end();                                                                  // if it's the last cycle and last pulse, then... stop the timers
        timer1.end();
        cycle = 0;                                                                     // ...and reset counters
        pulse = 0;
        on = 0;
        off = 0;
        meas_number = 0;

        /*
    options for relative humidity, temperature, contactless temperature. light_intensity,co2
         0 - off
         1 - single measurement
         2 - calibration (calibration only for contactless temperature, light intensity, and co2
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
      averages = 0;   			                                        // number of times to repeat the entire run 
      wait = 0;                  	                                                // seconds wait time between averages
      analog_averages = 0;                                                             // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
      act_light = 0;
      red_light = 0;
      alt1_light = 0;
      alt2_light = 0;
      detector = 0;
      pulsesize = 0;                                                                // measured in microseconds
      pulsedistance = 0;
      actintensity1 = 0;                                                            // intensity at LOW setting below
      actintensity2 = 0;                                                            // intensity at HIGH setting below
      measintensity = 0;                                                            // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
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
      act_background_light = 13;

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
      delay(measurements_delay);                                                   // delay between measurements
    }
  }
  Serial.println("]}");
  Serial.println("");
  Serial1.println("]}");
  Serial1.println("");
  digitalWriteFast(act_background_light, LOW);                                    // turn off the actinic background light at the end of all measurements
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

void lighttests() {
  while (Serial.available()>0 | Serial1.available()>0) {                                                            // flush incoming serial
    Serial.read();
    Serial1.read();
  } 
  int choose = 0;
  analogWrite(ACTINICLIGHT_INTENSITY2, 255);
  analogWrite(ACTINICLIGHT_INTENSITY1, 255);
  analogWrite(CALIBRATINGLIGHT_PWM, 255);
  analogWrite(MEASURINGLIGHT_PWM, 255);
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
    Serial1.println("or enter 999 to exit");  

    while (Serial.available()<3 && Serial1.available()<3) {
    }
    choose = calc_Protocol();
    Serial.println(choose);
    Serial1.println(choose);

    if (choose<30) {
      Serial.println("First actinic intensty switch high, then actinic intensity switch low");
      Serial1.println("First actinic intensty switch high, then actinic intensity switch low");
      delay(1000);
      for (y=0;y<2;y++) {
        for (x=0;x<256;x++) {
          Serial.println(x);
          Serial1.println(x);
          analogWrite(MEASURINGLIGHT_PWM, x);
          analogWrite(CALIBRATINGLIGHT_PWM, x);
          analogWrite(ACTINICLIGHT_INTENSITY1, x);
          analogWrite(ACTINICLIGHT_INTENSITY2, x);
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
        for (x=256;x>0;x--) {
          Serial.println(x);
          Serial1.println(x);
          analogWrite(MEASURINGLIGHT_PWM, x);
          analogWrite(CALIBRATINGLIGHT_PWM, x);
          analogWrite(ACTINICLIGHT_INTENSITY1, x);
          analogWrite(ACTINICLIGHT_INTENSITY2, x);
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
    // Based on Adafruit's example code 'TCS34725', added averaging of 3 measurements
    uint16_t r, g, b, c, colorTemp, colorTemp1, colorTemp2, colorTemp3, lux, lux1, lux2, lux3;
    tcs.getRawData(&r, &g, &b, &c);
    colorTemp1 = tcs.calculateColorTemperature(r, g, b);
    lux1 = tcs.calculateLux(r, g, b);
    tcs.getRawData(&r, &g, &b, &c);
    colorTemp2 = tcs.calculateColorTemperature(r, g, b);
    lux2 = tcs.calculateLux(r, g, b);
    tcs.getRawData(&r, &g, &b, &c);
    colorTemp3 = tcs.calculateColorTemperature(r, g, b);
    lux3 = tcs.calculateLux(r, g, b);

    lux = (lux1+lux2+lux3)/3;
    colorTemp = (colorTemp1+colorTemp2+colorTemp3)/3;

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
     */
  }
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

