// implement MarkTs suggestion - comparing the smaller values (not huge values) when running micros()
// you cant't start the 2nd interrupt during the first!  It has to be done in the main loop



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

#define DEBUG

// SD CARD ENABLE AND SET
#include <Time.h>   // enable real time clock library
#include <Wire.h>
#include <EEPROM.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <Adafruit_Sensor.h>
#include <Adafruit_TMP006.h>
#include <Adafruit_TCS34725.h>
//#include <sleep.h>

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
char filename[13] = "ALGAE"; // Base filename used for data files and directories on the SD Card
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

// DIRKF VARIABLES
int drepeatrun = 1; // number of times the measurement is repeated
int ddelayruns = 0; // millisecond delay between each repeated run
int dmeasurements = 3; // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
int drunlength = 2; // in seconds... minimum = cyclelength
volatile unsigned long dpulselengthon = 30; // pulse length in us... minimum = 6us
volatile float dcyclelength = 10000; // time between cycles of pulse/actinicoff/pulse/actinic on in us... minimum = pulselengthon + 7.5us
volatile int dactinicoff = 500; // in us... length of time actinic is turned off
volatile int dsaturatingcycleon = 50; // The cycle number in which the saturating light turns on (set = 0 for no saturating light) - NOTE! This number is twice the number of the stored value (so this counts 200 cycles to produce a graph with only 100 points, because values are saved in alternating cycles)... so if you expect to have 100 graphed points and you want saturating to start at 25 and end at 50, then set it here to start at 50 and end at 100.
volatile int dsaturatingcycleoff = 100; //The cycle number in which the saturating light turns off
const char* dirkfending = "-D.CSV"; // filename ending for the basicfluor subroutine - just make sure it's no more than 6 digits total including the .CSV
int edge = 2; // The number of values to cut off from the front and back edges of Fm, Fs, Fd.  For example, if Fm starts at 25 and ends at 50, edge = 2 causes it to start at 27 and end at 48.
int* ddatasample1;
int* ddatasample2; 
int* ddatasample3;
int* ddatasample4;

/*
              d
 <------------------------->
 a
 <->
 | |   b  | |     c    | |        
 | |<---->| |<-------->| |       
 --  ------- ------------  -------...
 */

// ps1 VARIABLES
int ps1_repeats =             1; // number of times to repeat the entire run (so if averages = 6 and repeats = 3, total runs = 18, total outputted data = 3)
int ps1_averages =            1; // number of runs to average
int ps1_measurements =        3; // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
volatile int ps1_meas_light =          measuringlight1;
volatile int ps1_act_light =           actiniclight1;
volatile int ps1_red_light =           measuringlight2;
volatile int ps1_alt1_light =          measuringlight2;
volatile int ps1_alt2_light =          measuringlight2;
volatile int ps1_pulsesize =           50; // measured in microseconds
volatile int ps1_actintensity1 =       255;
volatile int ps1_actintensity2 =       10;
volatile int ps1_measintensity =       255; // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
volatile int ps1_pulses [] =           {
  100,100,100,100,100,100}; // Maximum pulses per cycle 100
volatile int ps1_pulsedistance [][2] = {
  {
    6000,3000    }
  ,{
    6000,3000    }
  ,{
    6000,3000    }
  ,{
    6000,3000    }
  ,{
    6000,3000    }
  ,{
    6000,3000    }
}; // measured in us. Minimum 200us
volatile int ps1_act [] =              {
  LOW,HIGH,2,2,HIGH,LOW}; // 2 is off.  "LOW" is ps1_actintensity1, "HIGH" is ps1_actintensity2.  May use this for combined actinic / saturation pulses  .  NOTE! You may set the intensity at any value during the run even if it's not preset - however, the LED intensity rise time is a few milliseconds.  The rise time on the preset values is in the nanoseconds range.
