
// implement MarkTs suggestion - comparing the smaller values (not huge values) when running micros()
// you cant't start the 2nd interrupt during the first!  It has to be done in the main loop

// concerned maybe the time that we turn interrupts off is too long (now that we have all that calibration stuff in the way... we may need to adjust this.


/*
DESCRIPTION:
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


//#define DEBUG 1  // uncomment to add debug features
//#define DEBUGSIMPLE 1

// SD CARD ENABLE AND SET
#include <Time.h>   // enable real time clock library
#include <Wire.h>
#include <EEPROM.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <Adafruit_Sensor.h>
#include <Adafruit_TMP006.h>
#include <Adafruit_TCS34725.h>

// PIN DEFINITIONS AND TEENSY SETTINGS
float reference = 1.2; // The reference (AREF) supplied to the ADC - currently set to INTERNAL = 1.2V
int analogresolution = 16; // Set the resolution of the analog to digital converter (max 16 bit, 13 bit usable)  
int measuringlight1 = 15; // Teensy pin for measuring light
int measuringlight2 = 16; // Teensy pin for measuring light
int measuringlight3 = 11; // Teensy pin for measuring light
int measuringlight4 = 12  ; // Teensy pin for measuring light
int actiniclight1 = 20;
int actiniclight2 = 2;
int calibratinglight1 = 14;
int calibratinglight2 = 10;
//int actiniclight1 = 12; // Teensy pin for actinic light - set same as measuringlight2

int measuringlight_pwm = 23;
int calibratinglight_pwm = 9;
int actiniclight_intensity2 = 3;
int actiniclight_intensity1 = 4;
int actiniclight_intensity_switch = 5;
int detector1 = A10; // Teensy analog pin for detector
int detector2 = A11; // Teensy analog pin for detector

// SHARED VARIABLES
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
volatile int off = 0; // counter for 'off' pulse
volatile int on = 0; // counter for 'on' pulse
volatile unsigned long meas_number = 0; // counter for measurement lights 1 - 4
float measuringlight1_baseline = 0, measuringlight2_baseline = 0, actiniclight1_baseline = 0, baseline = 0;
unsigned long cal1sum_high = 0;
unsigned long cal1sum_low = 0;
unsigned long meas1sum_high = 0;
unsigned long meas1sum_low = 0;
unsigned long meas2sum_high = 0;
unsigned long meas2sum_low = 0;
unsigned long act1sum_high = 0;
unsigned long act1sum_low = 0;
unsigned long cal1sum_sample = 0;
float cal1_high = 0;
float cal1_low = 0;
float meas1_high = 0;
float meas1_low = 0;
float meas2_high = 0;
float meas2_low = 0;
float act1_high = 0;
float act1_low = 0;
float cal1_sample = 0;


// HARDWARE NOTES
/*
Optical:
RISE TIME, MEASURING: 0.4us
FALL TIME, Measuring: 2.5us

RISE TIME Actinic: <1us
FALL TIME Actinic: delayed by 15us, 25us fall time

Electrical:
measuring is OK
Actinic fall time: 5 - 10us

Other:
using namespace - <.2us
digitalWriteFast - <.1us
analogread (single average) - 4 - 6us
noise due to detector reference impacted by actinic pulse - 6 detector units (.3mv)
USB as power supply DOES NOT WORK - causes significant sags when using actinic lights

*/

String protocol_name;
int repeats;   			                                         // number of times to repeat the entire run 
int wait;                  	                                         // seconds wait time between repeats
int averages;                                                            // number of runs to average
int measurements;                                                        // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
int meas1_light; 
int meas1_baseline = 0;                                                  // set the baseline to subtract from each measured value for meas1_light
int meas2_light; 
int meas2_baseline = 0;                                                  // set the baseline to subtract from each measured value for meas2_light 
int meas3_light; 
int meas3_baseline = 0;                                                  // set the baseline to subtract from each measured value for meas3_light
int meas4_light;
int meas4_baseline = 0;                                                  // set the baseline to subtract from each measured value for meas4_light 
int act_light;
int red_light;
int alt1_light;
int alt2_light;
int detector;
int pulsesize;                                                          // measured in microseconds
int pulsedistance;
int actintensity1;                                                      // intensity at LOW setting below
int actintensity2;                                                      // intensity at HIGH setting below
int measintensity;                                                      // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
int calintensity;                                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
int pulses [10] = {0,0,0,0,0,0,0,0,0,0};            	                // 
int measlights [10][4] = {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};       // measured in us. Minimum 200us
int act [10] = {2,2,2,2,2,2,2,2,2,2};                                   // 2 is off.  "LOW" is actintensity1, "HIGH" is actintensity2.  May use this for combined actinic / saturation pulses  .  NOTE! You may set the intensity at any value during the run even if it's not preset - however, the LED intensity rise time is a few milliseconds.  The rise time on the preset values is in the nanoseconds range.
int alt1 [10] = {LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};              // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
int alt2 [10] = {LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};              // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
int red [10] = {LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};               // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
int total_cycles;	                                                // Total number of cycles - note first cycle is cycle 0
boolean cal_true = 0;                                                   // Bool to identify calibration routines
int cal_pulses = 400;                                                    // number of total pulses in the calibration routine

// KEY CALIBRATION VARIABLES
unsigned long calpulsecycles = 50; // Number of times the "pulselengthon" and "pulselengthoff" cycle during calibration (on/off is 1 cycle)
// data for measuring and actinic pulses --> to calculate total time=pulsecycles*(pulselengthon + pulselengthoff)
unsigned long calpulselengthon = 30; // Pulse LED on length for calibration in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
unsigned long calpulselengthoff = 49970; // Pulse LED off length for calibration in uS (minimum = 20us + any additional operations which you may want to call during that time).
unsigned long cmeasurements = 4; // # of measurements per pulse (min 1 measurement per 6us pulselengthon)

