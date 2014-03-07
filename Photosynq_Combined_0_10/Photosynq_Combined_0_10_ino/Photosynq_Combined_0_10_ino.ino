// CHANGE fluorescence to fluorescence_phi2
// CHANGE dirk to ecs_dirk


/*
/////////////////////LICENSE/////////////////////
 GPLv3 license
 by Greg Austic
 If you use this code, please be nice and attribute!
 Libraries from Adafruit were used for TMP006 and TCS34725, as were code snippets from their examples for taking the measurements.  Thanks Adafruit!
 
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
 'repeats' are the number of times that you repeat the same measurement, start to finish.  Each repeat outputs it's own unique JSON.  
 'averages' are the number of times you repeat the same measurement BUT the values are averaged and only a single JSON is outputted.
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
int analogresolution = 16;                                                   // Set the resolution of the analog to digital converter (max 16 bit, 13 bit usable)  
int measuringlight1 = 15;                                                    // Teensy pin for measuring light
int measuringlight2 = 16;                                                    // Teensy pin for measuring light
int measuringlight3 = 11;                                                    // Teensy pin for measuring light
int measuringlight4 = 12;                                                    // Teensy pin for measuring light
int _meas_light;															 // measuring light to be used during the interrupt
int actiniclight1 = 20;
int actiniclight2 = 2;
int calibratinglight1 = 14;
int calibratinglight2 = 10;
int measuringlight_pwm = 23;
int calibratinglight_pwm = 9;
int actiniclight_intensity2 = 3;
int actiniclight_intensity1 = 4;
int actiniclight_intensity_switch = 5;
int detector1 = A10;                                                          // Teensy analog pin for detector
int detector2 = A11;                                                          // Teensy analog pin for detector

//////////////////////Shared Variables///////////////////////////
volatile int data1=0, off = 0, on = 0;
volatile unsigned long meas_number = 0;                                       // counter to cycle through measurement lights 1 - 4 during the run
int analogresolutionvalue;
int i=0,j=0, k=0,z=0,y=0,q=0,x=0,p=0;                                         // Used as a counters
int protocol_number, protocols;
unsigned long starttimer0, starttimer1, starttimer2;
int cycle = 0;                                                                // current cycle number (start counting at 0!)
int pulse = 0;                                                                // current pulse number
IntervalTimer timer0, timer1, timer2;
int detector;
int cal_pulses = 400;                                               	      // number of total pulses in the calibration routine if it is a calibration routine
int total_cycles;	                       	                        		 // Total number of cycles - note first cycle is cycle 0
unsigned long dif_json;
unsigned long start_json;
String json2;
char w;

///////////////////////Calibration Variables////////////////////////// 
float measuringlight1_baseline = 0, measuringlight2_baseline = 0, actiniclight1_baseline = 0, baseline = 0, baseline_flag = 0;
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
int repeats;   			                                    		     // number of times to repeat the entire run 
int wait;                  	                               		          // seconds wait time between repeats
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
int pulsesize;                                                          // length of measuring pulses in us, must be <100us
int pulsedistance;                                                      // distance between pulses (min 1000us or 3000us with DEBUGSIMPLE on)
int actintensity1;                                                      // intensity at LOW setting below
int actintensity2;                                                      // intensity at HIGH setting below
int measintensity;                                                      // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
int calintensity;                                                       // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
int pulses [10] = {
  0,0,0,0,0,0,0,0,0,0};            	                // array with the number of pulses in each cycle
int measlights [10][4] = {   										    // for each cycle you may select one of the 4 measuring lights specified above (meas1 - meas4).  For example, for cycle 1 if you want to alternative between meas2 and meas4 then set the values at {2,4,2,4}
  {
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
  ,{
    0,0,0,0  }
};
int detectors [10][4] = {												// for each cycle you may select one of the 2 detectors (detector 1 is on main board, detector 2 is on add on).  This is preset to 1 (main detector)
  {
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
  ,{
    1,1,1,1  }
};       
int act [10] = {														// 2 is off.  "LOW" is actintensity1, "HIGH" is actintensity2.  May use this for combined actinic / saturation pulses  .  NOTE! You may set the intensity at any value during the run even if it's not preset - however, the LED intensity rise time is a few milliseconds.  The rise time on the preset values is in the nanoseconds range.
  2,2,2,2,2,2,2,2,2,2};                                   
int alt1 [10] = {														// If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
  LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};              					
int alt2 [10] = {														// If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
  LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};              				
int red [10] = {														// If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is actintensity1, HIGH is actintensity2. 
  LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW,LOW};               				
int cal_true = 0;                                                       // identify this as a calibration routine (0 = false, 1 = re-calibrate, 2 = calibrate to sample)

//////////////////////HTU21D Temp/Humidity variables///////////////////////////
#define temphumid_address 0x40                                           // HTU21d Temp/hum I2C sensor address
HTU21D htu;                                                              // create class object
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
unsigned long co2_value;
int co2_counter = 0;                                                     // counters for co2 evolution
int co2_count = 0;                  
volatile int co2_flag = 0;

////////////////////TMP006 variables - address is 1000010 (adr0 on SDA, adr1 on GND)//////////////////////
Adafruit_TMP006 tmp006(0x42);  // start with a diferent i2c address!  ADR1 is GND, ADR0 is SDA
float tmp006_cal_S = 6.4;
float tmp006_walk = 2;
int tmp006_up = 0;
int tmp006_down = 0;
float objt;
float diet;

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
  delay(500);
  tmp006.begin();                                                              // this opens wire.begin as well, and initializes tmp006, tcs light sensor, and htu21D.  by default, contactless leaf temp measurement takes 4 seconds to complete

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






JsonHashTable hashTable;
char * json3 = malloc(4);




//////////////////////// MAIN LOOP /////////////////////////
void loop() {

  if (protocol_number <990) {                                                  // don't print this if it's a baseline protocol or a protocol for environmental sensors (CO2, light, temp, etc.)
    Serial.println();
    Serial.println("Please select a 3 digit protocol code to begin");
    Serial.println("(000 for light testing)");
    Serial.println("(001 for DIRKF / PMF, RGB light, and CO2 measurement)");  
    Serial.println("");
  }
  while (Serial1.available()<3 && Serial.available()<3) {
  }
  protocol_number = calc_Protocol();                    						// Retreive the 3 digit protocol code 000 - 999
#ifdef DEBUG
  Serial.print("you selected protocol number: ");
  Serial.println(protocol_number);
#endif

  recall_all();                                        							// recall and save the data from the EEPROM
  if (baseline_flag == 1) {                           							 // calculate baseline values from saved EEPROM data only if the previous run was a sample baseline run (otherwise leave baseline == 0)
    cal_baseline();
  }

  switch(protocol_number) {


  
  
  case 20:                                             //////////////////// INPUT FROM JSON ///////////////////////
    json2;  // the String
    w;
    start_json = micros();
    while (Serial.available()>0 | dif_json > 50000) {
      if (Serial.available()>0) {
        w = Serial.read();
        json2 += w;
      }
      dif_json = micros() - start_json;
    }
	Serial.println(json2);
	Serial.println(json2.length());
	Serial.print(json2.c_str());

/* So here's the deal - 
we use malloc to declare a global variable up top... then we free it and reinitialize at the time of the Serial communication, then create the hash map.
I think then we should probably just save all thsoe variables directly at that point, and then we can delete the free the hash map at that point to save memory.  
*/

    JsonParser<10> parser;
    free(json3); // MAKE SURE THAT JSON3 HAS BEEN MALLOC'D FIRST!!!!!
    json3 = (char*)malloc(sizeof(char)*json2.length()+1);
    strncpy(json3, json2.c_str(), json2.length()+1);
    json3[json2.length()] = '\0';
    hashTable = parser.parseHashTable(json3);
    const char* name;
    name = hashTable.getString("Name");
    Serial.println(name);
    break;
										
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
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       10000;                      // distance between pulses
    actintensity1 =       50;                         // intensity at LOW setting below
    actintensity2 =       100;                        // intensity at HIGH setting below
    measintensity =       255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           50;
    pulses[1] =           50; 
    pulses[2] =           50; 
    pulses[3] =           50;
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
    measlights [3][0] =   1;
    measlights [3][1] =   1;
    measlights [3][2] =   1; 
    measlights [3][3] =   1;  
    act [1] =             HIGH;   
    total_cycles =        sizeof(pulses)/sizeof(pulses[0])-1;      // (start counting at 0!)
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
    pulsesize =           20;                  // us, length of pulse must be <100us
    pulsedistance =       1000;                // distance between pulses
    actintensity1 =       100;                  // intensity at LOW setting below
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
    detectors [0][0] =    2; 
    detectors [0][1] =    2; 
    detectors [0][2] =    2;
    detectors [0][3] =    2; 
    detectors [1][0] =    2; 
    detectors [1][1] =    2;
    detectors [1][2] =    2; 
    detectors [1][3] =    2;
    detectors [2][0] =    2; 
    detectors [2][1] =    2;
    detectors [2][2] =    2;
    detectors [2][3] =    2;     
    act [0] =             LOW;                   // keep on low until we get the new detector           
    act [2] =             LOW;  
    total_cycles =        sizeof(pulses)/sizeof(pulses[0])-1;        // (start counting at 0!)
    break;

  case 3:                                              //////////////////// CHLOROPHYLL SPAD ////////////////////////  NOTE not currently adjusted for fluorescence due to the 650 flash - this would require an additional calibration but it's ok, the impact is very small and consistent in the same direction
    protocol_name =       "chlorophyll_spad";
    repeats =             1;                           // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted finished data = 3)
    wait =                0;                           // seconds wait time between repeats
    averages =            1;                           // number of runs to average
    measurements =        10;                          // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
    meas1_light =         calibratinglight1;            // 940
    meas2_light =         actiniclight1;               // 650
    pulsesize =           75;                         // us, length of pulse must be <100us
    pulsedistance =       3000;                       // distance between pulses (min 1000us or 3000us with DEBUGSIMPLE on)
    actintensity1 =       1;                          // intensity at LOW setting below
    actintensity2 =       1;                          // intensity at HIGH setting below
    measintensity =       200;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    calintensity =        255;                        // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
    pulses[0] =           500; 
    measlights [0][0] =   1;
    measlights [0][1] =   2; 
    measlights [0][2] =   1;
    measlights [0][3] =   2;
    detectors [0][0] =    2;
    detectors [0][1] =    2; 
    detectors [0][2] =    2;
    detectors [0][3] =    2;
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
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       1000;                       // distance between pulses
    actintensity1 =       100;                         // intensity at LOW setting below
    actintensity2 =       200;                        // intensity at HIGH setting below
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
    act [0] =               LOW;                      // keep on low until we get the new detector
    act [2] =               LOW;
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
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       1000;                       // distance between pulses
    actintensity1 =       100;                         // intensity at LOW setting below
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
    act [1] =               LOW;                       // keep on low until we get the new detector
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
    pulsesize =           20;                         // us, length of pulse must be <100us
    pulsedistance =       1000;                       // distance between pulses
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
    detectors [0][0] =    1;
    detectors [0][1] =    1;
    detectors [0][2] =    1;
    detectors [0][3] =    1;
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

  case 13:
    Co2_evolution(12000,11999,4,1000,1);
    break;

  case 14:
    Co2(0);
    break;

  case 15:
    temp(0);
    relh(0);
    break;
    
  case 997:
    lightmeter();
    relh(0);
    temp(0);      
    Co2(0);
    leaftemp();
    break;

  case 996:                                                               // calibration of tmp006 light meter.  Calibrate with HTU21D onboard, walk values until equal
    tmp006_calibration();  
    break;

  case 995:
    for (i=0;i<20;i++) {
      relh(0);
      delay(5);
      temp(0);  
      delay(500);
    }
    break;

  case 994:
    for (i=0;i<20;i++) {
      lightmeter();  
      delay(500);
    }    
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

  for (x=0;x<repeats;x++) {                                                       // Repeat the entire measurement this many times  
    if (cal_true != 2) {                                                          //Begin JSON file printed to bluetooth on Serial ports
      Serial1.print("{\"device_id\": ");
      Serial.print("{\"device_id\": ");
      Serial1.print(device_id);
      Serial.print(device_id);
      Serial1.print(", "); 
      Serial.print(", ");
      Serial1.print("\"firmware_version\": \"");                                          //Begin JSON file printed to bluetooth on Serial ports
      Serial.print("\"firmware_version\": \"");
      Serial1.print(firmware_version);
      Serial.print(firmware_version);
      Serial1.print("\", "); 
      Serial.print("\", ");
      Serial1.print("\"protocol_number\": ");
      Serial.print("\"protocol_number\": ");
      Serial1.print(protocol_number);
      Serial.print(protocol_number);
      Serial1.print(", ");    
      Serial.print(", ");    
      Serial1.print("\"protocol_name\": \"");
      Serial.print("\"protocol_name\": \"");
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
      Serial1.print("\"chlorophyll_spad_calibration\": [");                         // print the maximum transmission values for the 650 and 940 LEDs used for calculate chlorophyll content measurement
      Serial.print("\"chlorophyll_spad_calibration\": [");
      Serial.print(cal1_blank_d2);                                                  // 940nm
      Serial1.print(cal1_blank_d2);
      Serial.print(",");
      Serial1.print(",");   
      Serial.print(act1_blank_d2);                                                  // 650nm
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
        Serial1.print("\"calibration_high_d2\": \"TRUE\",");
        Serial.print("\"calibration_high_d2\": \"TRUE\",");
        detector = detector1;
        break;
      case 2:
        Serial1.print("\"calibration_low_d1\": \"TRUE\",");
        Serial.print("\"calibration_low_d1\": \"TRUE\",");
        detector = detector1;
        break;
      case 3:
        Serial1.print("\"calibration_low_d2\": \"TRUE\",");
        Serial.print("\"calibration_low_d2\": \"TRUE\",");
        detector = detector2;
        break;
      case 4:
        Serial1.print("\"calibration_blank_d1\": \"TRUE\",");
        Serial.print("\"calibration_blank_d1\": \"TRUE\",");
        detector = detector2;
        break;
      case 5:
        Serial1.print("\"calibration_blank_d2\": \"TRUE\",");
        Serial.print("\"calibration_blank_d2\": \"TRUE\",");
        detector = detector2;
        break;
      }
    }