volatile int ps1_alt1 [] =             {
  LOW,LOW,LOW,LOW,LOW,LOW}; // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is ps1_actintensity1, HIGH is ps1_actintensity2. 
volatile int ps1_alt2 [] =             {
  LOW,LOW,LOW,LOW,LOW,LOW}; // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is ps1_actintensity1, HIGH is ps1_actintensity2. 
volatile int ps1_red [] =              {
  LOW,LOW,LOW,LOW,LOW,LOW}; // If set to measuring or calibrating light, "HIGH" is on and "LOW" is off. If set to actinic light, LOW is ps1_actintensity1, HIGH is ps1_actintensity2. 
volatile int ps1_edge = 2; // The number of values to cut off from the front and back edges of Fm, Fs, Fd.  For example, if Fm starts at 25 and ends at 50, edge = 2 causes it to start at 27 and end at 48.  Also note that one could vary the intensity of 
volatile int ps1_a; // maximum at first peak after actinic and actinic pulse (cycle 2)
volatile int ps1_b; // maximum at second peak after far red and actinic pulse (cycle 5)
volatile int* ps1_datasample1;
volatile int* ps1_datasample2; 
volatile int* ps1_datasample3;
volatile int* ps1_datasample4;
volatile int ps1_total_cycles = sizeof(ps1_pulses)/sizeof(ps1_pulses[0])-1; // (start counting at 0!)
volatile int repeats = 0; // current repeat number
volatile int cycle = 0; // current cycle number (start counting at 0!)
volatile int pulse = 0; // current pulse number
volatile int pulse1 = 0; // current pulse number of measuring pulse 1
volatile int pulse2 = 0; // current pulse number of measuring pulse 2

//BASIC FLUORESCENCE VARIABLES
int brepeatrun = 1;
int bmeasurements = 4; // # of measurements per pulse (min 1 measurement per 6us pulselengthon)
unsigned long bpulselengthon = 25; // pulse length in us... minimum = 6us
float bcyclelength = .01; // in seconds... minimum = pulselengthon + 7.5us
float brunlength = 1.5; // in seconds... minimum = cyclelength
const char* basicfluorending = "-B.CSV"; // filename ending for the basicfluor subroutine - just make sure it's no more than 6 digits total including the .CSV
int bsaturatingccycleon = 50; //The cycle number in which the saturating light turns on (set = 0 for no saturating light)
int bsaturatingcycleoff = 100; //The cycle number in which the actinic light turns off
int* bdatasample;

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
int wait = 200; // typical delay to let device finish working before requesting the data
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
volatile float data1f, data2f, data3f, data4f, irtinvalue, irtapevalue, rebeltapevalue, rebeltinvalue, irsamplevalue, rebelsamplevalue, baselineir, dataaverage, caldataaverage1, caldataaverage2, rebelslope, irslope, baseline = 0;
char filenamedir[13];
char filenamelocal[13];
volatile int data0=0, data1=0, data2=0, data3=0, data4=0, data5=0, data6=0, data7=0, data8=0, data9=0, data10=0, off, on, caldata1, caldata2, caldata3, caldata4, analogresolutionvalue, dpulse1count, dpulse1noactcount, dpulse2count;
int i=0, j=0, k=0,z=0,y=0,q=0,x=0,p=0; // Used as a counters
int* caldatatape; 
int* caldatatin; 
int* caldatasample;
int* rebeldatasample;
int val=0, cal=0, cal2=0, cal3=0, val2=0, flag=0, flag2=0, keypress=0, protocol, protocols;
IntervalTimer timer0, timer1, timer2;
char c;

