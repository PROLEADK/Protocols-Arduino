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
 
 for dirkf you need to put the counters in the inside loop - now they are on the outside loop which won't work for loop >1.
 update Basic Fluor
 Add methods for CO2 measurement, light, humidity, etc.
 Add Serial.print for all available methods at top
 Figure out low power mode
 
 RECENT UPDATES:
 * Changed input structure of file so you choose between 000 - 999 to select what type of measurement you want to run
 * No longer saves any data to SD card (left in the SD card initialization in case we want to add it back in, but sd card is no longer requires)
 * All data is pushed to bluetooth, according to defined structure (<DATA>...</DATA>... etc.)
 * No longer pushes time data from the Teensy itself - time will be pulled from the phone
 
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
// CO2 sensor hardware http://co2meters.com/Documentation/Datasheets/DS-S8-PSP1081.pdf
// CO2 sensor communication http://co2meters.com/Documentation/Datasheets/DS-S8-Modbus-rev_P01_1_00.pdf


// SD CARD ENABLE AND SET
#include <SdFat.h>
#include <SdFatUtil.h> // use functions to print strings from flash memory
#include <Time.h>   // enable real time clock library
#include <Wire.h>
#include <EEPROM.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
//#include <sleep.h>

const uint8_t SD_CHIP_SELECT = SS;

SdFat sd;
#define error(s) sd.errorHalt_P(PSTR(s))  // store error strings in flash to save RAM

// KEY VARIABLES USED IN THE PROTOCOL

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

// WASP VARIABLES
int wrepeatrun = 2; // # of times to repeat set1, set2, and set3 combined (limited based on dynamic array size of datasample[] - generally keep low at <5)
int wmeasurements = 1; // FOR WASP LEAVE AT 1!  # of measurements per pulse (min 1 measurement per 6us pulselengthon)
int wnumberpulses1 = 30; // number of pulses in set 1
int wnumberpulses2 = 30; // number of pulses in set 2
int wnumberpulses3 = 30; // number of pulses in set 3
unsigned long wpulselengthon1 = 25; // pulse length in set 1 us... minimum = 6us
unsigned long wpulselengthon2 = 25; // pulse length in set 2 us... minimum = 6us
unsigned long wpulselengthon3 = 25; // pulse length in set 3 us... minimum = 6us
float wcyclelength1 = .1; // length of pulse cycle, set 1.  in seconds... minimum = pulselengthon + 7.5us
float wcyclelength2 = .01; // length of pulse cycle, set 2.   in seconds... minimum = pulselengthon + 7.5us
float wcyclelength3 = .001;// length of pulse cycle, set 3.  in seconds... minimum = pulselengthon + 7.5us
const char* waspending = "-W.CSV"; // filename ending for the wasp subroutine - just make sure it's no more than 6 digits total including the .CSV
int* wdatasample;
unsigned long wdatasampleaverage1;
unsigned long wdatasampleaverage2;
unsigned long wdatasampleaverage3;
unsigned long wdatasampleaverage4;

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

//BASIC FLUORESCENCE VARIABLES
int brepeatrun = 1;
int bmeasurements = 4; // # of measurements per pulse (min 1 measurement per 6us pulselengthon)
unsigned long bpulselengthon = 25; // pulse length in us... minimum = 6us
float bcyclelength = .01; // in seconds... minimum = pulselengthon + 7.5us
float brunlength = 1.5; // in seconds... minimum = cyclelength
const char* basicfluorending = "-B.CSV"; // filename ending for the basicfluor subroutine - just make sure it's no more than 6 digits total including the .CSV
int bsaturatingcycleon = 30; //The cycle number in which the saturating light turns on (set = 0 for no saturating light)
int bsaturatingcycleoff = 100; //The cycle number in which the saturating light turns off
int* bdatasample;

// KEY CALIBRATION VARIABLES
unsigned long calpulsecycles = 50; // Number of times the "pulselengthon" and "pulselengthoff" cycle during calibration (on/off is 1 cycle)
// data for measuring and saturating pulses --> to calculate total time=pulsecycles*(pulselengthon + pulselengthoff)
unsigned long calpulselengthon = 30; // Pulse LED on length for calibration in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
unsigned long calpulselengthoff = 49970; // Pulse LED off length for calibration in uS (minimum = 20us + any additional operations which you may want to call during that time).
unsigned long cmeasurements = 4; // # of measurements per pulse (min 1 measurement per 6us pulselengthon)

// PIN DEFINITIONS AND TEENSY SETTINGS
float reference = 1.2; // The reference (AREF) supplied to the ADC - currently set to INTERNAL = 1.2V
int analogresolution = 16; // Set the resolution of the analog to digital converter (max 16 bit, 13 bit usable)  
/*
// for old unit
 int measuringlight1 = 3; // Teensy pin for measuring light
 int saturatinglight1 = 4; // Teensy pin for saturating light
 int calibratinglight1 = 2; // Teensy pin for calibrating light
 int actiniclight1 = 5; // Teensy pin for actinic light
 */
// for new unit
int measuringlight1 = 15; // Teensy pin for measuring light
int measuringlight2 = 16; // Teensy pin for measuring light
int saturatinglight1 = 20;
int calibratinglight1 = 14;
int actiniclight1; // Teensy pin for actinic light - not currently set

int measuringlight_pwm = 23;
int calibratinglight1_pwm = 9;
int saturatinglight1_intensity2 = 3;
int saturatinglight1_intensity1 = 4;
int saturatinglight1_intensity_switch = 5;
int detector1 = A10; // Teensy analog pin for detector

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
int co2calibration = 17; // manual CO2 calibration pin (CO2 sensor has auto-calibration, so manual calibration is only necessary when you don't want to wait for autocalibration to occur - see datasheet for details 
unsigned long valCO2;

// INTERNAL VARIABLES, COUNTERS, ETC.
int protocol;
volatile unsigned long start1,start1orig,end1, start3, end3, calstart1orig, calend1, start5, start6, start7, end5;
unsigned long pulselengthoncheck, pulselengthoffcheck, pulsecyclescheck, totaltimecheck, caltotaltimecheck;
volatile float data1f, data2f, data3f, data4f, irtinvalue, irtapevalue, rebeltapevalue, rebeltinvalue, irsamplevalue, rebelsamplevalue, baselineir, dataaverage, caldataaverage1, caldataaverage2, rebelslope, irslope, baseline = 0;
char filenamedir[13];
char filenamelocal[13];
volatile int data0=0, data1=0, data2=0, data3=0, data4=0, data5=0, data6=0, data7=0, data8=0, data9=0, data10=0, caldata1, caldata2, caldata3, caldata4, analogresolutionvalue, dpulse1count, dpulse1noactcount, dpulse2count;
int i = 0; // Used as a counter
int j = 0; // Used as a counter
int k = 0; // Used as a counter
int z = 0; // Used as a counter
int y = 0; // Used as a counter
int q = 0; // Used as a counter
int x = 0; // Used as a counter
int p = 0; // Used as a counter
int* caldatatape;
int* caldatatin;
int* caldatasample;
int* rebeldatasample;
int val = 0;
int cal = 0;
int cal2 = 0;
int cal3 = 0;  
int val2 = 0;
int flag = 0;
int flag2 = 0;
int keypress = 0;
IntervalTimer timer0;
IntervalTimer timer1;
IntervalTimer timer2;
SdFile file;
SdFile sub1;
SdBaseFile* dir = &sub1;
char c;
int protocols;
int manualadjust=80;