// HTU21D Temp/Humidity variables
#define temphumid_address 0x40 // HTU21d Temp/hum I2C sensor address
int sck = 19; // clock pin
int sda = 18; // data pin
int wait2 = 200; // typical delay to let device finish working before requesting the data
unsigned int tempval;
unsigned int rhval;
float temperature;
float rh;

// S8 CO2 variables
byte readCO2[] = {
  0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};  //Command packet to read CO2 (see app note)
byte response[] = {
  0,0,0,0,0,0,0};  //create an array to store CO2 response
float valMultiplier = 0.1;
int CO2calibration = 17; // manual CO2 calibration pin (CO2 sensor has auto-calibration, so manual calibration is only necessary when you don't want to wait for autocalibration to occur - see datasheet for details 
unsigned long valCO2;

// TMP006 variables - address is 1000010 (adr0 on SDA, adr1 on GND)
Adafruit_TMP006 tmp006;

//TCS34725 variables
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

// INTERNAL VARIABLES, COUNTERS, ETC.
volatile unsigned long start1,start2, start1orig,end1, start3, end3, calstart1orig, calend1, start5, start6, start7, end5;
unsigned long pulselengthoncheck, pulselengthoffcheck, pulsecyclescheck, totaltimecheck, caltotaltimecheck;
volatile float data1f, data2f, data3f, data4f, irtinvalue, irtapevalue, rebeltapevalue, rebeltinvalue, irsamplevalue, rebelsamplevalue, baselineir, dataaverage, caldataaverage1, caldataaverage2, rebelslope, irslope;
char filenamedir[13];
char filenamelocal[13];
volatile int data0=0, data1=0, data2=0, data3=0, data4=0, data5=0, data6=0, data7=0, data8=0, data9=0, data10=0, caldata1, caldata2, caldata3, caldata4, analogresolutionvalue, dpulse1count, dpulse1noactcount, dpulse2count;
int i=0, j=0, k=0,z=0,y=0,q=0,x=0,p=0; // Used as a counters
int* caldatatape; 
int* caldatatin; 
int* caldatasample;
int* rebeldatasample;
int val=0, cal=0, cal2=0, cal3=0, val2=0, flag=0, flag2=0, keypress=0, protocol_value, protocols;
IntervalTimer timer0, timer1, timer2;
char c;

