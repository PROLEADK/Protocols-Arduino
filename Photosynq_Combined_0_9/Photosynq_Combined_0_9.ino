// NEXT STEPS //
// -- give to chris to double check, give feedback
// -- rewrite description and the whole top part to reflect most recent changes
// -- 

/////////////////////DESCRIPTION//////////////////////
/*
 This file is used to measure pulse modulated fluorescence (PMF) using a saturating pulse and a measuring pulse.  The measuring pulse LED (for example, Rebel Luxeon Orange) can itself produce some infra-red
 fluorescence which is detected by the detector.  This signal causes the detector response to be higher than expected which causes bias in the results.  So, in order to account for this added infra-red, we 
 perform a calibration.  The calibration is a short set of pulses which occur at the beginning of the program from an infra-red LED (810nm).  The detector response from these flashes indicates how "reflective"
 the sample is to infra-red light in the range produced by our measuring LED.  We can create a calibration curve by measuring a very reflective sample (like tin foil) and a very non-reflective sample (like
 black electrical tape) with both the calibrating LED and the measuring LED.  This curve tells us when we see detector response X for the calibrating LED, that correspond to a baseline response of Y for the
 measuring LED.  Once we have this calibration curve, when we measure an actual sample (like a plant leaf) we automatically calculate and remove the reflected IR produced by the measuring pulse.  Vioala!
 
 The LED on/off cycles are taken care of by using interval timers.  One timer turns the measuring pulse on (and takes a measurement using analogRead()), one timer turns it off, and one timer controls the length 
 of the whole run.  
 
 Using Arduino 1.0.3 w/ Teensyduino installed downloaded from http://www.pjrc.com/teensy/td_download.html .   
 */

// NOTES FOR USER
// When changing protocols, makes sure to change the name of the saved file (ie data-I.csv) so you know what it is.
// Max file name size is 12 TOTAL characters (8 characters + .csv extension).  Program does not distinguish between upper and lower case
// Files have a basename (like ALGAE), and then for each subroutine an extension is added (like ALGAE-I) to indicate from which subroutine the data came.  
// When you create a new experiment, you save the data (ALGAE-I) in a folder.  If you want stop and restart but save data to the same file, you may append that file.
// Otherwise, you can create a new data file (like ALGAE-I) file an a different folder, which will be named 01GAE, 02GAE, 03GAE and so on.
// Calibration is performed to create an accurate reflectance in the sample using a 850nm LED.  You can find a full explanation of the calibration at https://opendesignengine.net/documents/14
// A new calibration should be performed when sample conditions have changed (a new sample is used, the same sample is used in a different position)
// The previous calibration value is saved in the SD card, so if no calibration is performed the most recently saved calibration value will be used!
// See KEY VARIABLES USED IN THE PROTOCOL below to change the measurement pulse cycles, pulse lengths, saturation pulses, etc.
// Note that the output values minimum and maximum are dependent on the analog resolution.  From them, you can calculate the actual current through the detector.
// ... So - at 10 bit resolution, min = 0, max = 10^2 = 1023; 16 bit resolution, min = 0, max = 16^2 = 65535; etc.
// ... To calculate actual voltage on the analog pin: 3.3*((measured value) / (max value based on analog resolution)).
// If you want to dig in and change values in this file, search for "NOTE!".  These areas provide key information that you will need to make changes.
// Pin A10 and A11 are 16 bit enabed with some added coding in C - the other pins cannot achieve 16 bit resolution.
// Real Time clock - includes sync to real time clock in ASCII 10 digit format and printed time with each sample (ie T144334434)
// 999 runs a calibration run.  User must have the calibration stick available to run it.
// 998 runs a baseline calculation.  This measures the sample reflectivity at 940nm and calculatees a baseline value from that.  This is used when measuring IR fluorescence and removes the effect of the IR produced by the LEDs themselves
// 000 allows the user to test the lights and detectors, one by one.

// HARDWARE NOTES
/*
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

NOTE ON POWER: USB power alone is not sufficient!  You must also have an additional power supply which can quickly supply power, like a 9V lithium ion battery.  Power supply may be from 5V - 24V
OVERALL: Excellent results - definitely good enough for the spectroscopic measurements we plant to make (absorbance (ECS), transmittance, fluorescence (PMF), etc.)

 */

// SPECS USING THIS METHOD: 
// Timing of Measuring pulse and Saturation pulse is within 500ns.  Peak to Peak variability, ON and OFF length variability all is <1us.  Total measurement variability = 0 to +2us (regardless of how many cycles)

// DATASHEETS
// CO2 sensor hardware http://CO2meters.com/Documentation/Datasheets/DS-S8-PSP1081.pdf
// CO2 sensor communication http://CO2meters.com/Documentation/Datasheets/DS-S8-Modbus-rev_P01_1_00.pdf

// ISSUES:
// separate addressing - 0x40 on tmp006 and htu... get them on different addresses.  tmp006
// can be put on sep address by changing adr0 and adr1, but preferbly we want to keep them
// on ground.
// tmp006, wire, and tcs all have to happen at the same time, adjust addresses and decide how to use the tcs / tmp libraries if possible
// Figure out low power mode

// FEATURES: 
// For calibration, removes the first set of measured values for all measuring lights (meas1 - meas4) when calculating the 'high' and 'low' calibration values to avoid any artifacts
// When taking measurements of a sample to calculate the baseline, the measured values are not printed to the Serial ports and the output is JSON compatible
// When performing a calibration, the user is given 10 seconds to flip from shiny to dull side (always start with shiny side).  The wait time between sides can be adjusted.

// TO DO:
// Turn tihs whole things into a library, to simplify the code
// Covert the calibration code into the new format
// Enable multiple (10) pulses and a code to define which pulse kicks when
// Move as many global variables as possible to local variables

// THINGS I'D LIKE TO HAVE:
// More timers!  If there were more timers, or more options for the ISR, like skipping an interrupts, or changing the timer on the fly, that'd be awesome
// CLEAN MORE... maybe make more measuring lights options (more than just 4).. 
// Use Vector instead of array (how to import, tried ProgrammingCplusplus library but it didn't work with Teensy.
// how to enable and disable serial inside the sketch... so far it seems once I disable it, it's gone for good (re-enabling doesn't work)
// a universal baseline generator - it runs every light at every intensity on every detector and creates arrays for the outputted values.  Then, if you want to subtract a baseline
// or make some other adjustment, the data is all stored there.

//#define DEBUG 1  // uncomment to add full debug features
//#define DEBUGSIMPLE 1  // uncomment to add partial debug features