void setup() {

  Serial.begin(115200); // set baud rate for Serial communication to computer via USB
  Serial1.begin(115200); // set baud rate for bluetooth communication on pins 0,1
  Serial3.begin(9600);
  Wire.begin();
  delay(wait);

  Serial.println();
  Serial.println("Current protocol version:");  
  //  Serial.print("<PROTOCOLVERSION>");
  Serial.println(protocolversion);
  //  Serial.println("</PROTOCOLVERSION>");
  //  Serial.println();

  pinMode(measuringlight1, OUTPUT); // set pin to output
  pinMode(measuringlight2, OUTPUT); // set pin to output
  pinMode(saturatinglight1, OUTPUT); // set pin to output
  pinMode(calibratinglight1, OUTPUT); // set pin to output  
  //  pinMode(actiniclight1, OUTPUT); // set pin to output (currently unset)
  analogReadAveraging(1); // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u
  pinMode(detector1, EXTERNAL);
  analogReadRes(analogresolution);
  analogresolutionvalue = pow(2,analogresolution); // calculate the max analogread value of the resolution setting
  
  // ADD ANY OTHER STARTING INFO HERE

  /*
  // SET SYSTEM TIME IN ASCII "T" PLUS 10 DIGITS
   setSyncProvider((getExternalTime) Teensy3Clock.get);
   //    while (!Serial);  // Wait for Arduino Serial Monitor to open
   if(timeStatus()!= timeSet) {
   Serial.println("Unable to sync with the RTC");
   Serial.println("Unable to sync with the RTC");
   }
   else {
   Serial.println("RTC has set the system time"); // NOTE! Chris enter experiment start time stamp here
   Serial.println("RTC has set the system time"); // NOTE! Chris enter experiment start time stamp here
   }
   
   Serial.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
   Serial.println("(if you make a mistake, restart teensy and reenter)");
   Serial.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
   Serial.println("(if you make a mistake, restart teensy and reenter)");
   
  /*delay(1000);
   Serial.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
   Serial.println("(if you make a mistake, restart teensy and reenter)");
   Serial.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
   Serial.println("(if you make a mistake, restart teensy and reenter)");
   delay(1000);
   }
   
   //  Serial.println(Serial1.available());
   
   
   while (Serial1.available()<11) {
   // c = Serial1.read();
   //  Serial.print(Serial1.available());
   //  Serial.print(",");
   //  Serial.println(Serial1.peek());
   //  delay(500);
   
   }
   time_t t = processSyncMessage();
   Serial.print("'T' plus ASCII 10 digit time: ");
   Serial.println(t);
   Serial.print("Serial buffer size: ");
   Serial.println(Serial1.available());
   setTime(t);          
   Serial.print("UTC time: ");
   SerialPrintClock();  
   //  delay(3000);
   //  digitalClockDisplay();
   //  }
   Serial.println("");
   
   */

  /*
//  if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) sd.initErrorHalt(); // Set SD Card to full speed ahead!
   sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED);
   
   //PRINT SD CARD MEMORY INFORMATION
   PgmPrint("Free RAM: ");
   Serial.println(FreeRam());  
   
   PgmPrint("Volume is FAT");
   Serial.println(sd.vol()->fatType(), DEC);
   Serial.println();
   
   // list file in root with date and size
   PgmPrintln("Files found in root:");
   sd.ls(LS_DATE | LS_SIZE);
   Serial.println();
   
   // Recursive list of all directories
   PgmPrintln("Files found in all dirs:");
   sd.ls(LS_R);
   
   // PRINT OTHER INFORMATION
   wprintout();
   
   //CREATE NEW DIRECTORY OR DATA FILE, OR APPEND EXISTING  
   //If the primary file name does not yet exist as a file or folder, then create the folder and store all subsequent files (ie file-m.csv, file.c.csv...) in that folder
   //... if the primary file name does already exist as a file or folder, then create a new folder with that name and store subsequent files in it.
   
   strcpy(filenamedir, filename);
   Serial.println(sd.exists(filenamedir));
   
   if (sd.exists(filenamedir) == 1) {
   Serial.print("Press 1 to append the existing data files in directory ");
   Serial.print(filenamedir);
   Serial.println(" or press 2 to create a new directory.");    
   
   while (cal3 != 1 && cal3 !=2) {
   cal3 = Serial1.read()-48; // for some crazy reason, inputting 0 in the Serial Monitor yields 48 on the Teensy itself, so this just shifts everything by 48 so that when you input X, it's saved in Teensy as X.
   if (cal3 == 2) {
   for (j = 0; j<100; j++) {
   filenamedir[0] = j/10 + '0';
   filenamedir[1] = j%10 + '0';
   if (sd.exists(filenamedir) == 0) {
   break;
   }
   }
   Serial.print("OK - creating a new directory called:  ");
   Serial.println(filenamedir);
   SdFile sub1;
   sub1.makeDir(sd.vwd(), filenamedir); // Creating new folder
   Serial.print(".  Data files are saved in that directory as ");
   Serial.print(filename);
   Serial.println(" plus -(subroutine extension letter).CSV for each subroutine.");
   }
   else if (cal3 == 1) {
   for (j = 0; j<100; j++) {
   filenamedir[0] = j/10 + '0';
   filenamedir[1] = j%10 + '0';
   if (sd.exists(filenamedir) == 0) {
   filenamedir[0] = (j-1)/10 + '0';
   filenamedir[1] = (j-1)%10 + '0';
   break;
   }
   }
   Serial.print("Ok - appending most recently used files located in directory ");
   Serial.println(filenamedir);
   break;
   }
   }
   }
   else if (sd.exists(filenamedir) == 0) {
   
   Serial.print("OK - creating a new directory called ");
   Serial.print(filenamedir);
   Serial.print(".  Data files are saved in that directory as ");
   Serial.print(filenamedir);
   Serial.println(" plus -(subroutine extension letter).CSV for each subroutine.");
   sub1.makeDir(sd.vwd(), filenamedir); // Creating new folder
   } 
   */

  // engagepins();

}

void engagepins() {

  pinMode(measuringlight1, OUTPUT); // set pin to output
  pinMode(measuringlight2, OUTPUT); // set pin to output
  pinMode(saturatinglight1, OUTPUT); // set pin to output
  pinMode(calibratinglight1, OUTPUT); // set pin to output
  //  pinMode(actiniclight1, OUTPUT); // set pin to output (currently unset)
  pinMode(co2calibration, OUTPUT); // set pin to output  
  analogReadAveraging(1); // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u
  pinMode(detector1, EXTERNAL);
  analogReadRes(analogresolution);
  analogresolutionvalue = pow(2,analogresolution); // calculate the max analogread value of the resolution setting
}

/* - still need to fix idle mode and get it working
 void idle() {
 for (int i=0; i<46; i++) {
 pinMode(i, OUTPUT);
 }
 set_sleep_mode(SLEEP_MODE_IDLE);
 noInterrupts();
 digitalWrite(2, LOW);
 sleep_enable();
 interrupts();
 sleep_cpu();
 sleep_disable();
 }
 */

int Protocol() {
  int a;
  int c;
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
  delay(4000);
  Serial.print("");
  Serial.println("");  
  Serial.print("Expected run time: ");  
  Serial.print(drunlength*drepeatrun+3);
  Serial.println(" seconds");  
  temp();
  relh();
  Co2();
  dirkf();
  while (1) {
  };
}