/* change the case switch statement determining measuring and actinic lights before the while loop waiting for off/on to happen... 
 * add detector to that new switch statement
 * create new detector string to determine which detectors are getting called at 0,1,2,3
 * Get this working.... try out new spad measurement with it
 * Figure out how to parse incoming JSON... 
 * save the values to the current value set
 * Try to run a file from Serial input... does it work?
 * averages - integrate and save with 3.1
 * can I instead of setting detectors to 0 or 1 and then using a switch statement to change it to detector0 and detector1, can't I just set it equal to the string 'detector0' and 'detector1'...??
*/

    else if (cal_true == 0) {
      lightmeter();                                                                                // initiate environmental measurements
      relh(0);
      temp(0);      
//      Co2(0);
//      leaftemp();
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
////
#ifdef DEBUG
  Serial.print(measlights[cycle][meas_number%4]);
  Serial.println(",");    
#endif
  switch (measlights[cycle][meas_number%4]) {										// define which measuring light is going to be pulsed
  case 1:
    _meas_light = meas1_light;
#ifdef DEBUG
    Serial.println("light one");
#endif
    break;
  case 2:
    _meas_light = meas2_light;
#ifdef DEBUG
    Serial.println("light two");
#endif
    break;
  case 3:
    _meas_light = meas3_light;
#ifdef DEBUG
    Serial.println("light three");
#endif
    break;
  case 4:
    _meas_light = meas4_light;
#ifdef DEBUG
    Serial.println("light four");
#endif
    break;
}

  switch (detectors[cycle][meas_number%4]) {										// define which detector is going to be used on the next measuring pulse
  case 1:
    detector = detector1;
#ifdef DEBUG
    Serial.println("light one");
#endif
    break;
  case 2:
    detector = detector2;
#ifdef DEBUG
    Serial.println("light two");
#endif
    break;
}
        while (on == 0 | off == 0) {                                        	     // if ALL pulses happened, then...
        }
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
          else if (x == 2) {
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
          else if (x == 3) {
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
          else if (x == 4) {
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
      Serial.println("\"end\":1}");
      Serial1.println("\"end\":1}");
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
    if (x+1 < repeats && cal_true != 1) {                                            // countdown to next repeat, unless it's the end of the very last run
      countdown(wait);
    }
    else if (x+1 < repeats && cal_true == 1 && (x == 1 | x == 3)) {                  // since we only want delays after 2 detector runs for the calibration, then we manually enter the countdown here. NOTE may want to put in a buzzer or something to indicate it's done
      countdown(cal_wait);
    }
  }
  reset_vars();                                                                     // reset all variables to zero.  If baseline run, do not reset the baseline calculated results.
}
////
void pulse1() {																		// turn on measuring light and/or actinic lights etc., tick counter
  digitalWriteFast(_meas_light, HIGH);												// turn on measuring light
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

void pulse2() {																	// turn off measuring light, tick counter
  digitalWriteFast(_meas_light, LOW);
  off=1;
#ifdef DEBUG
  Serial.print("pulse off");
#endif
}

void reset_vars() {
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
	  detectors[i][j] = 1;
    }
    act[i] = 2;
    alt1[i] = LOW;
    alt2[i] = LOW;
    red[i] = LOW;
  }

  cal1sum_sample = 0;
  data1_sum = 0;
  baseline = 0;
  json2 = "";


  if (protocol_number < 990) {                                                      // Only reset these variables, which are used to calculate the baseline, if the previous run was NOT a baseline measurement.
    measuringlight1_baseline = 0;
    measuringlight2_baseline = 0;
    actiniclight1_baseline = 0;
    baseline_flag = 0;                                                            // reset the baseline flag
  }
  else if (protocol_number == 998) {
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
        for (x=0;x<1000;x++) {
          Serial.println(analogRead(A10));
          delay(10);
        }
        break;

      case 1711:
        for (x=0;x<1000;x++) {
          Serial.println(analogRead(A11));
          delay(10);
        }
        break;
      }
    }
  }
}