#include <Time.h>   // enable real time clock library
#include <Wire.h>
#include <EEPROM.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <Adafruit_Sensor.h>
#include <Adafruit_TMP006.h>
#include <Adafruit_TCS34725.h>

//////////////////////PIN DEFINITIONS AND TEENSY SETTINGS////////////////////////
int analogresolution = 16; // Set the resolution of the analog to digital converter (max 16 bit, 13 bit usable)  
int measuringlight1 = 15; // Teensy pin for measuring light
int measuringlight2 = 16; // Teensy pin for measuring light
int measuringlight3 = 11; // Teensy pin for measuring light
int measuringlight4 = 12  ; // Teensy pin for measuring light
int actiniclight1 = 20;
int actiniclight2 = 2;
int calibratinglight1 = 14;
int calibratinglight2 = 10;
int measuringlight_pwm = 23;
int calibratinglight_pwm = 9;
int actiniclight_intensity2 = 3;
int actiniclight_intensity1 = 4;
int actiniclight_intensity_switch = 5;
int detector1 = A10; // Teensy analog pin for detector
int detector2 = A11; // Teensy analog pin for detector

//////////////////////Shared Variables///////////////////////////
volatile int data1=0, off = 0, on = 0;
volatile unsigned long meas_number = 0; // counter for measurement lights 1 - 4
int analogresolutionvalue;
int i=0,j=0, k=0,z=0,y=0,q=0,x=0,p=0; // Used as a counters
int protocol_value, protocols;
char* protocolversion = "001"; // Current long term support version of this file
const char* variable1name = "Fs"; // Fs
const char* variable2name = "Fm"; // Fm (peak value during saturation)
const char* variable3name = "Fd"; // Fd (value when actinic light is off
const char* variable4name = "Fv"; // == Fs-Fo
const char* variable5name = "Phi2"; // == Fv/Fm
const char* variable6name = "1/Fs-1/Fd"; // == 1/Fs-1/Fd
unsigned long starttimer0, starttimer1, starttimer2;
float Fs;
float Fd;
float Fm;
float Phi2;
float invFsinvFd;
int cycle = 0; // current cycle number (start counting at 0!)
int pulse = 0; // current pulse number
IntervalTimer timer0, timer1, timer2;

///////////////////////Calibration Variables////////////////////////// 
float measuringlight1_baseline = 0, measuringlight2_baseline = 0, actiniclight1_baseline = 0, baseline = 0, baseline_flag = 0;
unsigned long cal1sum_high_d1 = 0;                                   // 'high' is value with reflective surface, 'low' is value with non-reflective surface, 'blank' is value with nothing in front of detector, d1 is detector 1 and d2 is detector 2
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
unsigned long data1_sum = 0;

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
int cal_wait = 15;                                                      // wait time during calibration for users to switch the calibration piece between repeats

//////////////////// USER MODIFIABLE VARIABLES ////////////////////////////
String protocol_name;                                                    // protocol name printed to JSON and stored in online database
int repeats;   			                                         // number of times to repeat the entire run 
int wait;                  	                                         // seconds wait time between repeats
int averages;                                                            // number of runs to average
int measurements;                                                        // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
int meas1_light;                                                         // light associated with the 1st measurement pulse (for example, actiniclight1, measuringlight1, calibratinglight2, etc.)
float meas1_baseline = 0;                                                // set the baseline to subtract from each measured value for meas1_light
int meas2_light;                                                         // light associated with the 2nd measurement pulse (for example, actiniclight1, measuringlight1, calibratinglight2, etc.) 
float meas2_baseline = 0;                                                // set the baseline to subtract from each measured value for meas2_light 
int meas3_light;                                                         // light associated with the 3rd measurement pulse (for example, actiniclight1, measuringlight1, calibratinglight2, etc.) 
float meas3_baseline = 0;                                                // set the baseline to subtract from each measured value for meas3_light
int meas4_light;                                                         // light associated with the 4th measurement pulse (for example, actiniclight1, measuringlight1, calibratinglight2, etc.)
float meas4_baseline = 0;                                                // set the baseline to subtract from each measured value for meas4_light 
int act_light;
int red_light;
int alt1_light;
int alt2_light;
int detector;
int pulsesize;                                                          // length of measuring pulses in us, must be <100us
int pulsedistance;                                                      // distance between pulses (min 1000us or 3000us with DEBUGSIMPLE on)
int actintensity1;                                                      // intensity at LOW setting below
int actintensity2;                                                      // intensity at HIGH setting below
int measintensity;                                                      // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
int calintensity;                                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
int pulses [10] = {0,0,0,0,0,0,0,0,0,0};            	                // array with the number of pulses in each cycle
int measlights [10][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};       // for each cycle you may select one of the 4 measuring lights specified above (meas1 - meas4).  For example, for cycle 1 if you want to alternative between meas2 and meas4 then set the values at {2,4,2,4}
int act [10] = {2,2,2,2,2,2,2,2,2,2};                                   // 2 is off.  "LOW" is actintensity1, "HIGH" is actintensity2.  May use this for combined actinic / saturation pulses  .  NOTE! You may set the intensity at any value during the run even if it's not preset - however, the LED intensity rise time is a few milliseconds.  The rise time on the preset values is in the nanoseconds range.
int alt1 [10] = {LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};              // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
int alt2 [10] = {LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};              // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
int red [10] = {LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};               // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
int total_cycles;	                                                // Total number of cycles - note first cycle is cycle 0
int cal_true = 0;                                                       // identify this as a calibration routine (0 = false, 1 = re-calibrate, 2 = calibrate to sample)
int cal_pulses = 400;                                                   // number of total pulses in the calibration routine

//////////////////////HTU21D Temp/Humidity variables///////////////////////////
#define temphumid_address 0x40                                           // HTU21d Temp/hum I2C sensor address
int sck = 19;                                                            // clock pin
int sda = 18;                                                            // data pin
int wait2 = 200;                                                         // typical delay to let device finish working before requesting the data
unsigned int tempval;
unsigned int rhval;
float temperature;
float rh;

//////////////////////S8 CO2 Variables///////////////////////////
byte readCO2[] = {
  0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};                             //Command packet to read CO2 (see app note)
byte response[] = {
  0,0,0,0,0,0,0};                                                        //create an array to store CO2 response
float valMultiplier = 0.1;
int CO2calibration = 17;                                                 // manual CO2 calibration pin (CO2 sensor has auto-calibration, so manual calibration is only necessary when you don't want to wait for autocalibration to occur - see datasheet for details 
unsigned long valCO2;

////////////////////TMP006 variables - address is 1000010 (adr0 on SDA, adr1 on GND)//////////////////////
Adafruit_TMP006 tmp006;

//////////////////////TCS34725 variables/////////////////////////////////
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