void co2cal() {
  Serial.print("place detector in fresh air (not in house or building) for 30 seconds, then press any button. Make sure sensor environment is steady and calm!");
  Serial.print("place detector in fresh air (not in house or building) for 30 seconds, then press any button.  Make sure sensor environment is steady and calm!");
  while (Serial1.available()<1 && Serial.available()<1) {
  }
  Serial.print("please wait about 6 seconds");
  Serial.print("please wait about 6 seconds");
  digitalWriteFast(co2calibration, HIGH);
  delay(6000);
  digitalWriteFast(co2calibration, LOW);
  Serial.print("place detector in 0% CO2 air for 30 seconds, then press any button.  Make sure sensor environment is steady and calm!");
  Serial.print("place detector in 0% CO2 air for 30 seconds, then press any button.  Make sure sensor environment is steady and calm!");
  while (Serial1.available()<2 && Serial.available()<2) {
  }
  Serial.print("please wait about 20 seconds");
  Serial.print("please wait about 20 seconds");
  digitalWriteFast(co2calibration, HIGH);
  delay(20000);
  digitalWriteFast(co2calibration, LOW);
  Serial.print("calibration complete!");
  Serial.print("calibration complete!");
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
  Serial.print("Relative humidity: ");
  Serial.print("Relative humidity: ");
  rh = 125*(rhval/pow(2,16))-6;
  //  Serial.print("<VARIABLE10>");
  Serial.print(rh);
  //  Serial.println("</VARIABLE10>");
  Serial.print(rh);
  Serial.println(" %");
  Serial.println(" %");
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
  Serial.print("Temperature: ");
  Serial.print("Temperature: ");
  temperature = 175.72*(tempval/pow(2,16))-46.85;
  //  Serial.print("<VARIABLE11>");
  Serial.print(temperature);
  Serial.print(temperature);
  Serial.println(" C");
  Serial.println(" C");
  //  Serial.println("</VARIABLE11>");
}

void Co2() {
  requestCo2(readCO2);
  valCO2 = getCo2(response);
  Serial.print("Co2 level = ");
  Serial.print(valCO2);
  Serial.print("Co2 level = ");
  Serial.print(valCO2);
  Serial.println(" ppm");
  Serial.println(" ppm");
  Serial.println("");
  delay(2000);
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

void eeprom() {

  int joe = 395;
  char joe2 [10];
  itoa(joe, joe2, 10);

  int digs = numDigits(joe);
  for (i=0;i<digs;i++) {
    //    joe2[i] = (char) joe/pow(10,(i-1));
    //    joe = joe - joe2[i]*pow(10,(i-1));
    Serial.println(joe2[i]);
  }
  Serial.println("");
  Serial.println(joe);
  //  joe2[5] = joe/100000;
  //  joe2[4] = joe/10000;
  // joe2[3] = joe/1000;
  // joe2[2] = joe/100;
  // joe2[1] = joe/10;
  Serial.println(joe);
  Serial.println(numDigits(joe));
  delay(50);
  joe = (char) joe;
  Serial.println(joe);

  int jack;
  EEPROM.write(1,1);
  EEPROM.write(2,2);
  EEPROM.write(3,3);
  EEPROM.write(4,4);  
  jack = EEPROM.read(1);
  Serial.println(jack);
  jack = EEPROM.read(2);
  Serial.println(jack);
  jack = EEPROM.read(3);
  Serial.println(jack);
  jack = EEPROM.read(4);
  Serial.println(jack);

}

void calibration() {

  analogReadAveraging(cmeasurements); // set analog averaging (ie ADC takes one signal per ~3u)
  Serial.print("<RUNTIME>");
  Serial.print("5");
  Serial.println("</RUNTIME>");


  Serial.println("Place the shiny side of the calibration piece face up in the photosynq, and close the lid.");
  Serial.println("When you're done, press any key to continue");
  Serial1.flush();
  while (Serial1.read() <= 0) {
  }

  Serial.println("Thanks - calibrating...");
  calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtin()!
  calibrationtin();

  Serial.println("");
  Serial.println("Now place the black side of the calibration piece face up in the photosynq, and close the lid.");
  Serial.println("");  
  Serial.println("");

  Serial.println("When you're done, press any key to continue");
  Serial1.flush();
  while (Serial1.read() <= 0) {
  }

  Serial.println("Thanks - calibrating...");
  calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtape()!
  calibrationtape();
  Serial.println("Calibration finished!");
  Serial.println("");
  //  cal = 2;
}

/*
    if (cal == 2) {
 delay(50);
 Serial.println("Proceeding with normal measurement protocol");
 Serial.println("Place the sample in front of the light guide and press any key to continue");
 Serial1.flush();
 while (Serial1.read() <= 0) {
 }
 Serial.println("Thanks - running protocol...");
 while (1) {
 calibrationsample(); // order matters here - calibrationsample() must be BEFORE basicflour()
 Serial.println("Now running DIRKF");
 dirkf();
 //       dcalculations();
 //        Uncomment below for Basic Fluor or WASP methods
 //        Serial.println("Now running Basic Fluor");
 //        basicfluor();
 //        bcalculations();
 //        Serial.println("Now running WASP");
 //        wasp();
 //        wcalculations();
 Serial.println("");        
 Serial.println("Press any key to take another measurement, or press reset button to calibrate or start a new file");
 while (Serial1.read() <= 0) {
 }
 }      
 }
 
 else if (cal > 2) {
 delay(50);
 Serial.println("That's not a 1 or a 2!  Please enter either 1 for yes, or 2 for no");
 cal = 0;
 }
 }
 }
 */

void calibrationrebel() {

  // CALIBRATION REBEL
  // Short pulses for calibration using the measuring LED (rebel orange)

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

  /*
  file.open("RTAPE.CSV", O_CREAT | O_WRITE);
   file.seekSet(0);
   file.print(rebeltapevalue, 8);
   file.close();
   
   file.open("RTIN.CSV", O_CREAT | O_WRITE);
   file.seekSet(0);
   file.print(rebeltinvalue, 8);
   file.close();
   
   file.open("ITAPE.CSV", O_CREAT | O_WRITE);
   file.seekSet(0);
   file.print(irtapevalue, 8);
   file.close();
   
   file.open("ITIN.CSV", O_CREAT | O_WRITE);
   file.seekSet(0);
   file.print(irtinvalue, 8);
   file.close();
   
   file.open("RSLOP.CSV", O_CREAT | O_WRITE);
   file.seekSet(0);
   file.print(rebelslope, 8);
   file.close();
   
   file.open("ISLOP.CSV", O_CREAT | O_WRITE);
   file.seekSet(0);
   file.print(irslope, 8);
   file.close();
   
   Serial.print("IR slope: ");  
   Serial.print("<IRSLOPE>");    
   Serial.print(irslope, 8);
   Serial.println("</IRSLOPE>");    
   Serial.print("rebel slope: "); 
   Serial.print("<REBELSLOPE>");    
   Serial.println(rebelslope, 8);
   Serial.println("</REBELSLOPE>");    
   Serial.print("ir tin value: ");  
   Serial.print("<IRTIN>");    
   Serial.println(irtinvalue, 8);
   Serial.println("</IRTIN>");    
   Serial.print("ir tape value: ");  
   Serial.print("<IRTAPE>");    
   Serial.println(irtapevalue, 8);
   Serial.println("</IRTAPE>");    
   Serial.print("rebel tin value: ");  
   Serial.print("<REBELTIN>");    
   Serial.println(rebeltinvalue, 8);
   Serial.println("</REBELTIN>");    
   Serial.print("rebel tape value: ");  
   Serial.print("<REBELTAPE>");    
   Serial.println(rebeltapevalue, 8);
   Serial.println("</REBELTAPE>");    
   */
}


void calibrationsample() {

  // CALIBRATION SAMPLE
  // Flash calibrating light to determine how reflective the sample is to ~850nm light with the actual sample in place.  This has been tested with Luxeon Rebel Orange as measuring pulse.

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
  //  Serial.println(irsamplevalue);
  irsamplevalue = (float) irsamplevalue;
  irsamplevalue = (irsamplevalue / calpulsecycles); 
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    //    Serial.print(caldatasample[i]);
    //    Serial.print(", ");
    //    Serial.print(" ");  
  }
  //  Serial.println(" ");    
  //  Serial.print("the baseline sample reflectance value from calibration:  ");
  //  Serial.print("<CAL0>");
  //  Serial.print(irsamplevalue, 4);
  //  Serial.println("</CAL0>");

  // RETRIEVE STORED TIN AND TAPE CALIBRATION VALUES AND CALCULATE BASELINE VALUE

  /*
  int c;
   char local[12];
   
   Serial.println("Calibration values pulled from SD Card");
   
   file.open("ISLOP.CSV", O_READ);
   i = 0; // reset counter just in case
   while ((c = file.read()) > 0) {
   local[i] = (char) c;
   i++;
   }
   i = 0; // reset counter
   irslope = strtod(local, NULL);
   Serial.println(irslope, 8);
   file.close();
   
   file.open("RSLOP.CSV", O_READ);
   i = 0; // reset counter just in case
   while ((c = file.read()) > 0) {
   local[i] = (char) c;
   i++;
   }
   i = 0; // reset counter
   rebelslope = strtod(local, NULL);
   Serial.println(rebelslope, 8);
   file.close();
   
   file.open("ITIN.CSV", O_READ);
   i = 0; // reset counter just in case
   while ((c = file.read()) > 0) {
   local[i] = (char) c;
   i++;
   }
   i = 0; // reset counter
   irtinvalue = strtod(local, NULL);
   Serial.println(irtinvalue, 8);
   file.close();
   
   file.open("ITAPE.CSV", O_READ);
   i = 0; // reset counter just in case
   while ((c = file.read()) > 0) {
   local[i] = (char) c;
   i++;
   }
   i = 0; // reset counter
   irtapevalue = strtod(local, NULL);
   Serial.println(irtapevalue, 8);
   file.close();
   
   file.open("RTIN.CSV", O_READ);
   i = 0; // reset counter just in case
   while ((c = file.read()) > 0) {
   local[i] = (char) c;
   i++;
   }
   i = 0; // reset counter
   rebeltinvalue = strtod(local, NULL);
   Serial.println(rebeltinvalue, 8);
   file.close();
   
   file.open("RTAPE.CSV", O_READ);
   i = 0; // reset counter just in case
   while ((c = file.read()) > 0) {
   local[i] = (char) c;
   i++;
   }
   i = 0; // reset counter
   rebeltapevalue = strtod(local, NULL);
   Serial.println(rebeltapevalue, 8);
   
   file.close();
   sub1.close();
   
   */

  // CALL AND SAVE CALIBRATION VALUES FROM EEPROM AND RUN IR CALIBRATION ON CURRENT SAMPLE
  //  Serial.print("<CAL1>");
  rebeltapevalue = callcalibration(0);
  //  Serial.println("</CAL1>");
  //  Serial.print("<CAL2>");
  rebeltinvalue = callcalibration(10);
  //  Serial.println("</CAL2>");
  //  Serial.print("<CAL3>");
  irtapevalue = callcalibration(20);
  //  Serial.println("</CAL3>");
  //  Serial.print("<CAL4>");
  irtinvalue = callcalibration(30);
  //  Serial.println("</CAL4>");
  //  Serial.print("<CAL5>");
  rebelslope = callcalibration(40);
  //  Serial.println("</CAL5>");
  //  Serial.print("<CAL6>");
  irslope = callcalibration(50);
  //  Serial.println("</CAL6>");

  // CALCULATE BASELINE VALUE
  baseline = (rebeltapevalue+((irsamplevalue-irtapevalue)/irslope)*rebelslope);
}