void setup() {

// IMPROVEMENTS!
// Be able to choose the number of pulses and which ones to save measurements on, and organize those measurements as part of the array
// Work on ifdef array... 10 pulses where you can set something (maybe a 001010111 or something) which defines which pules are 'active' and which are 'off'... then save data in array also

  delay(3000);
  Serial.begin(115200); // set baud rate for Serial communication to computer via USB
  Serial.println("Serial works");
  Serial1.begin(115200); // set baud rate for bluetooth communication on pins 0,1
  Serial.println("Serial1 works");
  Serial3.begin(9600);
  Serial.println("Serial3 works");
  Wire.begin(); // This causes the actinic light not to flash, and 
  Serial.println("Wire works");
  // TCS and tmp006 require additional work to get them to work with the other wire libraries
  //  tcs.begin();
  //  Serial.println("TCS works");
  //  tmp006.begin();
  //  Serial.println("tmp works");
  //  if (! tmp006.begin()) {
  //    Serial.println("No IR temperature sensor found (TMP006)");
  //    }

  pinMode(measuringlight1, OUTPUT); // set pin to output
  pinMode(measuringlight2, OUTPUT); // set pin to output
  pinMode(measuringlight3, OUTPUT); //
  pinMode(measuringlight4, OUTPUT); //
  pinMode(actiniclight1, OUTPUT); // set pin to output
  pinMode(actiniclight2, OUTPUT); // set pin to output
  pinMode(calibratinglight1, OUTPUT); // set pin to output  
  pinMode(calibratinglight2, OUTPUT); // set pin to output  
  pinMode(measuringlight_pwm, OUTPUT); // set pin to output  
  pinMode(actiniclight_intensity2, OUTPUT); // set pin to output
  pinMode(actiniclight_intensity1, OUTPUT); // set pin to output
  pinMode(actiniclight_intensity_switch, OUTPUT); // set pin to output
  pinMode(calibratinglight_pwm, OUTPUT); // set pin to output  
  pinMode(actiniclight1, OUTPUT); // set pin to output (currently unset)
  analogReadAveraging(1); // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u
  pinMode(detector1, EXTERNAL);
  pinMode(detector2, EXTERNAL);
  analogReadRes(analogresolution);
  analogresolutionvalue = pow(2,analogresolution); // calculate the max analogread value of the resolution setting
  Serial.println("All LEDs and Detectors are powered up!");
  analogWriteFrequency(3, 375000);
  analogWriteFrequency(5, 375000); // Pins 3 and 5 are each on timer 0 and 1, respectively.  This will automatically convert all other pwm pins to the same frequency.
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

void loop() {


/*
using namespace spad;
Serial.print("total cycles are: ");
Serial.println(total_cycles);
Serial.print("sizeof pulses are: ");
Serial.println(sizeof(pulses));
Serial.print("sizeof pulses 0: ");
Serial.println(sizeof(pulses[0]));
Serial.print("total cycles: ");
Serial.println(total_cycles);
Serial.print("pulses in final cycle: ");
Serial.println(pulses[total_cycles]);
*/  

  if (protocol_value !=8) {
    Serial.println();
    Serial.println("Please select a 3 digit protocol code to begin");
    Serial.println("(000 for light testing)");
    Serial.println("(001 for DIRKF / PMF, RGB light, and CO2 measurement)");  
    Serial.println("");
  }
  while (Serial1.available()<3 && Serial.available()<3) {
  }
  protocol_value = calc_Protocol(); // Retreive the 3 digit protocol code 000 - 999
  #ifdef DEBUG
  Serial.print("you selected protocol number: ");
  Serial.println(protocol_value);
  #endif
  
  switch(protocol_value) {
  case 999:        // END TRANSMISSION
    break;
  case 998:        // NULL RETURN
    Serial.println("");
    Serial.println("nothing happens - please use bluetooth for serial communication!");
    Serial1.println("");
    Serial1.println("nothing happens");
    break;
  case 0:        // CALIBRATION
    lighttests();
    break;

  case 1:                                           //////////////////// FLUORESCENCE ////////////////////////
    protocol_name =       "fluorescence";
    repeats =             1;                          // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                1;                          // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight1;            // orange
    meas1_baseline =      measuringlight1_baseline;   // set the baseline to subtract from each measured value 
    meas2_light =         measuringlight1;            // orange
    meas3_light =         measuringlight3;            // orange
    meas4_light =         measuringlight1;            // orange
    act_light =           actiniclight2;              // red
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector1;
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       10000;                       // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       255;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           50;                         // 
    pulses[1] =           50;                         // 
    pulses[2] =           50;                         // 
    pulses[3] =           50;                         // 
    measlights [0][0] = 1;                            // measured in us. Minimum 200us
    measlights [0][1] = 1;                            // measured in us. Minimum 200us
    measlights [0][2] = 1;                            // measured in us. Minimum 200us
    measlights [0][3] = 1;                            // measured in us. Minimum 200us
    measlights [1][0] = 1;                            // measured in us. Minimum 200us
    measlights [1][1] = 1;                            // measured in us. Minimum 200us
    measlights [1][2] = 1;                            // measured in us. Minimum 200us
    measlights [1][3] = 1;                            // measured in us. Minimum 200us
    measlights [2][0] = 1;                            // measured in us. Minimum 200us
    measlights [2][1] = 1;                            // measured in us. Minimum 200us
    measlights [2][2] = 1;                            // measured in us. Minimum 200us
    measlights [2][3] = 1;                            // measured in us. Minimum 200us
    measlights [3][0] = 1;                            // measured in us. Minimum 200us
    measlights [3][1] = 1;                            // measured in us. Minimum 200us
    measlights [3][2] = 1;                            // measured in us. Minimum 200us
    measlights [3][3] = 1;                            // measured in us. Minimum 200us
    act [1] = HIGH;                                      // measured in us. Minimum 200us
    total_cycles = sizeof(pulses)/sizeof(pulses[0])-1;// (start counting at 0!)
    break;

  case 2:                                    //////////////////// DIRK ////////////////////////
    protocol_name =       "dirk";
    repeats =             1;                    // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                1;                   // seconds wait time between repeats
    averages =            1;                    // number of runs to average
    measurements =        3;                    // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight2;     // 520 blue
    meas2_light =         measuringlight2;     // 520 blue
    meas3_light =         measuringlight2;     // 520 blue
    meas4_light =         measuringlight2;     // 520 blue
    act_light =           actiniclight1;       // any
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector2;
    pulsesize =           20;                  // us, length of pulse must be <100us
    pulsedistance =       1000;                // distance between pulses
    actintensity1 =       50;                  // intensity at LOW setting below
    actintensity2 =       255;                 // intensity at HIGH setting below
    measintensity =       255;                 // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                 // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           100;
    pulses[1] =           100;
    pulses[2] =           100;
    measlights [0][0] =   1;                 // measured in us. Minimum 200us
    measlights [0][1] =   1;                 // measured in us. Minimum 200us
    measlights [0][2] =   1;                 // measured in us. Minimum 200us
    measlights [0][3] =   1;                 // measured in us. Minimum 200us
    measlights [1][0] =   1;                 // measured in us. Minimum 200us
    measlights [1][1] =   1;                 // measured in us. Minimum 200us
    measlights [1][2] =   1;                 // measured in us. Minimum 200us
    measlights [1][3] =   1;                 // measured in us. Minimum 200us    measlights [0][0] = 1;     // measured in us. Minimum 200us
    measlights [2][0] =   1;                 // measured in us. Minimum 200us
    measlights [2][1] =   1;                 // measured in us. Minimum 200us
    measlights [2][2] =   1;                 // measured in us. Minimum 200us
    measlights [2][3] =   1;                 // measured in us. Minimum 200us
    act [0] =             HIGH;              // measured in us. Minimum 200us
    act [2] =             HIGH;              // measured in us. Minimum 200us    
    total_cycles = sizeof(pulses)/sizeof(pulses[0])-1;// (start counting at 0!)
    break;

  case 3:        //////////////////// SPAD ////////////////////////
    protocol_name =       "spad";
    repeats =             1;                           // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                1;                           // seconds wait time between repeats
    averages =            1;                           // number of runs to average
    measurements =        10;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         calibratinglight1;           // 940
    meas2_light =         actiniclight1;               // 650
    meas3_light =         calibratinglight1;           // 940
    meas4_light =         actiniclight1;               // 650
    act_light =           actiniclight1;               // any
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector2;
    pulsesize =           75;                         // us, length of pulse must be <100us
    pulsedistance =       1000;                       // distance between pulses
    actintensity1 =       1;                         // intensity at LOW setting below
    actintensity2 =       1;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           500;                        // 
    measlights [0][0] =   1;                         // measured in us. Minimum 200us
    measlights [0][1] =   2;                         // measured in us. Minimum 200us
    measlights [0][2] =   1;                         // measured in us. Minimum 200us
    measlights [0][3] =   2;                         // measured in us. Minimum 200us
    total_cycles = sizeof(pulses)/sizeof(pulses[0])-1;// (start counting at 0!)
    break;

  case 4:        // TEMP / RH
    protocol_name =       "810_dirk";
    repeats =             1;                         // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                1;                         // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight3; // 850
    meas2_light =         measuringlight3; // 850
    meas3_light =         measuringlight3; // 850
    meas4_light =         measuringlight3; // 850
    act_light =           actiniclight2;   // any
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector1;
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       1000;                       // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       255;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           100;                        // 
    pulses[1] =           100;                        // 
    pulses[2] =           100;                        // 
    measlights [0][0] = 1;                            // measured in us. Minimum 200us
    measlights [0][1] = 1;                            // measured in us. Minimum 200us
    measlights [0][2] = 1;                            // measured in us. Minimum 200us
    measlights [0][3] = 1;                            // measured in us. Minimum 200us
    measlights [1][0] = 1;                            // measured in us. Minimum 200us
    measlights [1][1] = 1;                            // measured in us. Minimum 200us
    measlights [1][2] = 1;                            // measured in us. Minimum 200us
    measlights [1][3] = 1;                            // measured in us. Minimum 200us
    measlights [2][0] = 1;                            // measured in us. Minimum 200us
    measlights [2][1] = 1;                            // measured in us. Minimum 200us
    measlights [2][2] = 1;                            // measured in us. Minimum 200us
    measlights [2][3] = 1;                            // measured in us. Minimum 200us
    act [0] = HIGH;                                   // measured in us. Minimum 200us
    act [2] = HIGH;                                   // measured in us. Minimum 200us
    break;

  case 5:        
    protocol_name =       "940_dirk";
    repeats =             1;                          // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                1;                          // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight4;            // 940
    meas2_light =         measuringlight4;            // 940
    meas3_light =         measuringlight4;            // 940
    meas4_light =         measuringlight4;            // 940
    act_light =           actiniclight2;              // any
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector1;
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       1000;                       // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       255;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           100;                        // 
    pulses[1] =           100;                        // 
    pulses[2] =           100;                        // 
    measlights [0][0] = 1;                            // measured in us. Minimum 200us
    measlights [0][1] = 1;                            // measured in us. Minimum 200us
    measlights [0][2] = 1;                            // measured in us. Minimum 200us
    measlights [0][3] = 1;                            // measured in us. Minimum 200us
    measlights [1][0] = 1;                            // measured in us. Minimum 200us
    measlights [1][1] = 1;                            // measured in us. Minimum 200us
    measlights [1][2] = 1;                            // measured in us. Minimum 200us
    measlights [1][3] = 1;                            // measured in us. Minimum 200us
    measlights [2][0] = 1;                            // measured in us. Minimum 200us
    measlights [2][1] = 1;                            // measured in us. Minimum 200us
    measlights [2][2] = 1;                            // measured in us. Minimum 200us
    measlights [2][3] = 1;                            // measured in us. Minimum 200us
    act [1] = HIGH;                                   // measured in us. Minimum 200us
    break;

  case 6:              
    protocol_name =       "810_saturation";
    repeats =             1;                          // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                1;                          // seconds wait time between repeats
    averages =            1;                          // number of runs to average
    measurements =        3;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         measuringlight3;            // 850
    meas2_light =         measuringlight4;            // 940 
    meas3_light =         measuringlight3;            // 850
    meas4_light =         measuringlight4;            // 940
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
    measlights [0][0] = 1;                            // measured in us. Minimum 200us
    measlights [0][1] = 2;                            // measured in us. Minimum 200us
    measlights [0][2] = 1;                            // measured in us. Minimum 200us
    measlights [0][3] = 2;                            // measured in us. Minimum 200us
    measlights [1][0] = 1;                            // measured in us. Minimum 200us
    measlights [1][1] = 2;                            // measured in us. Minimum 200us
    measlights [1][2] = 1;                            // measured in us. Minimum 200us
    measlights [1][3] = 2;                            // measured in us. Minimum 200us
    measlights [2][0] = 1;                            // measured in us. Minimum 200us
    measlights [2][1] = 2;                            // measured in us. Minimum 200us
    measlights [2][2] = 1;                            // measured in us. Minimum 200us
    measlights [2][3] = 2;                            // measured in us. Minimum 200us
    measlights [3][0] = 1;                            // measured in us. Minimum 200us
    measlights [3][1] = 2;                            // measured in us. Minimum 200us
    measlights [3][2] = 1;                            // measured in us. Minimum 200us
    measlights [3][3] = 2;                            // measured in us. Minimum 200us
    measlights [4][0] = 1;                            // measured in us. Minimum 200us
    measlights [4][1] = 2;                            // measured in us. Minimum 200us
    measlights [4][2] = 1;                            // measured in us. Minimum 200us
    measlights [4][3] = 2;                            // measured in us. Minimum 200us
    measlights [5][0] = 1;                            // measured in us. Minimum 200us
    measlights [5][1] = 2;                            // measured in us. Minimum 200us
    measlights [5][2] = 1;                            // measured in us. Minimum 200us
    measlights [5][3] = 2;                            // measured in us. Minimum 200us
    act [0] = LOW;                                    // measured in us. Minimum 200us
    act [1] = HIGH;                                   // measured in us. Minimum 200us
    act [4] = HIGH;                                   // measured in us. Minimum 200us
    act [5] = LOW;                                    // measured in us. Minimum 200us
    break;
  case 7:        // CALIBRATION // easy way to check to see if this is working correctly - set pulses[0] = 8, then manually see if the averages for each pulse are equal to the saved values for low and high.
    protocol_name =       "calibration";
    cal_true =            1;                                         // identify this as a calibration routine (1 = true, 0 = false)
    repeats =             2;                                         // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                10;                                         // seconds wait time between repeats
    averages =            1;                                         // number of runs to average
    measurements =        12;                                        // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         calibratinglight1;                         // 850
    meas2_light =         measuringlight1;                           // amber
    meas3_light =         measuringlight2;                           // cyan
    meas4_light =         actiniclight1;                             // 650
    act_light =           actiniclight2;   
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector1;
    pulsesize =           75;                                        // us, length of pulse must be <100us
    pulsedistance =       1000;                                      // us, distance between pulses
    actintensity1 =       1;                                       // intensity at LOW setting below
    actintensity2 =       1;                                       // intensity at HIGH setting below
    measintensity =       255;                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           cal_pulses;                                // cal_pulses must be a global variable, so please set this at the top of the file
    measlights [0][0] = 1;                                           // measured in us. Minimum 200us
    measlights [0][1] = 2;                                           // measured in us. Minimum 200us
    measlights [0][2] = 3;                                           // measured in us. Minimum 200us
    measlights [0][3] = 4;                                           // measured in us. Minimum 200us
    break;
  case 8:        // BASELINE CALCULATION
    protocol_name =       "baseline_sample";
    cal_true =            2;                                         // identify this as a calibration routine (0 = false, 1 = re-calibrate, 2 = calibrate to sample)
    repeats =             1;                                         // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                                         // seconds wait time between repeats
    averages =            1;                                         // number of runs to average
    measurements =        12;                                        // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         calibratinglight1;                         // 850
    meas2_light =         measuringlight1;                           // amber
    meas3_light =         measuringlight2;                           // cyan
    meas4_light =         actiniclight1;                         // 650
    act_light =           actiniclight2;   
    red_light =           calibratinglight1;
    alt1_light =          measuringlight2;
    alt2_light =          measuringlight2;
    detector =            detector1;
    pulsesize =           75;                                        // us, length of pulse must be <100us
    pulsedistance =       1000;                                      // us, distance between pulses
    actintensity1 =       1;                                        // intensity at LOW setting below
    actintensity2 =       1;                                        // intensity at HIGH setting below
    measintensity =       255;                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           500;                                      // cal_pulses must be a global variable, so please set this at the top of the file
    measlights [0][0] = 1;                                           // measured in us. Minimum 200us
    measlights [0][1] = 1;                                           // measured in us. Minimum 200us
    measlights [0][2] = 1;                                           // measured in us. Minimum 200us
    measlights [0][3] = 1;                                           // measured in us. Minimum 200us
    break;
  case 10:        // eeprom tests
    leaftemp();
    break;
  case 11:        // eeprom tests
    lightmeter();
    break;
  case 12:        // eeprom tests
    break;
  case 13:        // eeprom tests
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
  Serial.print("sample calibration value: ");
  Serial.println(cal1_sample);
  #endif
  cal_baseline();                                                        // Calculate and print baseline data, display saved calibration values
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
    }
    for (y=0;y<averages;y++) {                                                                       // Average this many measurements together to yield a single measurement output
      while ((cycle < total_cycles | pulse != pulses[total_cycles]) && pulses[cycle] != 0) {          // Keep doing the following until the last pulse of the last cycle...
        if (cycle == 0 && pulse == 0) {                                                                // if it's the beginning of a measurement, then...
          if (cal_true != 2) {
          Serial1.print("\"average\": ");                                                             // begin sending JSON data via serial and bluetooth
          Serial1.print(y);  
          Serial1.print(",");   
          Serial.print("\"average\": ");
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
          while (micros()-starttimer0 < pulsesize) {}                               // wait a full pulse size, then...                                                                                          
          timer1.begin(pulse2,pulsedistance);                                       // Begin second pulse
          #ifdef DEBUG
          Serial.println(starttimer0);
          #endif
        }  
        while (on == 0 | off == 0) {}                                               // if ALL pulses happened, then...
        switch (meas_number%4) {                                                    // set the baseline for the next measuring light
          case 0:
            baseline = meas1_baseline;
            break;
          case 1:
            baseline = meas2_baseline;
            break;
          case 2:
            baseline = meas3_baseline;
            break;
          case 3:
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

        if (measlights[cycle][meas_number%4] != 0 && cal_true != 2) {             // As long as it's not an empty cycle (ie pulses in this cycle aren't == 0) and not a baseline measurement then...
          Serial.print(data1);                                                    // Output data in JSON format to serial, bluetooth
          Serial1.print(data1);
        }
        Serial.print(",");
        Serial.print(meas_number);
        Serial.println(",");
        if (cal_true == 1 && meas_number >= 4) {                                    // if this is a calibration run, then save the average of each separate array.  If it's a sample calibration run, then sum all into cal1sum_sample.  Also, ignore the first sets of measurements (it's usually low due to an artifact)
          if (x == 0) {                                                             // if it's the first repeat (x = 0), save as 'high' values (tin), if it's the 2nd repeat (x = 1) save as 'low' values (tape)
            switch (meas_number%4) {
              case 0:
                cal1sum_high += data1;                                                 // Sum all of the data points for each light together for 'high' (used to calculate the baseline calibration value)
                break;
              case 1:
                meas1sum_high += data1;
                break;
              case 2:
                meas2sum_high += data1;
                break;
              case 3:
                act1sum_high += data1;
                break;
            }
          }          
          else if (x == 1) {
            switch (meas_number%4) {
              case 0:
                cal1sum_low += data1;                                                // Sum all of the data points for each light together for 'low' (used to calculate the baseline calibration value)
                break;
              case 1:
                meas1sum_low += data1;
                break;
              case 2:
                meas2sum_low += data1;
                break;
              case 3:
                act1sum_low += data1;
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
  
  if (cal_true == 1 && x == 1) {                                                             // if it's a calibration run and it's finished both high and low sides (ie x == 2), then print and save calibration data to eeprom
    #ifdef DEBUG
    Serial.print("\"calsum_values\": [");
    Serial.print(cal1sum_high);
    Serial.print(",");
    Serial.print(cal1sum_low);
    Serial.print(",");
    Serial.print(meas1sum_high);
    Serial.print(",");
    Serial.print(meas1sum_low);
    Serial.print(",");
    Serial.print(meas2sum_high);
    Serial.print(",");
    Serial.print(meas2sum_low);
    Serial.print(",");
    Serial.print(cal2sum_high);
    Serial.print(",");
    Serial.print(cal2sum_low);
    Serial.print(",");
    Serial.print("0],");
    #endif
    Serial.print("\"cal_values\": [");
    Serial1.print("\"cal_values\": [");
    save_eeprom(cal1sum_high,0);
    save_eeprom(cal1sum_low,10);
    save_eeprom(meas1sum_high,20);
    save_eeprom(meas1sum_low,30);
    save_eeprom(meas2sum_high,40);
    save_eeprom(meas2sum_low,50);
    save_eeprom(act1sum_high,60);
    save_eeprom(act1sum_low,70);
    Serial.print("0],");
    Serial1.print("0],");   
  }
  else if (cal_true == 2) {
    cal1_sample = cal1sum_sample/(cal_pulses-4);                                        // when converting cal1_sum to a per-measurement value, we must take into account that we ignored the first 4 values when summing to avoid the artifacts (see above)
    Serial.print("\"baseline_sample\": ");
    Serial1.print("\"baseline_sample\": ");
    Serial.print(cal1_sample,2);
    Serial1.print(cal1_sample,2);
    Serial.print(",");
    Serial1.print(",");
  }
  if (cal_true != 2) {
    Serial.print("\"end\":1");
    Serial.println("}");
    Serial1.print("\"end\":1");
    Serial1.println("}");
    }
  countdown(wait);
}
  reset_vars();                                                                     // reset all variables to zero
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
  cal_true = 0;                                         // identify this as a calibration routine (=TRUE)
  repeats = 0;   			                                        // number of times to repeat the entire run 
  wait = 0;                  	                                                // seconds wait time between repeats
  averages = 0;                                                                 // number of runs to average
  measurements = 0;                                                             // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
  meas1_light = 0; 
  meas2_light = 0; 
  meas3_light = 0; 
  meas4_light = 0; 
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
cal1sum_high = 0;
cal1sum_low = 0;
meas1sum_high = 0;
meas1sum_low = 0;
meas2sum_high = 0;
meas2sum_low = 0;
act1sum_high = 0;
act1sum_low = 0;
cal1_high = 0;
cal1_low = 0;
meas1_high = 0;
meas1_low = 0;
meas2_high = 0;
meas2_low = 0;
act1_high = 0;
act1_low = 0;
if (protocol_value != 8) {
  measuringlight1_baseline = 0;
  measuringlight2_baseline = 0;
  actiniclight1_baseline = 0;
  meas1_baseline = 0;
  meas2_baseline = 0;
  meas3_baseline = 0;
  meas4_baseline = 0;
  baseline = 0;
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

void calibration() {

  analogReadAveraging(cmeasurements); // set analog averaging (ie ADC takes one signal per ~3u)
  Serial.print("<RUNTIME>");
  Serial.print("5");
  Serial.println("</RUNTIME>");


  Serial.println("Place the shiny side of the calibration piece face up in the photosynq, and close the lid.");
  Serial.println("When you're done, press any key to continue");
  Serial.flush();
  while (Serial.read() <= 0) {
  }

  Serial.println("Thanks - calibrating...");
  calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtin()!
  calibrationtin();

  Serial.println("");
  Serial.println("Now place the black side of the calibration piece face up in the photosynq, and close the lid.");
  Serial.println("");  
  Serial.println("");

  Serial.println("When you're done, press any key to continue");
  Serial.flush();
  while (Serial.read() <= 0) {
  }

  Serial.println("Thanks - calibrating...");
  calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtape()!
  calibrationtape();
  Serial.println("Calibration finished!");
  Serial.println("");
}


void calibrationrebel2() {

  // CALIBRATION REBEL2
  // Short pulses for calibration using the measuring LED (rebel orange)

  digitalWriteFast(calibratinglight_pwm, 255); // set calibrating light intensity
  delay(50);

  rebeldatasample = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts();

  start1orig = micros();
  start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(measuringlight1, HIGH); 
    data1 = analogRead(detector1); 
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(measuringlight1, LOW); 
    rebeldatasample[i] = data1; 
    while (micros()<start1) {
    } 
  }
  end1 = micros();
  interrupts();

  free(rebeldatasample); // release the memory allocated for the data

  for (i=0;i<calpulsecycles;i++) {
    rebelsamplevalue += rebeldatasample[i]; // totals all of the analogReads taken
  }
  delay(50);

  rebelsamplevalue = (float) rebelsamplevalue; // create a float for rebelsamplevalue so it can be saved later
  rebelsamplevalue = (rebelsamplevalue / calpulsecycles);
  Serial.print("Rebel sample value:  ");
  Serial.println(rebelsamplevalue);
  Serial.println("");  
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(rebeldatasample[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println("");
}



void calibrationrebel() {

  // CALIBRATION REBEL
  // Short pulses for calibration using the measuring LED (rebel orange)

  digitalWriteFast(calibratinglight_pwm, 255); // set calibrating light intensity
  delay(50);

  rebeldatasample = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts();

  start1orig = micros();
  start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(measuringlight1, HIGH); 
    data1 = analogRead(detector1); 
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(measuringlight1, LOW); 
    rebeldatasample[i] = data1; 
    while (micros()<start1) {
    } 
  }
  end1 = micros();
  interrupts();

  free(rebeldatasample); // release the memory allocated for the data

  for (i=0;i<calpulsecycles;i++) {
    rebelsamplevalue += rebeldatasample[i]; // totals all of the analogReads taken
  }
  delay(50);

  rebelsamplevalue = (float) rebelsamplevalue; // create a float for rebelsamplevalue so it can be saved later
  rebelsamplevalue = (rebelsamplevalue / calpulsecycles);
  Serial.print("Rebel sample value:  ");
  Serial.println(rebelsamplevalue);
  Serial.println("");  
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(rebeldatasample[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println("");
}


void calibrationtin() {

  // CALIBRATION TIN
  // Flash calibrating light to determine how reflective the sample is to ~850nm light with the tin foil as the sample (low reflectance).  This has been tested with Luxeon Rebel Orange as measuring pulse.

  digitalWriteFast(calibratinglight_pwm, 255); // set calibrating light intensity
  delay(50);

  caldatatin = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts(); // turn off interrupts to reduce interference from other calls

    start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(calibratinglight1, HIGH);
    caldata1 = analogRead(detector1);
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(calibratinglight1, LOW); 
    caldatatin[i] = caldata1;  
    while (micros()<start1) {
    } 
  }

  interrupts();

  for (i=0;i<calpulsecycles;i++) {
    irtinvalue += caldatatin[i]; // totals all of the analogReads taken
  }
  Serial.println(irtinvalue);  
  irtinvalue = (float) irtinvalue;
  irtinvalue = (irtinvalue / calpulsecycles); //  Divide by the total number of samples to get the average reading during the calibration - NOTE! The divisor here needs to be equal to the number of analogReads performed above!!!! 
  rebeltinvalue = rebelsamplevalue;
  rebelsamplevalue = (int) rebelsamplevalue; // reset rebelsamplevalue to integer for future operations
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(caldatatin[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println(" ");    
  Serial.print("the baseline high reflectance value from calibration: ");
  Serial.println(irtinvalue, 7);
  Serial.print("The last 4 data points from the calibration: ");  
  Serial.println(caldata1);
}

void calibrationtape() {

  // CALIBRATION TAPE
  // Flash calibrating light to determine how reflective the sample is to ~850nm light with the black tape as the sample (low reflectance).  This has been tested with Luxeon Rebel Orange as measuring pulse.

  digitalWriteFast(calibratinglight_pwm, 255);
  delay(50);

  caldatatape = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts(); // turn off interrupts to reduce interference from other calls

    start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(calibratinglight1, HIGH);
    caldata1 = analogRead(detector1);
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(calibratinglight1, LOW);
    caldatatape[i] = caldata1; 
    while (micros()<start1) {
    } 
  }

  interrupts();

  for (i=0;i<calpulsecycles;i++) {
    irtapevalue += caldatatape[i]; // totals all of the analogReads taken
  }
  Serial.println(irtapevalue);
  irtapevalue = (float) irtapevalue;
  irtapevalue = (irtapevalue / calpulsecycles); 
  rebeltapevalue = rebelsamplevalue;
  rebelsamplevalue = (int) rebelsamplevalue; // reset rebelsamplevalue to integer for future operations 
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(caldatatape[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println(" ");    
  Serial.print("the baseline low reflectance value from calibration:  ");
  Serial.println(irtapevalue, 7);

  //CALCULATE AND SAVE CALIBRATION DATA TO SD CARD
  rebelslope = rebeltinvalue - rebeltapevalue;
  irslope = irtinvalue - irtapevalue;

  //CALCULATE AND SAVE CALIBRATION DATA TO EEPROM (convert to integer and save decimal places by x10,000)

  Serial.println("Rebel tape value: ");
  savecalibration(rebeltapevalue, 0);
  Serial.print("<CAL1>");
  callcalibration(0);
  Serial.println("</CAL1>");
  Serial.println("");

  Serial.println("Rebel tin value: ");
  savecalibration(rebeltinvalue, 10);
  Serial.print("<CAL2>");
  callcalibration(10);
  Serial.println("</CAL2>");
  Serial.println("");

  Serial.println("IR tape value: ");
  savecalibration(irtapevalue, 20);
  Serial.print("<CAL3>");
  callcalibration(20);
  Serial.println("</CAL3>");
  Serial.println("");

  Serial.println("IR tin value: ");
  savecalibration(irtinvalue, 30);
  Serial.print("<CAL4>");
  callcalibration(30);
  Serial.println("</CAL4>");
  Serial.println("");

  Serial.println("Rebel slope value: ");
  savecalibration(rebelslope, 40);
  Serial.print("<CAL5>");
  callcalibration(40);
  Serial.println("</CAL5>");
  Serial.println("");

  Serial.println("IR slope value: ");
  savecalibration(irslope, 50);
  Serial.print("<CAL6>");
  callcalibration(50);
  Serial.println("</CAL6>");
  Serial.println("");

}

void calibrationsample() {

  // CALIBRATION SAMPLE
  // Flash calibrating light to determine how reflective the sample is to ~850nm light with the actual sample in place.  This has been tested with Luxeon Rebel Orange as measuring pulse.

  digitalWriteFast(calibratinglight_pwm, 255); // set calibrating light intensity
  delay(50);

  caldatasample = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts(); // turn off interrupts to reduce interference from other calls

    calstart1orig = micros();
  start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(calibratinglight1, HIGH);
    caldata1 = analogRead(detector1);
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(calibratinglight1, LOW); 
    caldatasample[i] = caldata1; 
    while (micros()<start1) {
    } 
  }
  calend1 = micros();

  interrupts();

  for (i=0;i<calpulsecycles;i++) {
    irsamplevalue += caldatasample[i]; // totals all of the analogReads taken
  }
  irsamplevalue = (float) irsamplevalue;
  irsamplevalue = (irsamplevalue / calpulsecycles); 
  for (i=0;i<calpulsecycles;i++) { // Print the results!
  }

  // CALCULATE BASELINE VALUE

  // Pull saved calibration values from EEPROM

  rebeltapevalue = callcalibration(0);
  rebeltinvalue = callcalibration(10);
  irtapevalue = callcalibration(20);
  irtinvalue = callcalibration(30);
  rebelslope = callcalibration(40);
  irslope = callcalibration(50);

  baseline = (rebeltapevalue+((irsamplevalue-irtapevalue)/irslope)*rebelslope);

  Serial1.print("\"ir_low\":");
  Serial1.print(irtapevalue);
  Serial1.print(",");
  Serial1.print("\"ir_high\":");
  Serial1.print(irtinvalue);
  Serial1.print(",");
  Serial1.print("\"led_low\":");
  Serial1.print(rebeltapevalue);
  Serial1.print(",");
  Serial1.print("\"led_high\":");
  Serial1.print(rebeltinvalue);
  Serial1.print(",");
  Serial1.print("\"baseline\":");
  Serial1.print(baseline);
  Serial1.print(",");

  Serial.print("\"ir_low\":");
  Serial.print(irtapevalue);
  Serial.print(",");
  Serial.print("\"ir_high\":");
  Serial.print(irtinvalue);
  Serial.print(",");
  Serial.print("\"led_low\":");
  Serial.print(rebeltapevalue);
  Serial.print(",");
  Serial.print("\"led_high\":");
  Serial.print(rebeltinvalue);
  Serial.print(",");
  Serial.print("\"baseline\":");
  Serial.print(baseline);
  Serial.print(",");

/*
  Serial.println("calibration values");
  Serial.println(irtapevalue);
  Serial.println(irtinvalue);
  Serial.println(rebeltapevalue);
  Serial.println(rebeltinvalue);
  Serial.println(rebelslope);
  Serial.println(irslope);
  Serial.println(irsamplevalue);
  Serial.println("baseline:");
  Serial.println(baseline);
*/
}

void save_eeprom(int saved_val, int loc) {
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

float call_eeprom(int loc) { 
  char temp [10];
  float called_val;
  for (i=0;i<10;i++) {
    temp[i] = EEPROM.read(loc+i);
  }
  called_val = ((float) atoi(temp) / ((cal_pulses-1)/4));                                          // when converting called_val to a per-measurement value, we have to divide by 1 less than cal_pulses/4 because we skipped the first set of values to avoid artifacts
  #ifdef DEBUGSIMPLE
  Serial.print(called_val);
  Serial.print(",");
  #endif
  return called_val;
}  

void savecalibration(float calval, int loc) {
  char str [10];
  calval = calval*1000000;
  calval = (int) calval;
  itoa(calval, str, 10);
  for (i=0;i<10;i++) {
    EEPROM.write(loc+i,str[i]);
    //    Serial1.print(str[i]);
    char temp = EEPROM.read(loc+i);
  }
  Serial.println(str);
}

int callcalibration(int loc) { 
  char temp [10];
  float calval;
  for (i=0;i<10;i++) {
    temp[i] = EEPROM.read(loc+i);
  }
  calval = atoi(temp);
  calval = calval / 1000000;
  #ifdef DEBUG
  Serial.print(calval,4);
  #endif
  return calval;
}  

// ok so it seems dumb to convert the values into floats, just so we can multiply them by a million when we save them and pull them back out...
// so create a new save cal and call cal function which doesn't use floats, and just uses the initial summed values.  Then, when it gets called back out,
// we can convert it to a float and divide by the pulse[0] at that time
// how to save a large integer... what is the max size of 'i'?
// add in meas3 and meas 4 once your done proving the idea...
// add in detector 2 once you're done with that
// add in varying the light levels to get a graph of baseline values by light level once your done with that!

void cal_saveandprint() {                                                      // calculate the baseline calibration values and save them to EEPROM

  Serial.print("\"cal_string\": [");
  Serial1.print("\"cal_string\": [");
  save_eeprom(cal1_low,0);                                                    // save calibration data
  save_eeprom(cal1_high,10);
  save_eeprom(meas1_low,20);
  save_eeprom(meas1_high,30);
  save_eeprom(meas2_low,40);
  save_eeprom(meas2_high,50);
  save_eeprom(act1_low,60);
  save_eeprom(act1_high,70);
  Serial.print("0]");
  Serial1.print("0]"); 
}

void cal_baseline() {
  #ifdef DEBUGSIMPLE
  Serial.print("Calibration values called from EEPROM: ");
  #endif
  float cal1_high = call_eeprom(0);
  float cal1_low = call_eeprom(10);                                                    // save calibration data
  float meas1_high = call_eeprom(20);
  float meas1_low = call_eeprom(30);
  float meas2_high = call_eeprom(40);
  float meas2_low = call_eeprom(50);
  float act1_high = call_eeprom(60);
  float act1_low = call_eeprom(70);
  measuringlight1_baseline = (meas1_low+((cal1_sample-cal1_low)/(cal1_high-cal1_low))*(meas1_high-meas1_low));
  measuringlight2_baseline = (meas2_low+((cal1_sample-cal1_low)/(cal1_high-cal1_low))*(meas2_high-meas2_low));
  actiniclight1_baseline = (act1_low+((cal1_sample-cal1_low)/(cal1_high-cal1_low))*(act1_high-act1_low));
  #ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("baseline values: ");
  Serial.print(measuringlight1_baseline);
  Serial.print(",");
  Serial.print(measuringlight2_baseline);
  Serial.print(",");
  Serial.println(actiniclight1_baseline);
  #endif
}

int protocol_runtime(volatile int protocol_pulses[], volatile int protocol_pulsedistance[][2], volatile int protocol_total_cycles) {
  int total_time = 0;
  for (x=0;x<protocol_total_cycles;x++) {
    total_time += protocol_pulses[x]*(protocol_pulsedistance[x][0]+protocol_pulsedistance[x][1]);
  }
  return total_time;
}