void setup() {
  delay(3000);
  Serial.begin(115200);                                                         // set baud rate for Serial, used for communication to computer via USB
  Serial.println("Serial works");
  Serial1.begin(115200);                                                        // set baud rate for Serial 1 used for bluetooth communication on pins 0,1
  Serial.println("Serial1 works");
  Serial3.begin(9600);                                                          // set baud rate for Serial 3 is used to communicate with the CO2 sensor
  Serial.println("Serial3 works");
  Wire.begin();                                                                 // turn on I2C
  Serial.println("Wire works");
  // TCS and tmp006 require additional work to get them to work with the other wire libraries
  //  tcs.begin();
  //  Serial.println("TCS works");
  //  tmp006.begin();
  //  Serial.println("tmp works");
  //  if (! tmp006.begin()) {
  //    Serial.println("No IR temperature sensor found (TMP006)");
  //    }

  pinMode(measuringlight1, OUTPUT);                                             // set appropriate pins to output
  pinMode(measuringlight2, OUTPUT);
  pinMode(measuringlight3, OUTPUT);
  pinMode(measuringlight4, OUTPUT);
  pinMode(actiniclight1, OUTPUT);
  pinMode(actiniclight2, OUTPUT);
  pinMode(calibratinglight1, OUTPUT);
  pinMode(calibratinglight2, OUTPUT);
  pinMode(measuringlight_pwm, OUTPUT); 
  pinMode(actiniclight_intensity2, OUTPUT);
  pinMode(actiniclight_intensity1, OUTPUT);
  pinMode(actiniclight_intensity_switch, OUTPUT);
  pinMode(calibratinglight_pwm, OUTPUT);
  analogReadAveraging(1);                                                       // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u) - this gets changed later by each protocol
  pinMode(detector1, EXTERNAL);                                                 // use external reference for the detectors
  pinMode(detector2, EXTERNAL);
  analogReadRes(analogresolution);                                              // set at top of file, should be 16 bit
  analogresolutionvalue = pow(2,analogresolution);                              // calculate the max analogread value of the resolution setting
  Serial.println("All LEDs and Detectors are powered up!");
  analogWriteFrequency(3, 187500);                                              // Pins 3 and 5 are each on timer 0 and 1, respectively.  This will automatically convert all other pwm pins to the same frequency.
  analogWriteFrequency(5, 187500);
}