void basicfluor() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

  calibrationsample();

  // Set saturating flash intensities via intensity1 and intensity2 (0 - 255)
  analogWrite(saturatinglight1_intensity1, 255); // set saturating light intensity
  analogWrite(saturatinglight1_intensity2, 255); // set saturating light intensity
  digitalWriteFast(saturatinglight1_intensity_switch, LOW); // tu rnintensity 1 on

  analogReadAveraging(bmeasurements); // set analog averaging (ie ADC takes one signal per ~3u)
  Serial.print("<RUNTIME>");
  Serial.print(brunlength*brepeatrun);
  Serial.print("</RUNTIME>");

  for (q=0;q<brepeatrun;q++) {

    /*
    strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
     strcat(filenamelocal, basicfluorending); // add the variable's extension defined in header
     sub1.open(filenamedir, O_READ);
     file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
     strcpy(filenamelocal,filename); // reset the localfilename variable;
     */

    // SAVE CURRENT TIME TO SD CARD
    //    printTime();

    Serial.println("check this out");
    Serial.println((brunlength/(bcyclelength))*sizeof(int));

    bdatasample = (int*)malloc((brunlength/(bcyclelength))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   

    starttimer0 = micros()+100; // This is some arbitrary reasonable value to give the Arduino time before starting
    starttimer1 = starttimer0+bpulselengthon;
    while (micros()<starttimer0) {
    }
    timer0.begin(bpulseon, bcyclelength*1000000); // LED on.  Takes about 1us to call "start" function in PITimer plus 5us per analogRead()
    while (micros()<starttimer1) {
    }
    timer1.begin(bpulseoff, bcyclelength*1000000); // LED off.
    timer2.begin(bstoptimers, brunlength*1000000); // turn off timers after runlength time

    // WAIT FOR TIMERS TO END (give it runlength plus a 10ms to be safe)
    delay(brunlength*1000+10);
    z = 0; // reset counter z

    free(bdatasample); // release the memory allocated for the data
    file.println("");
    file.close(); // close out the file
    sub1.close(); // close out the directory (you MUST do this - if you don't, the file won't save the data!)

    //delay(30000); // wait 30 seconds for things to mix around
    bcalculations();

    Serial.print("<END>");
  }
}

void bpulseon() {
  //    Serial.print(z);
  if (z==(bsaturatingcycleon-1)) { // turn on saturating light at beginning of measuring light
    digitalWriteFast(saturatinglight1, HIGH);
  }
  digitalWriteFast(measuringlight1, HIGH);
  data1 = analogRead(detector1);
  i = 0;
}