void tmp006_calibration() {

  Serial.println("Place a piece of black electrical tape, or other high emissivity item in the leaf clip and permanently clamp the clip with a rubber band.  Place the entire unit in a cooler out of the sun for 15 minutes to let the temeprature come to equilibrium in the space.  Once finished, press any key and the calibration will continue.  It will take anywhere between 30 seconds and 10 minutes, depending how far out of calibration you were initially");
    while (Serial.available()<1) {}                                         // wait for button press
    while (tmp006_walk > .05 && tmp006_up < 100 && tmp006_down < 100) {     // if it's walked 100 steps in any direction or if it's zoned in on the value (ie walk is <.1) then save the final value to EEPROM
      leaftemp();
      delay(10);
      temp(0);
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
  objt = readObjTempC_mod();
  Serial.print("\"object_temperature\": ");
  Serial.print((float) readObjTempC_mod());
  Serial.print(",");
  Serial1.print("\"object_temperature\": "); 
  Serial1.print((float) readObjTempC_mod());
  Serial1.print(",");    
  diet = tmp006.readDieTempC();
  Serial.print("\"board_temperature\": "); 
  Serial.print((float) tmp006.readDieTempC());
  Serial.print(",");
  Serial1.print("\"board_temperature\": "); 
  Serial1.print((float) tmp006.readDieTempC());
  Serial1.print(",");
  delay(4000); // 4 seconds per reading for 16 samples per reading but shortened to make faster samples
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
      while (co2_flag < 1) {}
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

void Co2_flag() {                                                                                                               // if co2_quiet == 1, then do not include additional print statment.  If 0, then include.
  co2_flag = 1;
}

unsigned long Co2(int quiet) {                                                                    // if co2_quiet == 1, then do not include additional print statment.  If 0, then include.
  if (quiet == 0) {
    Serial1.print("\"co2_content\": ");
    Serial.print("\"co2_content\": ");
  }
  requestCo2(readCO2);
  co2_value = getCo2(response);
  Serial1.print(co2_value);  
  Serial1.print(",");
  Serial.print(co2_value);  
  Serial.print(",");
  delay(100);
  return co2_value;
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

void relh(int quiet) {
  rh = htu.readHumidity();
  if (quiet == 0) {
    Serial1.print("\"relative_humidity\": ");
    Serial.print("\"relative_humidity\": ");
  }
  Serial1.print(rh);  
  Serial1.print(",");
  Serial.print(rh);  
  Serial.print(",");
}

void temp(int quiet) {
  temperature = htu.readTemperature();
  if (quiet == 0) {
    Serial1.print("\"temperature\": ");
    Serial.print("\"temperature\": ");
  }
  Serial1.print(temperature);  
  Serial1.print(",");
  Serial.print(temperature);  
  Serial.print(",");
}

/* void relh2(int quiet) {

  Wire.beginTransmission(0x40); // 7 bit address
  delay(wait2);
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
  if (quiet == 0) {
    Serial1.print("\"relative_humidity\": ");
    Serial.print("\"relative_humidity\": ");
  }
  Serial1.print(rh);  
  Serial1.print(",");
  Serial.print(rh);  
  Serial.print(",");
}

void temp2(int quiet) {
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
  if (quiet == 0) {
  Serial1.print("\"temperature\": ");
  Serial.print("\"temperature\": ");
  }
  Serial1.print(temperature);  
  Serial1.print(",");
  Serial.print(temperature);  
  Serial.print(",");
}
*/

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
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("TMP006 contactless temperature sensor calibration value from EEPROM: ");
#endif  
//  tmp006_cal_S = call_eeprom(240)*((cal_pulses-1)/4);
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