void loop() {

  if (protocol_value != 998) {                                                  // don't print this if it's a baseline protocol
    Serial.println();
    Serial.println("Please select a 3 digit protocol code to begin");
    Serial.println("(000 for light testing)");
    Serial.println("(001 for DIRKF / PMF, RGB light, and CO2 measurement)");  
    Serial.println("");
  }
  while (Serial1.available()<3 && Serial.available()<3) {
  }
  protocol_value = calc_Protocol();                    // Retreive the 3 digit protocol code 000 - 999
#ifdef DEBUG
  Serial.print("you selected protocol number: ");
  Serial.println(protocol_value);
#endif

  recall_all();                                        // recall and save the data from the EEPROM
  if (baseline_flag == 1) {                            // calculate baseline values from saved EEPROM data only if the previous run was a sample baseline run (otherwise leave baseline == 0)
    cal_baseline();
  }

  switch(protocol_value) {
    
  case 0:                                             //////////////////// LIGHT TESTING ///////////////////////
    lighttests();
    break;

  case 1:                                             //////////////////// FLUORESCENCE ////////////////////////
    protocol_name =       "fluorescence";
    repeats =             1;                          // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                          // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight1;            // orange
    meas1_baseline =      measuringlight1_baseline;   // set the baseline to subtract from each measured value 
    act_light =           actiniclight1;              // any
    detector =            detector1;
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       10000;                      // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       255;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           50;
    pulses[1] =           50; 
    pulses[2] =           50; 
    pulses[3] =           50;
    measlights [0][0] =     1;
    measlights [0][1] =     1;
    measlights [0][2] =     1;
    measlights [0][3] =     1; 
    measlights [1][0] =     1; 
    measlights [1][1] =     1;
    measlights [1][2] =     1; 
    measlights [1][3] =     1;
    measlights [2][0] =     1;
    measlights [2][1] =     1; 
    measlights [2][2] =     1;
    measlights [2][3] =     1;
    measlights [3][0] =     1;
    measlights [3][1] =     1;
    measlights [3][2] =     1; 
    measlights [3][3] =     1;
    act [1] =               HIGH;   
    total_cycles =          sizeof(pulses)/sizeof(pulses[0])-1;      // (start counting at 0!)
    break;

  case 2:                                    //////////////////// DIRK ////////////////////////
    protocol_name =       "dirk";
    repeats =             1;                    // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                   // seconds wait time between repeats
    averages =            1;                    // number of runs to average
    measurements =        3;                    // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight2;     // 520 cyan
    meas1_baseline =      measuringlight2_baseline;
    act_light =           actiniclight1;       // any
    detector =            detector2;
    pulsesize =           20;                  // us, length of pulse must be <100us
    pulsedistance =       3000;                // distance between pulses
    actintensity1 =       50;                  // intensity at LOW setting below
    actintensity2 =       255;                 // intensity at HIGH setting below
    measintensity =       255;                 // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                 // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           100;
    pulses[1] =           100;
    pulses[2] =           100;
    measlights [0][0] =   1; 
    measlights [0][1] =   1;
    measlights [0][2] =   1;
    measlights [0][3] =   1; 
    measlights [1][0] =   1; 
    measlights [1][1] =   1;
    measlights [1][2] =   1; 
    measlights [1][3] =   1;
    measlights [2][0] =   1; 
    measlights [2][1] =   1;
    measlights [2][2] =   1;
    measlights [2][3] =   1; 
    act [0] =             HIGH;
    act [2] =             HIGH;  
    total_cycles =        sizeof(pulses)/sizeof(pulses[0])-1;        // (start counting at 0!)
    break;

  case 3:                                              //////////////////// SPAD ////////////////////////  NOTE not currently adjusted for fluorescence due to the 650 flash - this would require an additional calibration but it's ok, the impact is very small and consistent in the same direction
    protocol_name =       "spad";
    repeats =             1;                           // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                           // seconds wait time between repeats
    averages =            1;                           // number of runs to average
    measurements =        10;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         calibratinglight1;           // 940
    meas1_baseline =      cal1_high_d2;                // saved calibration value for 940
    meas2_light =         actiniclight1;               // 650
    meas2_baseline =      act1_high_d2;                // saved calibration value for 650
    detector =            detector2;
    pulsesize =           75;                         // us, length of pulse must be <100us
    pulsedistance =       3000;                       // distance between pulses (min 1000us or 3000us with DEBUGSIMPLE on)
    actintensity1 =       1;                          // intensity at LOW setting below
    actintensity2 =       1;                          // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           500; 
    measlights [0][0] =   1;
    measlights [0][1] =   2; 
    measlights [0][2] =   1;
    measlights [0][3] =   2;
    total_cycles =        sizeof(pulses)/sizeof(pulses[0])-1;// (start counting at 0!)
    break;

  case 4:                                             //////////////////////// 810 dirk ////////////////////////////
    protocol_name =       "810_dirk";
    repeats =             1;                         // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                1;                         // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight3; // 850
    act_light =           actiniclight2;   // any
    detector =            detector1;
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       3000;                       // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       255;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           100;                        // 
    pulses[1] =           100;                        // 
    pulses[2] =           100;                        // 
    measlights [0][0] =     1;
    measlights [0][1] =     1; 
    measlights [0][2] =     1; 
    measlights [0][3] =     1;
    measlights [1][0] =     1;
    measlights [1][1] =     1;  
    measlights [1][2] =     1; 
    measlights [1][3] =     1; 
    measlights [2][0] =     1; 
    measlights [2][1] =     1;  
    measlights [2][2] =     1; 
    measlights [2][3] =     1;    
    act [0] =               HIGH; 
    act [2] =               HIGH;
    total_cycles =          sizeof(pulses)/sizeof(pulses[0])-1;        // (start counting at 0!)
    break;
  
  case 5:                                             ///////////////////// 940 dirk ///////////////////////////
    protocol_name =       "940_dirk";
    repeats =             1;                          // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                          // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight4;            // 940
    act_light =           actiniclight2;              // any
    detector =            detector1;
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       3000;                       // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       255;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           100;                        // 
    pulses[1] =           100;                        // 
    pulses[2] =           100;                        // 
    measlights [0][0] =     1; 
    measlights [0][1] =     1; 
    measlights [0][2] =     1; 
    measlights [0][3] =     1; 
    measlights [1][0] =     1; 
    measlights [1][1] =     1; 
    measlights [1][2] =     1; 
    measlights [1][3] =     1; 
    measlights [2][0] =     1; 
    measlights [2][1] =     1; 
    measlights [2][2] =     1;
    measlights [2][3] =     1;
    act [1] =               HIGH;
    total_cycles =          sizeof(pulses)/sizeof(pulses[0])-1;        // (start counting at 0!)
    break;

  case 6:                                             //////////////////// 810 saturation /////////////////////    
    protocol_name =       "810_saturation";
    repeats =             1;                          // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                          // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight3;            // 850
    meas2_light =         measuringlight4;            // 940 
    act_light =           actiniclight2;              // any
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector1;
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       3000;                       // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       255;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           100;                        // 
    pulses[1] =           100;                        //
    pulses[2] =           500;                        //
    pulses[3] =           500;                        //
    pulses[4] =           200;                        //
    pulses[5] =           100;                        //
    measlights [0][0] =     1;
    measlights [0][1] =     2; 
    measlights [0][2] =     1;
    measlights [0][3] =     2; 
    measlights [1][0] =     1; 
    measlights [1][1] =     2; 
    measlights [1][2] =     1; 
    measlights [1][3] =     2;
    measlights [2][0] =     1;
    measlights [2][1] =     2; 
    measlights [2][2] =     1;
    measlights [2][3] =     2;  
    measlights [3][0] =     1;
    measlights [3][1] =     2; 
    measlights [3][2] =     1;
    measlights [3][3] =     2; 
    measlights [4][0] =     1; 
    measlights [4][1] =     2;  
    measlights [4][2] =     1;
    measlights [4][3] =     2;  
    measlights [5][0] =     1;
    measlights [5][1] =     2; 
    measlights [5][2] =     1;
    measlights [5][3] =     2;
    act [0] =               LOW; 
    act [1] =               HIGH;  
    act [4] =               HIGH;
    act [5] =               LOW;
    total_cycles =          sizeof(pulses)/sizeof(pulses[0])-1;        // (start counting at 0!)
    break;
  case 999:                                              // CALIBRATION // easy way to check to see if this is working correctly - set pulses[0] = 8, then manually see if the averages for each pulse are equal to the saved values for low and high.
    protocol_name =       "calibration";
    cal_true =            1;                             // identify this as a calibration routine (0 = false, 1 = re-calibrate, 2 = calibrate to sample)
    repeats =             6;                             // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                10;                            // seconds wait time between repeats
    averages =            1;                             // number of runs to average
    measurements =        12;                            // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         calibratinglight1;             // 940
    meas2_light =         measuringlight1;               // amber
    meas3_light =         measuringlight2;               // cyan
    meas4_light =         actiniclight1;                 // 650
    act_light =           actiniclight2;   
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector1;
    pulsesize =           75;                             
    pulsedistance =       3000;               
    actintensity1 =       1;                                         // leave at 1 - used for calibration of SPAD measurement
    actintensity2 =       1;                                         // leave at 1 - used for calibration of SPAD measurement
    measintensity =       255;      
    calintensity =        255;          
    pulses[0] =           cal_pulses;                                // cal_pulses must be a global variable, so please set this at the top of the file
    measlights [0][0] =   1;
    measlights [0][1] =   2;
    measlights [0][2] =   3;
    measlights [0][3] =   4;
    total_cycles =        sizeof(pulses)/sizeof(pulses[0])-1;        // (start counting at 0!)
    break;

  case 998:                                                          ///////////////////// BASELINE CALCULATION /////////////////////
    protocol_name =       "baseline_sample";
    cal_true =            2;                                         // identify this as a calibration routine (0 = false, 1 = re-calibrate, 2 = calibrate to sample)
    repeats =             1;                                         // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                                         // seconds wait time between repeats
    averages =            1;                                         // number of runs to average
    measurements =        12;                                        // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         calibratinglight1;                         // 850
    detector =            detector1;
    pulsesize =           75;                                        // us, length of pulse must be <100us
    pulsedistance =       3000;                                      // us, distance between pulses
    actintensity1 =       1;                                        // intensity at LOW setting below
    actintensity2 =       1;                                        // intensity at HIGH setting below
    measintensity =       255;                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           cal_pulses;                                // cal_pulses must be a global variable, so please set this at the top of the file
    measlights [0][0] =   1;
    measlights [0][1] =   1;
    measlights [0][2] =   1; 
    measlights [0][3] =   1; 
    total_cycles =        sizeof(pulses)/sizeof(pulses[0])-1;        // (start counting at 0!)
    break;
    
  case 10:
    leaftemp();
    break;
  case 11: 
    lightmeter();
    break;
  case 12:
    break;
  case 13:
    lighttests();
    break;
  }

#ifdef DEBUGSIMPLE
  Serial.print("protocol name is: ");
  Serial.println(protocol_name);
  Serial.print("total cycles are: ");
  Serial.println(total_cycles);
  Serial.print("sizeof pulses are: ");
  Serial.println(sizeof(pulses));
  Serial.print("sizeof pulses 0: ");
  Serial.println(sizeof(pulses[0]));
  Serial.print("pulses in first cycle: ");
  Serial.println(pulses[0]);
#endif

  for (x=0;x<repeats;x++) {                                              // Repeat the entire measurement this many times  
    if (cal_true != 2) {
      Serial1.print("{\"device_version\": 1,");                            //Begin JSON file printed to bluetooth on Serial ports
      Serial.print("{\"device_version\": 1,");
      Serial1.print("\"protocol\": \"");
      Serial.print("\"protocol\": \"");
      Serial1.print(protocol_name);
      Serial.print(protocol_name);
      Serial1.print("\", ");    
      Serial.print("\", ");    
      Serial.print("\"baseline_sample\": ");
      Serial1.print("\"baseline_sample\": ");
      Serial.print(cal1_sample,2);
      Serial1.print(cal1_sample,2);
      Serial.print(",");
      Serial1.print(",");
      Serial.print("\"baseline_values\": [");                                      // print the baseline values describing the amount of IR produced by the 2 measuring lights and the actinic light.  This baseline will be subtracted from the collected data if desired
      Serial1.print("\"baseline_values\": [");
      Serial.print(measuringlight1_baseline);
      Serial1.print(measuringlight1_baseline);
      Serial.print(",");
      Serial1.print(",");
      Serial.print(measuringlight2_baseline);
      Serial1.print(measuringlight2_baseline);
      Serial.print(",");
      Serial1.print(",");
      Serial.print(actiniclight1_baseline);
      Serial1.print(actiniclight1_baseline);
      Serial.print("]");
      Serial1.print("]");   
      Serial.print(",");
      Serial1.print(",");
      Serial1.print("\"chlorophyll_max\": [");                                      // print the maximum transmission values for the 650 and 940 LEDs used for calculate chlorophyll content measurement
      Serial.print("\"chlorophyll_max\": [");
      Serial.print(cal1_blank_d2);
      Serial1.print(cal1_blank_d2);
      Serial.print(",");
      Serial1.print(",");   
      Serial.print(act1_blank_d2);
      Serial1.print(act1_blank_d2);
      Serial.print("]");
      Serial1.print("]");   
      Serial.print(",");
      Serial1.print(",");
      }
    if (cal_true == 1) {
      switch (x) {                                                                   // indicate that this is a calibration run at low/high/blank reflectance for detector 1 or detector 2
        case 0:
          Serial1.print("\"calibration_high_d1\": TRUE,");
          Serial.print("\"calibration_high_d1\": TRUE,");
          detector = detector1;
          break;
        case 1:
          Serial1.print("\"calibration_low_d1\": \"TRUE\",");
          Serial.print("\"calibration_low_d1\": \"TRUE\",");
          detector = detector1;
          break;
        case 2:
          Serial1.print("\"calibration_blank_d1\": \"TRUE\",");
          Serial.print("\"calibration_blank_d1\": \"TRUE\",");
          detector = detector1;
          break;
        case 3:
          Serial1.print("\"calibration_high_d2\": \"TRUE\",");
          Serial.print("\"calibration_high_d2\": \"TRUE\",");
          detector = detector2;
          break;
        case 4:
          Serial1.print("\"calibration_low_d2\": \"TRUE\",");
          Serial.print("\"calibration_low_d2\": \"TRUE\",");
          detector = detector2;
          break;
        case 5:
          Serial1.print("\"calibration_blank_d2\": \"TRUE\",");
          Serial.print("\"calibration_blank_d2\": \"TRUE\",");
          detector = detector2;
          break;
        }
      }
    for (y=0;y<averages;y++) {                                                                        // Average this many measurements together to yield a single measurement output
      while ((cycle < total_cycles | pulse != pulses[total_cycles]) && pulses[cycle] != 0) {          // Keep doing the following until the last pulse of the last cycle...
        if (cycle == 0 && pulse == 0) {                                                                // if it's the beginning of a measurement, then...
          if (cal_true != 2) {
            Serial1.print("\"averages\": ");                                                             // begin sending JSON data via serial and bluetooth
            Serial1.print(y);  
            Serial1.print(",");   
            Serial.print("\"averages\": ");
            Serial.print(y);                     
            Serial.print(",");
            Serial1.print("\"repeats\": "); 
            Serial1.print(x);  
            Serial1.print(",");   
            Serial.print("\"repeats\": "); 
            Serial.print(x); 
            Serial.print(",");
            Serial1.print("\"data_raw\": [");                                        // Start recording the raw data as an array in JSON format
            Serial.print("\"data_raw\": [");
          }
          analogReadAveraging(measurements);                                      // set analog averaging (ie ADC takes one signal per ~3u)
          analogWrite(actiniclight_intensity1, actintensity1);                    // set intensities for each of the lights
          analogWrite(actiniclight_intensity2, actintensity2);
          analogWrite(measuringlight_pwm, measintensity);
          analogWrite(calibratinglight_pwm, calintensity);
          digitalWriteFast(meas1_light, LOW);                                     // make sure the lights doesn't flash at the beginning of the run
          digitalWriteFast(meas2_light, LOW);
          digitalWriteFast(meas3_light, LOW);
          digitalWriteFast(meas4_light, LOW);
          digitalWriteFast(act_light, LOW);
          digitalWriteFast(alt1_light, LOW);
          digitalWriteFast(alt2_light, LOW);
          digitalWriteFast(red_light, LOW);
          delay(5);                                                                // wait a few milliseconds so that the actinic pulse presets can stabilize
          starttimer0 = micros();
          timer0.begin(pulse1,pulsedistance);                                       // Begin firsts pulse
          while (micros()-starttimer0 < pulsesize) {
          }                               // wait a full pulse size, then...                                                                                          
          timer1.begin(pulse2,pulsedistance);                                       // Begin second pulse
#ifdef DEBUG
          Serial.println(starttimer0);
#endif
        }  
        while (on == 0 | off == 0) {
        }                                                                           // if ALL pulses happened, then...
        switch (measlights[cycle][meas_number%4]) {                                 // set the baseline for the next measuring light
        case 1:
          baseline = meas1_baseline;
          break;
        case 2:
          baseline = meas2_baseline;
          break;
        case 3:
          baseline = meas3_baseline;
          break;
        case 4:
          baseline = meas4_baseline;
          break;
        }
        noInterrupts();                                                              // turn off interrupts because we're checking volatile variables set in the interrupts
        on = 0;                                                                      // reset pulse counters
        off = 0;  
        pulse++;                                                                     // progress the pulse counter and measurement number counter
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
        if (measlights[cycle][meas_number%4] != 0 && cal_true != 2) {           // As long as it's not an empty cycle (ie pulses in this cycle aren't == 0) and not a baseline measurement then...
          Serial.print(data1-baseline);                                         // Output data in JSON format to serial, bluetooth
          Serial1.print(data1-baseline);
          data1_sum += data1;
#ifdef DEBUGSIMPLE
          Serial.print("!");
          Serial1.print("!");
          Serial.print(data1); 
          Serial1.print(data1);
          Serial.print("@"); 
          Serial1.print("@");
          Serial.print(baseline); 
          Serial1.print(baseline);
#endif
        }
        if (cal_true == 1 && meas_number >= 4) {                                // if this is a calibration run, then save the average of each separate array.  If it's a sample calibration run, then sum all into cal1sum_sample.  Also, ignore the first sets of measurements (it's usually low due to an artifact)
          if (x == 0) {                                                         // if it's the first repeat (x = 0), save as 'high' values (tin), if it's the 2nd repeat (x = 1) save as 'low' values (tape)
            switch (meas_number%4) {
            case 0:
              cal1sum_high_d1 += data1;                                            // Sum all of the data points for each light together for 'high', 'low', and 'blank' for detector 1 and detector 2 respectively (used to calculate the baseline calibration value)
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
            }
          }          
          else if (x == 1) {
            switch (meas_number%4) {
              case 0:
                cal1sum_low_d1 += data1; 
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
            }
          }
          else if (x == 2) {
            switch (meas_number%4) {
              case 0:
                cal1sum_blank_d1 += data1;
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
            }
          }
          else if (x == 3) {
            switch (meas_number%4) {
              case 0:
                cal1sum_high_d2 += data1;
                break;
              case 1:
                meas1sum_high_d2 += data1;
                break;
              case 2:
                meas2sum_high_d2 += data1;
                break;
              case 3:
                act1sum_high_d2 += data1;
                break;
            }
          }
          else if (x == 4) {
            switch (meas_number%4) {
              case 0:
                cal1sum_low_d2 += data1;
                break;
              case 1:
                meas1sum_low_d2 += data1;
                break;
              case 2:
                meas2sum_low_d2 += data1;
                break;
              case 3:
                act1sum_low_d2 += data1;
                break;
            }
          }
            
          else if (x == 5) {
            switch (meas_number%4) {
              case 0:
                cal1sum_blank_d2 += data1;
                break;
              case 1:
                meas1sum_blank_d2 += data1;
                break;
              case 2:
                meas2sum_blank_d2 += data1;
                break;
              case 3:
                act1sum_blank_d2 += data1;
                break;
            }            
          }
        }
        else if (cal_true == 2 && meas_number >= 4) {                               // If we are calculating the baseline, then sum all of the values together.  Again, ignore the first sets of measurements which often contain an artifact (abnormally high or low values)
          cal1sum_sample += data1;
        }
        interrupts();                                                              // done with volatile variables, turn interrupts back on
        meas_number++;                                                              // progress measurement number counters
        if (cal_true != 2) {
          Serial.print(",");      
          Serial1.print(",");
        }        
        if (pulse == pulses[cycle]) {                                             // if it's the last pulse of a cycle...
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
      if (cal_true != 2) {                                                           // close out the array (this repeats the last value in the array once, otherwise there's a hanging comma!)
        Serial.print(data1);
        Serial1.print(data1);
        Serial.print("],");
        Serial1.print("],");
      }
      timer0.end();                                                                  // if it's the last cycle and last pulse, then... stop the timers
      timer1.end();
      digitalWriteFast(meas1_light, LOW);                                            // ..make sure remaining lights are off
      digitalWriteFast(act_light, LOW);
      digitalWriteFast(alt1_light, LOW);
      digitalWriteFast(alt2_light, LOW);
      digitalWriteFast(red_light, LOW);
      cycle = 0;                                                                     // ...and reset counters
      pulse = 0;
      on = 0;
      off = 0;
      meas_number = 0;
    }
    if (cal_true == 0) {
      Serial.print("\"end\":1}");
      Serial1.print("\"end\":1}");
    }
    else if (cal_true == 1 && x == 5) {                                               // if it's a calibration run and it's finished both high and low sides (ie x == 2), then print and save calibration data to eeprom
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
      Serial.print(act2sum_high_d2);
      Serial.print(",");
      Serial.print(act2sum_low_d2);
      Serial.print(",");
      Serial.print(act2sum_blank_d2);
      Serial.print("],");
#endif
      Serial.print("\"cal_values\": [");
      Serial1.print("\"cal_values\": [");
      save_eeprom(cal1sum_high_d1,0);                                                      // save values for detector 1
      save_eeprom(cal1sum_low_d1,10);
      save_eeprom(cal1sum_blank_d1,20);
      save_eeprom(meas1sum_high_d1,30);
      save_eeprom(meas1sum_low_d1,40);
      save_eeprom(meas1sum_blank_d1,50);
      save_eeprom(meas2sum_high_d1,60);
      save_eeprom(meas2sum_low_d1,70);
      save_eeprom(meas2sum_blank_d1,80);
      save_eeprom(act1sum_high_d1,90);
      save_eeprom(act1sum_low_d1,100);
      save_eeprom(act1sum_blank_d1,110);
      
      save_eeprom(cal1sum_high_d2,120);                                                      // save values for detector 2
      save_eeprom(cal1sum_low_d2,130);
      save_eeprom(cal1sum_blank_d2,140);
      save_eeprom(meas1sum_high_d2,150);
      save_eeprom(meas1sum_low_d2,160);
      save_eeprom(meas1sum_blank_d2,170);
      save_eeprom(meas2sum_high_d2,180);
      save_eeprom(meas2sum_low_d2,190);
      save_eeprom(meas2sum_blank_d2,200);
      save_eeprom(act1sum_high_d2,210);
      save_eeprom(act1sum_low_d2,220);
      save_eeprom(act1sum_blank_d2,230);
      Serial.print("0]}");
      Serial1.print("0]}");   
    }
    else if (cal_true == 2) {
      cal1_sample = cal1sum_sample/(cal_pulses-4);                                        // when converting cal1_sum to a per-measurement value, we must take into account that we ignored the first 4 values when summing to avoid the artifacts (see above)
#ifdef DEBUGSIMPLE
      Serial.print("\"baseline_sample\": ");
      Serial.print(cal1_sample,2);
      Serial.print(",");
      Serial.print("\"baseline_values\": [");
      Serial.print(measuringlight1_baseline);
      Serial.print(",");
      Serial.print(measuringlight2_baseline);
      Serial.print(",");
      Serial.print(actiniclight1_baseline);
      Serial.println("]");
#endif
    }
    calculations();
    if (x+1 < repeats) {                                                           // countdown to next repeat, unless it's the end of the very last run
      countdown(wait);
    }
    if (cal_true == 1 && (repeats == 1 | repeats == 3)) {                           // since we only want delays after 2 detector runs for the calibration, then we manually enter the countdown here. NOTE may want to put in a buzzer or something to indicate it's done
      countdown(cal_wait);
    }
  }
  reset_vars();                                                                     // reset all variables to zero.  If baseline run, do not reset the baseline calculated results.
}

void pulse1() {
#ifdef DEBUG
  Serial.print(measlights[cycle][meas_number%4]);
  Serial.println(",");    
#endif
  switch (measlights[cycle][meas_number%4]) {
  case 1:
    digitalWriteFast(meas1_light, HIGH);
#ifdef DEBUG
    Serial.println("light one");
#endif
    break;
  case 2:
    digitalWriteFast(meas2_light, HIGH);
#ifdef DEBUG
    Serial.println("light two");
#endif
    break;
  case 3:
    digitalWriteFast(meas3_light, HIGH);
#ifdef DEBUG
    Serial.println("light three");
#endif
    break;
  case 4:
    digitalWriteFast(meas4_light, HIGH);
#ifdef DEBUG
    Serial.println("light four");
#endif
    break;
  }
  if (pulse == 0) {                                                                // if it's the first pulse of a cycle, then change sat, act, far red, alt1 and alt2 values as per array's set at beginning of the file
    if (act[cycle] == 2) {
      digitalWriteFast(act_light, LOW);
#ifdef DEBUG
      Serial.print("light off");
#endif
    }
    else {
      digitalWriteFast(actiniclight_intensity_switch, act[cycle]);
      digitalWriteFast(act_light, HIGH);
#ifdef DEBUG
      Serial.print("light on");
#endif
    }
    digitalWriteFast(alt1_light, alt1[cycle]);    
    digitalWriteFast(alt2_light, alt2[cycle]);
    digitalWriteFast(red_light, red[cycle]);
  }
  data1 = analogRead(detector)-baseline;                                          // save the detector reading as data1, and subtract of the baseline (if there is no baseline then baseline is automatically set = 0)
  on=1;
#ifdef DEBUG
  Serial.print("pulse on");
#endif
}

void pulse2() {
  digitalWriteFast(meas1_light, LOW);
  digitalWriteFast(meas2_light, LOW);
  digitalWriteFast(meas3_light, LOW);
  digitalWriteFast(meas4_light, LOW);
  off=1;
#ifdef DEBUG
  Serial.print("pulse off");
#endif
}

void reset_vars() {
  cal_true = 0;                                                                 // identify this as a calibration routine (=TRUE)
  repeats = 0;   			                                        // number of times to repeat the entire run 
  wait = 0;                  	                                                // seconds wait time between repeats
  averages = 0;                                                                 // number of runs to average
  measurements = 0;                                                             // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
  meas1_light = 0; 
  meas1_baseline = 0;
  meas2_light = 0; 
  meas2_baseline = 0;
  meas3_light = 0; 
  meas3_baseline = 0;
  meas4_light = 0; 
  meas4_baseline = 0;
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
  for (i=0;i<10;i++) {
    pulses[i] = 0;
    for (j=0;j<4;j++) {
      measlights[i][j] = 0;
    }
    act[i] = 2;
    alt1[i] = LOW;
    alt2[i] = LOW;
    red[i] = LOW;
  }

  cal1sum_sample = 0;
  data1_sum = 0;
  baseline = 0;
  
  if (protocol_value != 998) {                                                      // Only reset these variables, which are used to calculate the baseline, if the previous run was NOT a baseline measurement.
    measuringlight1_baseline = 0;
    measuringlight2_baseline = 0;
    actiniclight1_baseline = 0;
    baseline_flag = 0;                                                            // reset the baseline flag
  }
  else if (protocol_value == 998) {
    baseline_flag = 1;                                                            // flag to indicate we ran a baseline run first - ensures that the next run we calculate the baseline data and use it
  }
}

void lighttests() {

  int choose = 0;
  analogWrite(actiniclight_intensity2, 255);
  analogWrite(actiniclight_intensity1, 255);
  analogWrite(calibratinglight_pwm, 255);
  analogWrite(measuringlight_pwm, 255);
  digitalWriteFast(actiniclight_intensity_switch, HIGH);

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

    while (Serial.available()<3) {
    }

    choose = calc_Protocol();
    Serial.println(choose);

    if (choose<30) {
      Serial.println("First actinic intensty switch high, then actinic intensity switch low");
      delay(1000);
      for (y=0;y<2;y++) {
        for (x=0;x<256;x++) {
          Serial.println(x);
          analogWrite(measuringlight_pwm, x);
          analogWrite(calibratinglight_pwm, x);
          analogWrite(actiniclight_intensity1, x);
          analogWrite(actiniclight_intensity2, x);
          if (y==0) {
            digitalWriteFast(actiniclight_intensity_switch, HIGH);
          }
          else {
            digitalWriteFast(actiniclight_intensity_switch, LOW);
          }
          delay(2);
          digitalWriteFast(choose, HIGH);
          delay(2);
          digitalWriteFast(choose, LOW);
        }
        for (x=256;x>0;x--) {
          Serial.println(x);
          analogWrite(measuringlight_pwm, x);
          analogWrite(calibratinglight_pwm, x);
          analogWrite(actiniclight_intensity1, x);
          analogWrite(actiniclight_intensity2, x);
          if (y==0) {
            digitalWriteFast(actiniclight_intensity_switch, HIGH);
          }
          else {
            digitalWriteFast(actiniclight_intensity_switch, LOW);
          }
          delay(2);
          digitalWriteFast(choose, HIGH);
          delay(2);
          digitalWriteFast(choose, LOW);
        }
      }
    }
    else {
      switch (choose) {

      case 1710:
        for (x=0;x<5000;x++) {
          Serial.println(analogRead(A10));
          delay(10);
        }
        break;

      case 1711:
        for (x=0;x<5000;x++) {
          Serial.println(analogRead(A11));
          delay(10);
        }
        break;
      }
    }
  }
}

void lightmeter() {
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
}

void leaftemp() {
  for (x=0;x<10;x++) {
    float objt = tmp006.readObjTempC();
    Serial.print("Object Temperature: "); 
    Serial.print(objt); 
    Serial.println("*C");
    float diet = tmp006.readDieTempC();
    Serial.print("Die Temperature: "); 
    Serial.print(diet); 
    Serial.println("*C");   
    delay(4000); // 4 seconds per reading for 16 samples per reading
  }
}

void CO2cal() {
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

void Co2_evolution() {
  //  measure every second, compare current measurement to previous measurement.  If delta is <x, then stop.
  //  save the dif between measurement 1 and 2 (call CO2_slope), save measurement at final point (call CO2_final), save all points (call CO2_raw)
  // create a raw file based on 300 seconds measurement to stop, then fill in ramaining
  // data with the final value...

  int* co2_raw;
  int co2_maxsize = 150;
  int co2_start; // first CO2 measurement
  int co2_end;
  float co2_drop;
  int co2_slope;
  int co2_2; // second CO2 measurement
  int delta = 20; // measured in ppm
  int delta_min = 1; // minimum difference to end measurement, in ppm
  int co2_time = 1; // measured in seconds
  int c = 0; // counter
  int valCO2_prev;
  int valCO2_prev2;

  co2_raw = (int*)malloc(co2_maxsize*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle

  analogWrite(actiniclight_intensity1, 1); // set actinic light intensity
  digitalWriteFast(actiniclight_intensity_switch, LOW); // turn intensity 1 on
  digitalWriteFast(actiniclight1, HIGH);

  for (x=0;x<co2_maxsize;x++) {
    requestCo2(readCO2);
    valCO2_prev2 = valCO2_prev;
    valCO2_prev = valCO2;
    valCO2 = getCo2(response);
    Serial.print(valCO2);
    Serial.print(",");
    delta = (valCO2_prev2-valCO2_prev)+(valCO2_prev - valCO2);
    Serial.println(delta);
    c = c+1;
    if (c == 1) {
      co2_start = valCO2;
    }
    else if (c == 2) {
      co2_2 = valCO2;
    }
    /*
    else if (delta <= delta_min) {
     co2_end = valCO2;
     co2_drop = (co2_start-co2_end)/co2_start;
     Serial.println("baseline is reached!");
     Serial.print("it took this many seconds: ");
     Serial.println(x);
     Serial.print("CO2 dropped from ");
     Serial.print(co2_start);
     Serial.print(" to ");
     Serial.print(co2_end);
     Serial.print(" a % drop of ");    
     Serial.println(co2_drop, 3);
     break;
     }
     */
    else if (c == co2_maxsize-1) {
      co2_end = valCO2;
      co2_drop = (co2_start-co2_end)/co2_start;
      Serial.print("maximum measurement time exceeded!  Try a larger leaf, a smaller space, or check that the CO2 detector is working properly.");
      Serial.print("it took this many seconds: ");
      Serial.println(x);
      Serial.print("CO2 dropped from ");
      Serial.print(co2_start);
      Serial.print(" to ");
      Serial.print(co2_end);
      Serial.print(" a % drop of ");    
      Serial.println(co2_drop, 3);
    }
    delay(2000);
  }
  Serial.print(x); 
  digitalWriteFast(actiniclight1, LOW);
  analogWrite(actiniclight_intensity1, 0); // set actinic light intensity
}

void Co2() {
  requestCo2(readCO2);
  valCO2 = getCo2(response);
  Serial1.print("\"co2_content\": ");
  Serial1.print(valCO2);  
  Serial1.print(",");
  Serial.print("\"co2_content\": ");
  Serial.print(valCO2);  
  Serial.print(",");
  delay(100);
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
      while(Serial3.available())  //flush whatever we have
          Serial3.read();
      break;                        //exit and try again
    }
    delay(50);
  }
  for (int i=0; i < 7; i++) {
    response[i] = Serial3.read();
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

void relh() {
  Wire.beginTransmission(0x40); // 7 bit address
  Wire.send(0xF5); // trigger temp measurement
  Wire.endTransmission();
  delay(wait2);

  // Print response and convert to Celsius:
  Wire.requestFrom(0x40, 2);
  byte byte1 = Wire.read();
  byte byte2 = Wire.read();
  rhval = byte1;
  rhval<<=8; // shift byte 1 to bits 1 - 8
  rhval+=byte2; // put byte 2 into bits 9 - 16
  rh = 125*(rhval/pow(2,16))-6;
  Serial1.print("\"relative_humidity\": ");
  Serial1.print(rh);  
  Serial1.print(",");
  Serial.print("\"relative_humidity\": ");
  Serial.print(rh);  
  Serial.print(",");
}

void temp() {
  Wire.beginTransmission(0x40); // 7 bit address
  Wire.send(0xF3); // trigger temp measurement
  Wire.endTransmission();
  delay(wait2);

  // Print response and convert to Celsius:
  Wire.requestFrom(0x40, 2);
  byte byte1 = Wire.read();
  byte byte2 = Wire.read();
  tempval = byte1;
  tempval<<=8; // shift byte 1 to bits 1 - 8
  tempval+=byte2; // put byte 2 into bits 9 - 16
  temperature = 175.72*(tempval/pow(2,16))-46.85;
  Serial1.print("\"temperature\": ");
  Serial1.print(temperature);  
  Serial1.print(",");
  Serial.print("\"temperature\": ");
  Serial.print(temperature);  
  Serial.print(",");
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
  Serial.println();
}

void cal_baseline() {
  measuringlight1_baseline = (meas1_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(meas1_high_d1-meas1_low_d1));
  measuringlight2_baseline = (meas2_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(meas2_high_d1-meas2_low_d1));
  actiniclight1_baseline = (act1_low_d1+((cal1_sample-cal1_low_d1)/(cal1_high_d1-cal1_low_d1))*(act1_high_d1-act1_low_d1));
}

int protocol_runtime(volatile int protocol_pulses[], volatile int protocol_pulsedistance[][2], volatile int protocol_total_cycles) {
  int total_time = 0;
  for (x=0;x<protocol_total_cycles;x++) {
    total_time += protocol_pulses[x]*(protocol_pulsedistance[x][0]+protocol_pulsedistance[x][1]);
  }
  return total_time;
}

int countdown(int _wait) {
  for (z=0;z<_wait;z++) {
#ifdef DEBUG
    Serial.print(_wait);
    Serial.print(",");
    Serial.print(z);
#endif
    Serial.print("Time remaining (press 1 to skip): ");
    Serial.println(_wait-z);
    delay(1000);
    if (Serial.available()>0) {
      Serial.println("Ok - skipping wait!");
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