void bpulseoff() {

  // NOTE! for very short OFF cycles, just store the data in the datasample[], and write to 
  // the SD card at the end.  If OFF cycle is long enough (50us or more), then you can write
  // directly to the SD card.  The advantage is you are limited to ~1500 data points for
  // datasample[] before it becomes too big for the memory to hold.

  digitalWriteFast(measuringlight1, LOW);
  if (z==(bsaturatingcycleoff-1)) { // turn off saturating light at end of measuring light pulse
    digitalWriteFast(saturatinglight1, LOW);
  }
  bdatasample[z] = data1-baseline; 
  Serial.print(bdatasample[z]);
  Serial.print(",");
  //  file.print(bdatasample[z]);
  //  file.print(",");  
  data1 = 0; // reset data1 for the next round
  z=z+1;
}

void bstoptimers() { // Stop timers, close file and directory on SD card, free memory from datasample[], turn off lights, reset counter variables, move to next row in .csv file, 
  timer0.end();
  timer1.end();
  timer2.end();
  end1 = micros();
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(calibratinglight1, LOW);
  digitalWriteFast(saturatinglight1, LOW);
  z=0; // reset counters
  i=0;
  Serial.println("");
  Serial.print("Total run time is ~: ");
  Serial.println(end1-starttimer0);
}

// USER PRINTOUT OF TEST RESULTS
void bcalculations() {

  Serial.println("DATA - RAW ANALOG VALUES IN uV");
  for (i=0;i<(brunlength/bcyclelength);i++) {
    bdatasample[i] = 1000000*((reference*bdatasample[i])/(analogresolutionvalue*bmeasurements)); 
  }
  for (i=0;i<(brunlength/bcyclelength);i++) { // Print the results!
    Serial.print(bdatasample[i], 8);
    Serial.print(",");
  }
  Serial.println("");

  /*
  Serial.println("ALL DATA IN CURRENT DIRECTORY - BASELINE ADJUSTED VALUES");
   
   int16_t c;
   sub1.open(filenamedir, O_READ);
   strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
   strcat(filenamelocal, waspending);
   Serial.print("file directory: ");
   Serial.println(filenamedir);
   Serial.print("file name: ");
   Serial.println(filenamelocal);
   file.open(dir, filenamelocal, O_READ);
   while ((c = file.read()) > 0) Serial1.write((char)c); // copy data to the Serial port
   strcpy(filenamelocal,filename); // reset the localfilename variable;
   
   file.close(); // close out the file
   sub1.close(); // close out the director (you MUST do this - if you don't, the file won't save the data!)
   */

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial.println("");
  Serial.print("Size of the baseline:  ");
  Serial.print("<BASELINE>");
  Serial.println(baseline, 8);
  Serial.print("</BASELINE>");

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial.println("");
  Serial.println("GENERAL INFORMATION");
  Serial.println("");

  Serial.print("total run length (measuring pulses):  ");
  Serial.println(totaltimecheck);

  Serial.print("expected run length(measuring pulses):  ");
  Serial.println(brunlength/bcyclelength);

  Serial.print("total run length (calibration pulses):  ");
  Serial.println(caltotaltimecheck);

  Serial.println("");
  Serial.println("CALIBRATION DATA");
  Serial.println("");


  Serial.print("The baseline from the sample is:  ");
  Serial.println(baseline);
  Serial.print("The calibration value using the reflective side for the calibration LED and measuring LED are:  ");
  Serial.print(rebeltinvalue);
  Serial.print(" and ");
  Serial.println(irtinvalue);
  Serial.print("The calibration value using the black side for the calibration LED and measuring LED are:  ");
  Serial.print(rebeltapevalue);
  Serial.print(" and ");
  Serial.println(irtapevalue);

  delay(50);

}



















void dirkf() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

  calibrationsample();

  // Set saturating flash intensities via intensity1 and intensity2 (0 - 255)
  //  analogWrite(saturatinglight1_intensity1, 1); // set saturating light intensity
  //  analogWrite(saturatinglight1_intensity2, 255); // set saturating light intensity
  //  digitalWriteFast(saturatinglight1_intensity_switch, LOW); // tu rnintensity 1 on

  analogReadAveraging(dmeasurements); // set analog averaging (ie ADC takes one signal per ~3u)
  //  Serial.print("<RUNTIME>");
  //  Serial.println("</RUNTIME>");  

  /*
    Serial.print("<PROTOCOLTIME>");
   printTime();
   Serial.print("</PROTOCOLTIME>");
   Serial.println("");
   */
  //Serial.println("check this out");
  //Serial.println((drunlength*1000000/(2*dcyclelength))*sizeof(int));

  ddatasample1 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   
  ddatasample2 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   
  ddatasample3 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   
  ddatasample4 = (int*)malloc((drunlength*1000000/(dcyclelength*2))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   

  // TURN ACTINIC LIGHT ON
  digitalWriteFast(actiniclight1, HIGH);  

  for (x=0;x<drepeatrun;x++) {

    /*
    strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
     strcat(filenamelocal, dirkfending); // add the variable's extension defined in header
     sub1.open(filenamedir, O_READ);
     file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
     strcpy(filenamelocal,filename); // reset the localfilename variable;
     */

    // START TIMERS WITH PROPER SEPARATION BETWEEN THEM
    starttimer0 = micros()+100; // This is some arbitrary reasonable value to give the Arduino time before starting
    starttimer1 = starttimer0+dactinicoff;
    while (micros()<starttimer0) {
    }
    timer0.begin(dpulse1,dcyclelength); // First pulse before actinic turns off
    while (micros()<starttimer1) {
    }
    timer1.begin(dpulse2,dcyclelength); // Second pulse, just before actinic turns back on
    delayMicroseconds(dpulselengthon+30); // wait for dpulse2 to end before saving data - Important!  If too short, Teensy will freeze occassionally
    timer2.begin(ddatasave,dcyclelength); // Save data from each pulse

    // WAIT FOR TIMERS TO END (give it runlength plus a 10ms to be safe)
    delay(drunlength*1000+10);

    end1 = micros();

    // STOP AND RESET COUNTERS
    timer0.end();
    timer1.end();
    timer2.end();

    // MAKE SURE ANY REMAINING LIGHTS TURN OFF
    digitalWriteFast(measuringlight1, LOW);
    digitalWriteFast(calibratinglight1, LOW);
    digitalWriteFast(saturatinglight1, LOW);
    digitalWriteFast(actiniclight1, LOW);

    //    file.close(); // close out the file
    //    sub1.close(); // close out the directory (you MUST do this - if you don't, the file won't save the data!)

    delay(ddelayruns); // wait a little bit
  }

  dcalculations();

  //  Serial.print("<END>");

  x=0; // Reset counter
  dpulse1count = 0;
  dpulse2count = 0;
  dpulse1noactcount = 0;
  i=0;

  delay(100);
  free(ddatasample1); // release the memory allocated for the data
  delay(100);
  free(ddatasample2); // release the memory allocated for the data
  delay(100);
  free(ddatasample3); // release the memory allocated for the data
  delay(100);
  free(ddatasample4); // release the memory allocated for the data
  delay(100);
}

void dpulse1() {
  //    Serial.print(z);
  start1 = micros();
  digitalWriteFast(measuringlight1, HIGH);
  // GOT STUCK HERE WITH NO SATURATING LIGHT ON - AFTER MEASURING LIGHT WAS ON
  if (dsaturatingcycleon == dpulse1count) {
    digitalWriteFast(saturatinglight1, HIGH);  // Turn saturating light on
    manualadjust = 160;
  }  
  data0 = analogRead(detector1);
  start1=start1+dpulselengthon;
  while (micros()<start1) {
  }
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(actiniclight1, LOW); // Turn actinic off
  data1 = data0;
  dpulse1count++;
  //  Serial.println(dpulse1count);
}