void setup() {
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


int Protocol() {
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

  //Serial1.println("Please select a 3 digit protocol code to begin a new protocol");
  //Serial1.println("");


  Serial.println("Please select a 3 digit protocol code to begin");
  Serial.println("(002 for light testing)");
  Serial.println("(001 for DIRKF / PMF, RGB light, and CO2 measurement)");  
  Serial.println("");

  while (Serial1.available()<3 && Serial.available()<3) {
  }
  protocol = Protocol(); // Retreive the 3 digit protocol code 000 - 999
  Serial.print(protocol);

  switch(protocol) {
  case 999:        // END TRANSMISSION
    break;
  case 998:        // NULL RETURN
    Serial.println("");
    Serial.println("nothing happens - please use bluetooth for serial communication!");
    Serial1.println("");
    Serial1.println("nothing happens");
    break;
  case 000:        // CALIBRATION
    calibration();
    break;
  case 001:        // DIRK-F
    //  Serial1.println();
    //Begin JSON file printed to bluetooth on Serial1
    Serial1.print("{\"device_version\": 1,");
    Serial.println();
    dirkf();
    Serial.println();
    //end JSON file printed to bluetooth on Serial1
    Serial1.print("}");
    break;
  case 003:        // LEAF TEMP
    ps1();
    break;
  case 004:        // TEMP / RH
    temp();
    relh();
    break;
  case 005:        // LIGHT METER
    lightmeter();
    break;
  case 006:        // CO2 CALIBRATION
    CO2cal();
    break;
  case 007:        // eeprom tests
    Co2();
    break;
  case 010:        // eeprom tests
    leaftemp();
    break;
  case 011:        // eeprom tests
    lightmeter();
    break;
  case 012:        // eeprom tests
    ps1();
    break;
  case 002:        // eeprom tests
    lighttests();
    break;
  }
}

void varioustests() {

  // ERROR CHECKS;
  /*
  int ps1_cycles [] = {sizeof(ps1_pulses)/sizeof(ps1_pulses[0]),sizeof(ps1_act)/sizeof(ps1_act[0]),sizeof(ps1_sat)/sizeof(ps1_sat[0]), sizeof(ps1_red)/sizeof(ps1_red[0])}; // count the number of pulses from user inputted information above 
   Serial.println(ps1_cycles[0]); 
   Serial.println(ps1_cycles[1]); 
   Serial.println(ps1_cycles[2]); 
   Serial.println(ps1_cycles[3]); 
   for 
   Serial.println("You have incorrectly set the pulse cycles - the number of sat, act, and red pulses should be equal to each other");
   }
   else { 
   Serial.println("Pulse cycles set correctly");
   Serial.println(ps1_cycles[0]);
   }
   */
  /*
    // can I initialize a measuring light on HIGH by refering to a string variable to input the HIGH value?
   Serial.println("test lights on/off with HIGH and LOW signals...");
   Serial.println("actinic:");
   delay(1000);
   
   Serial.println("should be on high...");
   if (ps1_act[1] == 0) {
   digitalWriteFast(ps1_act_light, LOW);
   }
   else digitalWriteFast(actiniclight_intensity_switch, ps1_act[1]);
   delay(2000);
   
   Serial.println("should be on low...");
   if (ps1_act[0] == 0) {
   digitalWriteFast(ps1_act_light, LOW);
   }
   else digitalWriteFast(actiniclight_intensity_switch, ps1_act[0]);
   delay(2000);
   
   Serial.println("should be off...");
   if (ps1_act[2] == 0) {
   digitalWriteFast(ps1_act_light, LOW);
   Serial.println("we kmade it!");
   }
   else digitalWriteFast(actiniclight_intensity_switch, ps1_act[2]);
   delay(2000);
   
   Serial.println("meas:");    
   Serial.println("switch between 255 and 0...");
   digitalWriteFast(ps1_meas_light, HIGH);
   analogWrite(measuringlight_pwm, ps1_meas[0]);
   delay(2000);
   Serial.println("should be off...");
   digitalWriteFast(measuringlight_pwm, ps1_meas[2]);
   delay(2000);
   digitalWriteFast(ps1_meas_light, LOW);
   
   Serial.println("alt1:");    
   Serial.println("should be on again...");
   digitalWriteFast(ps1_alt1_light, ps1_alt1[0]);
   delay(2000);
   Serial.println("should be off...");
   digitalWriteFast(ps1_alt1_light, ps1_alt1[2]);
   delay(2000);
   
   Serial.println("alt2:");    
   Serial.println("should be on again...");
   digitalWriteFast(ps1_alt2_light, ps1_alt2[0]);
   delay(2000);
   Serial.println("should be off...");
   digitalWriteFast(ps1_alt2_light, ps1_alt2[2]);
   delay(2000);
   
   Serial.println("red:");    
   Serial.println("should be on again...");
   digitalWriteFast(ps1_red_light, ps1_red[0]);
   delay(2000);
   Serial.println("should be off...");
   digitalWriteFast(ps1_red_light, ps1_red[2]);
   delay(2000);
   // RESULTS: Yep - works great!
   */

  // Measuring how long it takes to send a message via USB or Bluetooth
  start1 = micros();
  Serial.print("[");
  Serial.print(70000);
  Serial.print(",");
  Serial.print("]");
  end1 = micros();
  Serial.print("It took this much time to write that to USB Serial: ");
  Serial.println(end1-start1);

  start1 = micros();
  Serial1.print("[");
  Serial1.print(70000);
  Serial1.print("]");
  Serial1.print(",");
  end1 = micros();
  Serial.print("It took this much time to write that to Bluetooth Serial (Serial1): ");
  Serial.println(end1-start1);
  // To write this it takes 21us for USB, and 32us for bluetooth.  So I think 100us limit on pulse distance seems conservative and reasonable



  // Measuring how long it takes to allocate memory
  start1 = micros();
  ps1_datasample1 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   
  ps1_datasample2 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   
  ps1_datasample3 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   
  ps1_datasample4 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   
  end1 = micros();

  Serial.print("It took this much time to write that to allocate memory: ");
  Serial.println(end1-start1);
  delay(100);
  start1 = micros();
  free(ddatasample1); // release the memory allocated for the data
  free(ddatasample2); // release the memory allocated for the data
  free(ddatasample3); // release the memory allocated for the data
  free(ddatasample4); // release the memory allocated for the data
  end1 = micros();
  Serial.print("It took this much time to write that to release memory: ");
  Serial.println(end1-start1);
  // to allocate 4 datasamples at 150 ints each (600 ints total), it takes 38us (for 1 datasample it's 11us).  To release the same 4, it takes 5us.
  // seems safe to give 100us time for this action as well... though if one were to allocate more than 10 datasamples, it may take >100us.

  //Serial1.println("Please select a 3 digit protocol code to begin a new protocol");
  //Serial1.println("");




  // Measuring how long it takes to pull a value from an int array (like {50,20,30,10});
  start1 = micros();
  int me2 = ps1_pulses [0];
  end1 = micros();
  Serial.print("It took this much time to write that to pull a datapoint from an int array: ");  
  Serial.println(end1-start1);
  delay(100);



  // Test to see if actinic light will turn on full if no pwm has been set  
  delay(2000);
  Serial.println("actinic light switch high");    
  digitalWriteFast(actiniclight_intensity_switch, HIGH);
  digitalWriteFast(actiniclight1, HIGH);
  delay(2000);
  digitalWriteFast(actiniclight1, LOW);
  Serial.println("actinic light switch low");    
  digitalWriteFast(actiniclight_intensity_switch, LOW);
  digitalWriteFast(actiniclight1, HIGH);
  delay(2000);
  digitalWriteFast(actiniclight1, LOW);

  // Now set PWM and see what happens:
  analogWrite(actiniclight_intensity1, 255);
  analogWrite(actiniclight_intensity2, 255);
  Serial.println("actinic light switch high with pwm on");    
  digitalWriteFast(actiniclight_intensity_switch, HIGH);
  digitalWriteFast(actiniclight1, HIGH);
  delay(2000);
  digitalWriteFast(actiniclight1, LOW);
  Serial.println("actinic light switch low with pwm on");    
  digitalWriteFast(actiniclight_intensity_switch, LOW);
  digitalWriteFast(actiniclight1, HIGH);
  delay(2000);
  digitalWriteFast(actiniclight1, LOW);
  // RESULTS: When no pwm is set, regadlress of the position of the intensity switch actinic light does not light up.  That means that 255 (full on) must be one of your intensity presets if you want to use it.



  // how long does it take for the timer.begin function to take place, and timer.end?
  Serial.println("does this work or not?");

  start1 = micros();
  timer0.begin(ps1_pulse1,ps1_pulsedistance[cycle][2]); // Begin firsts pulse      
  end1 = micros();
  Serial.print("It took this much time to start the timer: ");  
  Serial.println(end1-start1);
  delay(100);
  start1 = micros();
  timer0.end();
  end1 = micros();
  Serial.print("It took this much time to stop the timer: ");  
  Serial.println(end1-start1);
  // RESULTS: Took 6us to start the timer, 3us to stop the timer
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

    choose = Protocol();
    Serial.println(choose);

    if (choose<30) {
      Serial.println("First actinic intensty switch high, then actinic intensity switch low");
      delay(1000);
      for (y=0;y<256;y++) {
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
        for (x=0;x<500;x++) {
          Serial.println(analogRead(A10));
          delay(10);
        }
        break;

      case 1711:
        for (x=0;x<500;x++) {
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

  Serial.print("Color Temp: "); 
  Serial.print(colorTemp, DEC); 
  Serial.print(" K - ");
  Serial.print("Lux: "); 
  Serial.print(lux, DEC); 
  Serial.print(" - ");
  Serial.print("R: "); 
  Serial.print(r, DEC); 
  Serial.print(" ");
  Serial.print("G: "); 
  Serial.print(g, DEC); 
  Serial.print(" ");
  Serial.print("B: "); 
  Serial.print(b, DEC); 
  Serial.print(" ");
  Serial.print("C: "); 
  Serial.print(c, DEC); 
  Serial.print(" ");
  Serial.println(" ");
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
  Serial.print("Co2 ppm = ");
  Serial.println(valCO2);
  Serial1.print("\"co2_content\": ");
  Serial1.print(valCO2);  
  Serial1.print(",");
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
  delay(wait);

  // Print response and convert to Celsius:
  Wire.requestFrom(0x40, 2);
  byte byte1 = Wire.read();
  byte byte2 = Wire.read();
  rhval = byte1;
  rhval<<=8; // shift byte 1 to bits 1 - 8
  rhval+=byte2; // put byte 2 into bits 9 - 16
  Serial.print("relative humidity in %: ");
  rh = 125*(rhval/pow(2,16))-6;
  Serial.println(rh);
  Serial1.print("\"relative_humidity\": ");
  Serial1.print(rh);  
  Serial1.print(",");
}

void temp() {
  Wire.beginTransmission(0x40); // 7 bit address
  Wire.send(0xF3); // trigger temp measurement
  Wire.endTransmission();
  delay(wait);

  // Print response and convert to Celsius:
  Wire.requestFrom(0x40, 2);
  byte byte1 = Wire.read();
  byte byte2 = Wire.read();
  tempval = byte1;
  tempval<<=8; // shift byte 1 to bits 1 - 8
  tempval+=byte2; // put byte 2 into bits 9 - 16
  Serial.print("Temperature in Celsius: ");
  temperature = 175.72*(tempval/pow(2,16))-46.85;
  Serial.println(temperature);
  Serial1.print("\"temperature\": ");
  Serial1.print(temperature);  
  Serial1.print(",");
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
}

int savecalibration(float calval, int loc) {
  char str [10];
  calval = calval*1000000;
  calval = (int) calval;
  itoa(calval, str, 10);
  for (i=0;i<10;i++) {
    EEPROM.write(loc+i,str[i]);
    //    Serial1.print(str[i]);
    char temp = EEPROM.read(loc+i);
  }
  Serial1.println(str);
}

int callcalibration(int loc) { 
  char temp [10];
  float calval;
  for (i=0;i<10;i++) {
    temp[i] = EEPROM.read(loc+i);
  }
  calval = atoi(temp);
  calval = calval / 1000000;
  Serial.print(calval,4);
  return calval;
}

int protocol_runtime(volatile int protocol_pulses[], volatile int protocol_pulsedistance[][2], volatile int protocol_total_cycles) {
  int total_time = 0;
  for (x=0;x<protocol_total_cycles;x++) {
    total_time += protocol_pulses[x]*(protocol_pulsedistance[x][0]+protocol_pulsedistance[x][1]);
  }
  return total_time;
}

void ps1() {
  for (x=0;x<ps1_repeats;x++) {                                                                        // Repeat the entire measurement this many times  
    for (y=0;y<ps1_averages;y++) {                                                                     // Average this many measurements together to yield a single measurement output
      while (cycle != ps1_total_cycles | pulse != ps1_pulses[ps1_total_cycles]) {                      // Do the following until the last pulse of the last cycle...
        if (cycle == 0 && pulse == 0) {                                                                // if it's the beginning of a measurement, then...
          calibrationsample();                                                                         // Run calibration
          analogReadAveraging(ps1_measurements);                                                       // set analog averaging (ie ADC takes one signal per ~3u)
          digitalWriteFast(calibratinglight_pwm, 255);                                                 // turn on calibrating light pwm
          analogWrite(actiniclight_intensity1, ps1_actintensity1);                                     // set intensities for each of the lights
          analogWrite(actiniclight_intensity2, ps1_actintensity2);
          analogWrite(ps1_meas_light, ps1_measintensity);
          delay(5);                                                                                    // wait a few milliseconds so that the actinic pulse presets can stabilize

          Serial1.print("\"raw\": [");                                                                // Start the beginning of this variable in JSON format
          Serial.print("\"raw\": [");
          starttimer0 = micros();
          timer0.begin(ps1_pulse1,ps1_pulsedistance[cycle][0]);                                       // Begin firsts pulse
          while (micros()-starttimer0<ps1_pulsesize) {}                                               // wait a full pulse size, then...                                                                                          
          timer1.begin(ps1_pulse2,ps1_pulsedistance[cycle][0]);                                       // Begin second pulse
        }  
        noInterrupts();                                                                             // turn off interrupts while we send the volatile variables off and on affected by the timers
        else if (on == 1 && off == 1) {                                                             // if ALL pulses happened, then...
        interrupts();
          pulse++;                                                                                     // progress the pulse counter
     // also change the measuring light type here...
 
          Serial.print(ps1_total_cycles);                                                              // uncomment to see the cycle and pulse counter progression
          Serial.print(",");  
          Serial.print(cycle);
          Serial.print(",");
          Serial.print(ps1_pulses[cycle]);
          Serial.print(",");
          Serial.println(pulse);
          /*
          noInterrupts();                                                                             // turn off interrupts while we send the volatile variables data1 and data2 affected by the timers
          Serial1.print("[");                                                                          // uncomment to send the data to bluetooth and/or USB
          Serial1.print(data1);
          Serial1.print(",");
          Serial1.print(data2);
          Serial1.print("]");
          interrupts();
           */
          noInterrupts();                                                                             // turn off interrupts while we send the volatile variables data1 and data2 affected by the timers
          Serial.print("[");                                                                          // uncomment to send the data to bluetooth and/or USB
          Serial.print(data1);
          Serial.print(",");
          Serial.print(data2);
          Serial.print("]");    
          interrupts();                                                                               // turn interrupts back on
        }
        if (pulse == ps1_pulses[cycle]) {
          pulse = 0;
          cycle++;
        }
      }
    }
    timer0.end();                                                                                // if it's the last cycle and last pulse, then... stop the timers
    timer1.end();
    digitalWriteFast(ps1_meas_light, LOW);                                                      // make sure remaining lights are off
    digitalWriteFast(ps1_act_light, LOW);
    digitalWriteFast(ps1_alt1_light, LOW);
    digitalWriteFast(ps1_alt2_light, LOW);
    digitalWriteFast(ps1_red_light, LOW);
    cycle = 0;                                                                                  // reset counters
    pulse = 0;
    pulse1 = 0;
    pulse2 = 0;
  }
}

void ps1_pulse1() {
// IF WE CAN SKIP A BEAT, GREAT!  OTHERWISE, ADD AN IF STATEMENT HERE BASED ON PULSE COUNTS
// USE NAMESPACES... THAT WAY YOU ONLY NEED 1 OF THESE SETS OF PULSES, AND JUST SWITCH NAMESPACES AT THE TOP TO CHANGE THE VARIABLE REFERENCE



  digitalWriteFast(ps1_meas_light, HIGH);
  if (pulse == 0) {                                                                            // if it's the first pulse of a cycle, then change sat, act, far red, alt1 and alt2 values as per array's set at beginning of the file
    if (ps1_act[cycle] == 2) {
      digitalWriteFast(ps1_act_light, LOW);
      //      Serial.print("light off");
    }
    else {
      digitalWriteFast(actiniclight_intensity_switch, ps1_act[cycle]);
      digitalWriteFast(ps1_act_light, HIGH);
      //      Serial.print("light on");
    }
    digitalWriteFast(ps1_alt1_light, ps1_alt1[cycle]);    
    digitalWriteFast(ps1_alt2_light, ps1_alt2[cycle]);
    digitalWriteFast(ps1_red_light, ps1_red[cycle]);
  }  
  data1 = analogRead(detector1);
  on++;
}

void ps1_pulse2() {
  digitalWriteFast(ps1_meas_light, LOW);
  off++;
}

void ps1_calculations() {                                                                 // USER PRINTOUT OF TEST RESULTS

  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
    Serial.print(ddatasample1[i]);
    Serial.print(",");
  }
  Serial.println(",");

  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
    Serial.print(ddatasample2[i]);
    Serial.print(",");
  }
  Serial.println(",");

  for (i=edge;i<(dsaturatingcycleon/2-edge);i++) { // Print the results!
    Fs += ddatasample3[i];
  }
  Fs = Fs / (dsaturatingcycleon/2-edge);

  for (i=edge;i<(dsaturatingcycleon/2-edge);i++) { // Print the results!
    Fd += ddatasample2[i];
  }
  Fd = Fd / (dsaturatingcycleon/2-edge);

  for (i=dsaturatingcycleon/2+edge;i<(dsaturatingcycleoff/2-edge);i++) { // Print the results!
    Fm += ddatasample3[i];
  }
  Fm = Fm / (dsaturatingcycleoff/2-dsaturatingcycleon/2-2*edge);

  Phi2 = (Fm-Fs)/Fm;

  Serial1.print("\"phi2_raw\": [");

  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
    Serial.print(ddatasample3[i]);
    Serial.print(",");
    Serial1.print(ddatasample3[i]);
    if (i<(dpulse2count/2)-1) {
      Serial1.print(",");
    }
    else {
    }
  }
  Serial.println(",");

  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
    Serial.print(ddatasample4[i]);
    Serial.print(",");
  }
  Serial.println();
  Serial1.print("],");
  Serial1.print(" \"photosynthetic_efficiency_phi2\": ");
  Serial1.print(Phi2,3);
  Serial1.print(",");
  Serial1.print(" \"fs\": ");
  Serial1.print(Fs);
  //  Serial1.print(",");
  //  Serial1.print(" \"baseline\": ");
  //  Serial1.print(baseline);
}