void dpulse2() {
  //    Serial.print(z);
  start1 = micros();
  digitalWriteFast(measuringlight1, HIGH);
  // GOT STUCK HERE, AFTER MEASURING LIGHT WAS ON BUT WHILE THE ACTINIC WAS STILL OFF
  data1 = analogRead(detector1);
  start1=start1+dpulselengthon;
  while (micros()<start1) {
  }
  digitalWriteFast(measuringlight1, LOW);
  if (dsaturatingcycleoff == dpulse2count) {
    digitalWriteFast(saturatinglight1, LOW);  // Turn saturating light back on if it was off
    manualadjust = 80;
  }
  digitalWriteFast(actiniclight1, HIGH); // Turn actinic back on
  data2 = data0;
  dpulse2count++;
}

void ddatasave() {
  //  start3 = micros();
  /*
  file.print(data1-baseline);
   file.print(",");
   file.print(data2-baseline);
   file.print(",");
   file.print(data3-baseline);
   file.print(",");
   file.print(data4-baseline);
   file.print(",");
   file.println();
   */

  if (dpulse1count%2 == 1) {
    ddatasample1[(dpulse1count-1)/2] = data1-manualadjust;
    ddatasample2[(dpulse1count-1)/2] = data2-manualadjust;
  }
  if (dpulse1count%2 == 0) {
    ddatasample3[(dpulse1count-1)/2] = data1-manualadjust;
    ddatasample4[(dpulse1count-1)/2] = data2-manualadjust;
  }

  //  Serial.print(ddatasample1[dpulse2count]);
  //  Serial.print(",");

  /*
  Serial.print(data1-baseline);
   Serial.print(",");
   Serial.print(data2-baseline);
   Serial.print(",");
   Serial.print(data3-baseline);
   Serial.print(",");
   Serial.print(data4-baseline);
   Serial.print(",");
   Serial.println();
   */
  //  Serial.println(data1-baseline);
  //  Serial.println (data1);
  //  Serial.println(data2-baseline);  
  //  Serial.println (data2);
  //  end3=micros();
  //  Serial.println(end3-start3);
}

// USER PRINTOUT OF TEST RESULTS
void dcalculations() {


  //  Serial.println("");
  //  Serial.println("DATA");
  //  Serial.println("Raw Data: ");
  //  Serial.print("<VARIABLE1>");
  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
    Serial.print(ddatasample1[i]);
    Serial.print(",");
    //    Serial.print(ddatasample1[i]);
    //    Serial.print(",");
  }
  Serial.println(",");
  //  Serial.println("</VARIABLE1>");
  //  Serial.print("<VARIABLE1NAME>");
  //  Serial.print("F1 data");
  //  Serial.println("</VARIABLE1NAME>");

  //  Serial.print("<VARIABLE2>");
  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
    Serial.print(ddatasample2[i]);
    Serial.print(",");
    //    Serial.print(ddatasample2[i]);
    //    Serial.print(",");
  }
  Serial.println(",");
  //  Serial.println("</VARIABLE2>");
  //  Serial.print("<VARIABLE2NAME>");
  //  Serial.print("F2 data");
  //  Serial.println("</VARIABLE2NAME>");

  //  Serial.print("<VARIABLE3>");






  //  Serial.print("<VARIABLE5>");
  for (i=edge;i<(dsaturatingcycleon/2-edge);i++) { // Print the results!
    Fs += ddatasample3[i];
  }
  Fs = Fs / (dsaturatingcycleon/2-edge);
  //  Serial.println("</VARIABLE5>");
  //  Serial.print("<VARIABLE5NAME>");
  Serial.print("Relative chlorophyll content: ");
  Serial.println(Fs);
  //  Serial.println("</VARIABLE5NAME>");

  //  Serial.print("<VARIABLE6>");
  for (i=edge;i<(dsaturatingcycleon/2-edge);i++) { // Print the results!
    Fd += ddatasample2[i];
  }
  Fd = Fd / (dsaturatingcycleon/2-edge);
  //  Serial.println("</VARIABLE6>");
  //  Serial.print("<VARIABLE6NAME>");
  //  Serial.print("Fd");
  //  Serial.print(Fd);
  //  Serial.println("</VARIABLE6NAME>");

  //  Serial.print("<VARIABLE7>");
  for (i=dsaturatingcycleon/2+edge;i<(dsaturatingcycleoff/2-edge);i++) { // Print the results!
    Fm += ddatasample3[i];
  }
  Fm = Fm / (dsaturatingcycleoff/2-dsaturatingcycleon/2-2*edge);
  //  Serial.println("</VARIABLE7>");
  //  Serial.print("<VARIABLE7NAME>");
  //  Serial.print("Saturation response: ");
  //  Serial.print(Fm);
  //  Serial.println("</VARIABLE7NAME>");

  //  Serial.print("<VARIABLE8>");
  Phi2 = (Fm-Fs)/Fm;
  //  Serial.println("</VARIABLE8>");
  //  Serial.print("<VARIABLE8NAME>");
  Serial.print("Photosynthetic efficiency (Phi(II)): ");
  Serial.println(Phi2,3);
  //  Serial.println("</VARIABLE8NAME>");

  //  Serial.print("<VARIABLE9>");
  //  invFsinvFd = (1/Fs-1/Fd);
  //  Serial.println(invFsinvFd,6);
  //  Serial.println("</VARIABLE9>");
  //  Serial.print("<VARIABLE9NAME>");
  //  Serial.print("1/Fs-1/Fd");
  //  Serial.println("</VARIABLE9NAME>");

if (Fs<20) {
  Serial.println("No fluorescence, plant or otherwise!");
  Serial.println();
}
else {

  if (Phi2<.05) {
    Serial.println("This either isn't a plant, or it's dead");
  }
  else if (Phi2<.2 && Phi2>.05) {
    Serial.println("This plant is barely alive!");
  }
  else if (Phi2>.2 && Phi2<.4) {
    Serial.println("This plant is really unhappy");
  }
  else if (Phi2>.4 && Phi2<.6) {
    Serial.println("This plant is somewhat unhappy");
  }
  else if (Phi2>.6 && Phi2<.8) {
    Serial.println("This plant is doing ok");
  }
  else if (Phi2>.8) {
    Serial.println("This plant is feeling great!");
  }
  else {
    Serial.println("something's wrong - not an expected Phi2 value");
  }

for (int u=0;u<20;u++) {
  Serial.println();
}

  Serial.print("{subject: apple, ");
  Serial.print("raw:");

  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
     Serial.print(ddatasample3[i]);
     Serial.print(",");
     Serial.print(ddatasample3[i]);
     Serial.print(",");
  }
//  Serial.println(",");
  //  Serial.println("</VARIABLE3>");
  //  Serial.print("<VARIABLE3NAME>");
  //  Serial.print("F3 data");
  //  Serial.println("</VARIABLE3NAME>");

  //  Serial.print("<VARIABLE4>");
//  for (i=0;i<(dpulse2count/2);i++) { // Print the results!
//    Serial.print(ddatasample4[i]);
//    Serial.print(",");
    //    Serial.print(ddatasample4[i]);
    //    Serial.print(",");
//  }
//  Serial.println();
  //  Serial.println();
  //  Serial.println("</VARIABLE4>");
  //  Serial.print("<VARIABLE4NAME>");
  //  Serial.print("F4 data");
  //  Serial.println("</VARIABLE4NAME>");

  /*
  Serial.println("ALL DATA IN CURRENT DIRECTORY - BASELINE ADJUSTED VALUES");
   int16_t c;
   sub1.open(filenamedir, O_READ);
   strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
   strcat(filenamelocal, dirkfending);
   Serial.print("file directory: ");
   Serial.println(filenamedir);
   Serial.print("file name: ");
   Serial.println(filenamelocal);
   file.open(dir, filenamelocal, O_READ);
   Serial.print("<DATA>");  
   while ((c = file.read()) > 0) Serial1.write((char)c); // copy data to the Serial port
   Serial.print("</DATA>");
   strcpy(filenamelocal,filename); // reset the localfilename variable;
   Serial.println();
   
   file.close(); // close out the file
   sub1.close(); // close out the directory (you MUST do this - if you don't, the file won't save the data!)
   
   */

  /*
const char* variable1name = "Fs"; // Fs
   const char* variable2name = "Fm"; // Fm (peak value during saturation)
   const char* variable3name = "Fd"; // Fd (value when actinic light is off
   const char* variable4name = "Fv"; // == Fs-Fo
   const char* variable5name = "Phi2"; // == Fv/Fm
   const char* variable6name = "1/Fs-1/Fd"; // == 1/Fs-1/Fd
   */

  //  Serial.println("");
  ////  Serial.print("Automatic_correction_factor:  ");
  //  Serial.print("<BASELINE>");
  ////  Serial.print(baseline,8);  
  //  Serial.println("</BASELINE>");  
  //  Serial.println("");

  Serial.print(" Photosynthetic_efficiency_(Phi(II)): ");
  Serial.print(Phi2,3);
  Serial.print(",");
  Serial.print(" Relative_chlorophyll_content: ");
  Serial.print(Fs);
  Serial.print(",");
  Serial.print(" ProjectID: ");
  Serial.print("Fruit - Alive or Dead?, ");
  Serial.print(" UserID: ");
  Serial.print("gbathree, ");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(",");
  Serial.print("Relative_Humidity: ");
  Serial.print(rh);
  Serial.print(",");
  Serial.print("CO2_content: ");
  Serial.print(valCO2);
  Serial.println("}");
  

}

  /*
  Serial.print("The calibration value using the reflective side for the calibration LED and measuring LED are:  ");
   Serial.print(rebeltinvalue);
   Serial.print(" and ");
   Serial.println(irtinvalue);
   Serial.print("The calibration value using the black side for the calibration LED and measuring LED are:  ");
   Serial.print(rebeltapevalue);
   Serial.print(" and ");
   Serial.println(irtapevalue);
   
   totaltimecheck = end1 - start1orig;
   caltotaltimecheck = calend1 - calstart1orig;
   
   Serial.print("Actual run length (measuring pulses):  ");
   Serial.println(drepeatrun*(end1-starttimer0-100-30));
   
   Serial.print("Expected run length (measuring pulses):  ");
   Serial.println((drunlength*1000000)*drepeatrun);
   
   Serial.println();  
   
   Serial.print("Actual run length (calibration pulses):  ");
   Serial.println(caltotaltimecheck);
   
   delay(50);
   */
}




























void wasp() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

  calibrationsample();

  analogReadAveraging(wmeasurements); // set analog averaging (ie ADC takes one signal per ~3u)
  Serial.print("<RUNTIME>");
  Serial.println(wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3);
  Serial.print("</RUNTIME>");

  for (y=0;y<wrepeatrun;y++) {

    /*
    strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
     strcat(filenamelocal, waspending); // add the variable's extension defined in header
     sub1.open(filenamedir, O_READ);
     file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
     strcpy(filenamelocal,filename); // reset the localfilename variable;
     
     // SAVE CURRENT TIME TO SD CARD
     printTime();
     */

    wdatasample = (int*)malloc((wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3)*sizeof(int)); // NOTE! change the number value based on the number of analogReads() per measurement.  create the array of proper size to save one value for all each ON/OFF cycle   


    // ADJUST TIMERS FOR SET 1
    //    PITimer0.period(wcyclelength1);
    //    PITimer1.period(wcyclelength1);
    //    PITimer2.period(wcyclelength1*wnumberpulses1);

    // START TIMERS FOR SET 1 
    starttimer0 = micros()+100; // This is some arbitrary reasonable value to give the Arduino time before starting
    starttimer1 = starttimer0+wpulselengthon1;
    while (micros()<starttimer0) {
    }
    timer0.begin(wpulseon,wcyclelength1); // LED on.  Takes about 1us to call "start" function in PITimer plus 5us per analogRead()
    while (micros()<starttimer1) {
    }
    timer1.begin(wpulseoff,wcyclelength1); // LED off.
    timer2.begin(wstoptimers,wcyclelength1); // turn off timers after runlength time

    // WAIT FOR TIMERS TO END (give it runlength plus a 10ms to be safe)
    delay(wnumberpulses1*wcyclelength1*1000+10);
    z = 0; // reset counter z

    // ADJUST TIMERS FOR SET 2
    //    PITimer0.period(wcyclelength2);
    //    PITimer1.period(wcyclelength2);
    //    PITimer2.period(wcyclelength2*wnumberpulses2);

    // START TIMERS FOR SET 2
    starttimer0 = micros()+100; // This is some arbitrary reasonable value to give the Arduino time before starting
    starttimer1 = starttimer0+wpulselengthon2;
    while (micros()<starttimer0) {
    }
    timer0.begin(wpulseon,wcyclelength2); // LED on.  Takes about 1us to call "start" function in PITimer plus 5us per analogRead()
    while (micros()<starttimer1) {
    }
    timer1.begin(wpulseoff,wcyclelength2); // LED off.
    timer2.begin(wstoptimers,wcyclelength2*wnumberpulses2); // turn off timers after runlength time

    // WAIT FOR TIMERS TO END
    delay(wnumberpulses2*wcyclelength2*1000+10);
    z = 0; // reset counter z

    // ADJUST TIMERS FOR SET 3
    //    PITimer0.period(wcyclelength3);
    //    PITimer1.period(wcyclelength3);
    //    PITimer2.period(wcyclelength3*wnumberpulses3);

    // START TIMERS FOR SET 3 
    starttimer0 = micros()+100; // This is some arbitrary reasonable value to give the Arduino time before starting
    starttimer1 = starttimer0+wpulselengthon3;
    while (micros()<starttimer0) {
    }
    timer0.begin(wpulseon,wcyclelength3); // LED on.  Takes about 1us to call "start" function in PITimer plus 5us per analogRead()
    while (micros()<starttimer1) {
    }
    timer1.begin(wpulseoff,wcyclelength3); // LED off.
    timer2.begin(wstoptimers,wcyclelength3*wnumberpulses3); // turn off timers after runlength time

    // WAIT FOR TIMERS TO END (give it runlength plus a 10ms to be safe)
    delay(wnumberpulses3*wcyclelength3*1000+10);
    z = 0; // reset counter z

    free(wdatasample); // release the memory allocated for the data
    file.println("");
    //    file.close(); // close out the file
    //    sub1.close(); // close out the director (you MUST do this - if you don't, the file won't save the data!)

    // delay(300000); // wait 5 minutes between measurements

    wcalculations();

  }
}

void wpulseon() {
  //  Serial.print(z);
  digitalWriteFast(measuringlight1, HIGH);
  data1 = analogRead(detector1);
  data2 = analogRead(detector1);
  data3 = analogRead(detector1);
  data4 = analogRead(detector1);
  i = 0;
}

void wpulseoff() {

  // NOTE! for very short OFF cycles, just store the data in the datasample[], and write to 
  // the SD card at the end.  If OFF cycle is long enough (50us or more), then you can write
  // directly to the SD card.  The advantage is you are limited to ~1500 data points for
  // datasample[] before it becomes too big for the memory to hold.

  digitalWriteFast(measuringlight1, LOW);
  wdatasampleaverage1 += data1;
  wdatasampleaverage2 += data2;
  wdatasampleaverage3 += data3;
  wdatasampleaverage4 += data4;
  wdatasample[z] = ((data1+data2+data3+data4)/4);
  Serial.println(wdatasample[z]);
  Serial.println(',');
  //  file.print(wdatasample[z]);
  //  file.print(",");  
  data1 = 0; // reset data1 for the next round
  data2 = 0; // reset data1 for the next round
  data3 = 0; // reset data1 for the next round
  data4 = 0; // reset data1 for the next round
  z=z+1;
}


void wstoptimers() { // Stop timers, close file and directory on SD card, free memory from datasample[], turn off lights, reset counter variables, move to next row in .csv file, 
  timer0.end();
  timer1.end();
  timer2.end();
  end1 = micros();
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(calibratinglight1, LOW);
  digitalWriteFast(saturatinglight1, LOW);
  z=0; // reset counters
  i=0;
  Serial.print("Total run time is ~: ");
  Serial.println(end1-starttimer0);
  Serial.println("");
  Serial.println("Average values for data measurements 1,2,3 and 4:  ");
  Serial.println(wdatasampleaverage1*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial.println(wdatasampleaverage2*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial.println(wdatasampleaverage3*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial.println(wdatasampleaverage4*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial.println("");
  wdatasampleaverage1 = 0;
  wdatasampleaverage2 = 0;
  wdatasampleaverage3 = 0;
  wdatasampleaverage4 = 0;
}

// USER PRINTOUT OF TEST RESULTS
void wcalculations() {

  Serial.println("DATA - RAW ANALOG VALUES IN uV");
  for (i=0;i<(wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3);i++) {
    wdatasample[i] = 1000000*((reference*wdatasample[i])/(analogresolutionvalue*wmeasurements)); 
  }
  for (i=0;i<(wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3);i++) { // Print the results!
    Serial.print(wdatasample[i], 8);
    Serial.print(",");
  }
  Serial.println("");

  /*
  Serial.println("ALL DATA IN CURRENT DIRECTORY - BASELINE ADJUSTED VALUES");
   
   int16_t c;
   sub1.open(filenamedir, O_READ);
   strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
   strcat(filenamelocal, waspending);
   Serial.print("file directory: ");
   Serial.println(filenamedir);
   Serial.print("file name: ");
   Serial.println(filenamelocal);
   file.open(dir, filenamelocal, O_READ);
   while ((c = file.read()) > 0) Serial1.write((char)c); // copy data to the Serial port
   strcpy(filenamelocal,filename); // reset the localfilename variable;
   
   file.close(); // close out the file
   sub1.close(); // close out the director (you MUST do this - if you don't, the file won't save the data!)
   */

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial.println("");
  Serial.print("Size of the baseline:  ");
  Serial.print("<BASELINE>");
  Serial.println(baseline, 8);
  Serial.print("</BASELINE>");

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial.println("");
  Serial.println("GENERAL INFORMATION");
  Serial.println("");

  Serial.print("total run length (measuring pulses):  ");
  Serial.println(totaltimecheck);

  Serial.print("expected run length(measuring pulses):  ");
  Serial.println(wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3);

  Serial.print("total run length (calibration pulses):  ");
  Serial.println(caltotaltimecheck);

  Serial.println("");
  Serial.println("CALIBRATION DATA");
  Serial.println("");

  Serial.print("The baseline from the sample is:  ");
  Serial.println(baseline);
  Serial.print("The calibration value using the reflective side for the calibration LED and measuring LED are:  ");
  Serial.print(rebeltinvalue);
  Serial.print(" and ");
  Serial.println(irtinvalue);
  Serial.print("The calibration value using the black side for the calibration LED and measuring LED are:  ");
  Serial.print(rebeltapevalue);
  Serial.print(" and ");
  Serial.println(irtapevalue);

  delay(50);

}


/*  code to process time sync messages from the Serial port   */
/*
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by unix time_t as ten ascii digits
 #define TIME_HEADER  'T'   // Header tag for Serial time sync message
 
 time_t processSyncMessage() {
 // return the time if a valid sync message is received on the Serial port.
 while(Serial1.available() >=  TIME_MSG_LEN ){  // time message consists of a header and ten ascii digits
 char c = Serial1.read() ; 
 Serial.print(c);  
 if(c == TIME_HEADER ) {       
 time_t pctime = 0;
 for(int i=0; i < TIME_MSG_LEN -1; i++){   
 c = Serial1.read();          
 if( c >= '0' && c <= '9'){   
 pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
 }
 }   
 return pctime;
 Serial.println("this is pc time");
 Serial.println(pctime);
 }  
 }
 return 0;
 i=0; // reset i
 }
 
 // PRINT CURRENT TIME TO DISPLAY IN Serial PORT
 void SerialPrintClock(){
 Serial.print(month());
 Serial.print("/");
 Serial.print(day());
 Serial.print("/");
 Serial.print(year()); 
 Serial.print(" ");
 Serial.print(hour());
 SerialprintDigits(minute());
 SerialprintDigits(second());
 Serial.println(); 
 }
 
 void SerialprintDigits(int digits){
 // utility function for digital clock display: prints preceding colon and leading 0
 Serial.print(":");
 if(digits < 10)
 Serial.print('0');
 Serial.print(digits);
 }
 
/*
 void fileprintDigits(int digits){
 // utility function for digital clock display: prints preceding colon and leading 0
 file.print(":");
 if(digits < 10)
 file.print('0');
 file.print(minute());
 }
 */
/*
void printTime () {
 
 file.print(month());
 file.print("/");
 file.print(day());
 file.print("/");
 file.print(year()); 
 file.print(" ");
 file.print(hour());
 file.print(":");
 if(minute() < 10)
 file.print('0');
 file.print(minute());
 file.print(":");
 if(second() < 10)
 file.print('0');
 file.print(second());
 file.print(","); 
 
 
 Serial.print(month());
 Serial.print("/");
 Serial.print(day());
 Serial.print("/");
 Serial.print(year()); 
 Serial.print(" ");
 Serial.print(hour());
 Serial.print(":");
 if(minute() < 10)
 Serial.print('0');
 Serial.print(minute());
 Serial.print(":");
 if(second() < 10)
 Serial.print('0');
 Serial.print(second());
 }
 */

int savecalibration(float calval, int loc) {
  char str [10];
  calval = calval*1000000;
  calval = (int) calval;
  itoa(calval, str, 10);
  for (i=0;i<10;i++) {
    EEPROM.write(loc+i,str[i]);
    //    Serial.print(str[i]);
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
  //  Serial.print(calval,4);
  return calval;
}




