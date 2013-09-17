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

RECENT UPDATES:
 * Replaced PITimer library with IntervalTimer, which is part of the Teensy 3.0 core libraries
 * Upon update to Arduino 1.0.5, changed atime_t to time_t to ensure that time variables worked
 * Added appropriate tags so that the text output can be hashed by the phone and web interface (using <>)
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

// NOTES:
// Chris needs to add an experiment start time stamp for setTime() at beginning of program
// time format: %m/%d/%Y %H:%M:%S

// Now correctly baseline adjusted automatically

// SPECS USING THIS METHOD: 
// Timing of Measuring pulse and Saturation pulse is within 500ns.  Peak to Peak variability, ON and OFF length variability all is <1us.  Total measurement variability = 0 to +2us (regardless of how many cycles)

// SD CARD ENABLE AND SET
#include <SdFat.h>
#include <SdFatUtil.h> // use functions to print strings from flash memory
#include <Time.h>   // enable real time clock library
#include <Wire.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
//#include <PITimer.h> // intervaltimer is now part of the primary teensy 3.0 library!

const uint8_t SD_CHIP_SELECT = SS;

SdFat sd;
#define error(s) sd.errorHalt_P(PSTR(s))  // store error strings in flash to save RAM

// KEY VARIABLES USED IN THE PROTOCOL

// SHARED VARIABLES

char filename[13] = "ALGAE"; // Base filename used for data files and directories on the SD Card
char protocolname = "Photosynq Main" // name of this arduino file (aka protocol)
char variable1name = "Fs" // Fs
char variable2name = "Fm" // Fm (peak value during saturation)
char variable3name = "Fd" // Fd (value when actinic light is off
char variable4name = "Fv" // == Fs-Fo
char variable5name = "Phi2" // == Fv/Fm
char variable6name = "1/Fs-1/Fd" // == 1/Fs-1/Fd
unsigned long starttimer0, starttimer1, starttimer2;

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
int ddelayruns = 500; // millisecond delay between each repeated run
int dnoactruns = 3; // number of runs (occuring at beginning) which do not turn actinic off (maximum = drepeatrun)
int dmeasurements = 3; // # of measurements per pulse (min 1 measurement per 6us pulselengthon)
int drunlength = 2; // in seconds... minimum = cyclelength
volatile unsigned long dpulselengthon = 30; // pulse length in us... minimum = 6us
volatile float dcyclelength = 10000; // time between cycles of pulse/actinicoff/pulse/actinic on in us... minimum = pulselengthon + 7.5us
volatile int dactinicoff = 500; // in us... length of time actinic is turned off
volatile int dsaturatingcycleon = 50; // The cycle number in which the saturating light turns on (set = 0 for no saturating light)
volatile int dsaturatingcycleoff = 100; //The cycle number in which the saturating light turns off
const char* dirkfending = "-D.CSV"; // filename ending for the basicfluor subroutine - just make sure it's no more than 6 digits total including the .CSV
// int* ddatasample1;
// int* ddatasample2;
// int* ddatasample3;
// int* ddatasample4;

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
float reference = 1.2; // The reference (AREF) supplied to the ADC - currently set to INTERNAL = 1.2V
int analogresolution = 16; // Set the resolution of the analog to digital converter (max 16 bit, 13 bit usable)  
int measuringlight1 = 3; // Teensy pin for measuring light
int saturatinglight1 = 4; // Teensy pin for saturating light
int calibratinglight1 = 2; // Teensy pin for calibrating light
int actiniclight1 = 5; // Teensy pin for actinic light
int detector1 = A10; // Teensy analog pin for detector

// INTERNAL VARIABLES, COUNTERS, ETC.
volatile unsigned long start1,start1orig,end1, start3, end3, calstart1orig, calend1, start5, start6, start7, end5;
unsigned long pulselengthoncheck, pulselengthoffcheck, pulsecyclescheck, totaltimecheck, caltotaltimecheck;
volatile float data1f, data2f, data3f, data4f, irtinvalue, irtapevalue, rebeltapevalue, rebeltinvalue, irsamplevalue, rebelsamplevalue, baselineir, dataaverage, caldataaverage1, caldataaverage2, rebelslope, irslope, baseline = 0;
char filenamedir[13];
char filenamelocal[13];
volatile int data1=0, data2=0, data3=0, data4=0, data5=0, data6=0, data7=0, data8=0, data9=0, data10=0, caldata1, caldata2, caldata3, caldata4, analogresolutionvalue, dpulse1count, dpulse1noactcount, dpulse2count;
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
IntervalTimer timer2;SdFile file;
SdFile sub1;
SdBaseFile* dir = &sub1;

void setup() {
  Serial.begin(115200); // set baud rate for Serial1 communication
  Serial1.begin(115200); // set baud rate for bluetooth communication on pins 0,1

  delay(5000);

  print("<PROTOCOLNAME>");
  print(protocolname);
  println("</PROTOCOLNAME>");

  // SET SYSTEM TIME IN ASCII "T" PLUS 10 DIGITS
  setSyncProvider((getExternalTime) Teensy3Clock.get);
  //  while (!Serial1);  // Wait for Arduino Serial1 Monitor to open
  if(timeStatus()!= timeSet) {
    Serial1.println("Unable to sync with the RTC");
    Serial.println("Unable to sync with the RTC");
  }
  else {
    Serial1.println("RTC has set the system time"); // NOTE! Chris enter experiment start time stamp here
    Serial.println("RTC has set the system time"); // NOTE! Chris enter experiment start time stamp here
  }

//for (i=0;i<50;i++) { 
  Serial1.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
  Serial1.println("(if you make a mistake, restart teensy and reenter)");
  Serial.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
  Serial.println("(if you make a mistake, restart teensy and reenter)");

/*delay(1000);
  Serial11.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
  Serial11.println("(if you make a mistake, restart teensy and reenter)");
  Serial11.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
  Serial11.println("(if you make a mistake, restart teensy and reenter)");
delay(1000);
}*/

  //  Serial1.println(Serial1.available());


  while (Serial1.available()<11) {
    // c = Serial1.read();
    //  Serial1.print(Serial1.available());
    //  Serial1.print(",");
    //  Serial1.println(Serial1.peek());
    //  delay(500);

  }
  time_t t = processSyncMessage();
  Serial1.print("'T' plus ASCII 10 digit time: ");
  Serial1.println(t);
  Serial1.print("Serial1 buffer size: ");
  Serial1.println(Serial1.available());
  setTime(t);          
  Serial1.print("UTC time: ");
  Serial1PrintClock();  
  //  delay(3000);
  //  digitalClockDisplay();
  //  }
  Serial1.println("");


  pinMode(measuringlight1, OUTPUT); // set pin to output
  pinMode(saturatinglight1, OUTPUT); // set pin to output
  pinMode(calibratinglight1, OUTPUT); // set pin to output  
  pinMode(actiniclight1, OUTPUT); // set pin to output
  analogReadAveraging(1); // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u
  pinMode(detector1, EXTERNAL);
  analogReadRes(analogresolution);
  analogresolutionvalue = pow(2,analogresolution); // calculate the max analogread value of the resolution setting

  if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) sd.initErrorHalt(); // Set SD Card to full speed ahead!

  //PRINT SD CARD MEMORY INFORMATION
  PgmPrint("Free RAM: ");
  Serial1.println(FreeRam());  

  PgmPrint("Volume is FAT");
  Serial1.println(sd.vol()->fatType(), DEC);
  Serial1.println();

  // list file in root with date and size
  PgmPrintln("Files found in root:");
  sd.ls(LS_DATE | LS_SIZE);
  Serial1.println();

  // Recursive list of all directories
  PgmPrintln("Files found in all dirs:");
  sd.ls(LS_R);

  // PRINT OTHER INFORMATION
  wprintout();

  //CREATE NEW DIRECTORY OR DATA FILE, OR APPEND EXISTING  
  //If the primary file name does not yet exist as a file or folder, then create the folder and store all subsequent files (ie file-m.csv, file.c.csv...) in that folder
  //... if the primary file name does already exist as a file or folder, then create a new folder with that name and store subsequent files in it.

  strcpy(filenamedir, filename);
  Serial1.println(sd.exists(filenamedir));

  if (sd.exists(filenamedir) == 1) {
    Serial1.print("Press 1 to append the existing data files in directory ");
    Serial1.print(filenamedir);
    Serial1.println(" or press 2 to create a new directory.");    

    while (cal3 != 1 && cal3 !=2) {
      cal3 = Serial1.read()-48; // for some crazy reason, inputting 0 in the Serial1 Monitor yields 48 on the Teensy itself, so this just shifts everything by 48 so that when you input X, it's saved in Teensy as X.
      if (cal3 == 2) {
        for (j = 0; j<100; j++) {
          filenamedir[0] = j/10 + '0';
          filenamedir[1] = j%10 + '0';
          if (sd.exists(filenamedir) == 0) {
            break;
          }
        }
        Serial1.print("OK - creating a new directory called:  ");
        Serial1.println(filenamedir);
        SdFile sub1;
        sub1.makeDir(sd.vwd(), filenamedir); // Creating new folder
        Serial1.print(".  Data files are saved in that directory as ");
        Serial1.print(filename);
        Serial1.println(" plus -(subroutine extension letter).CSV for each subroutine.");
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
        Serial1.print("Ok - appending most recently used files located in directory ");
        Serial1.println(filenamedir);
        break;
      }
    }
  }
  else if (sd.exists(filenamedir) == 0) {

    Serial1.print("OK - creating a new directory called ");
    Serial1.print(filenamedir);
    Serial1.print(".  Data files are saved in that directory as ");
    Serial1.print(filenamedir);
    Serial1.println(" plus -(subroutine extension letter).CSV for each subroutine.");
    sub1.makeDir(sd.vwd(), filenamedir); // Creating new folder

  }



  // MAIN PROGRAM
  Serial1.println("Would you like to run a calibration? -- type 1 for yes or 2 for no");
  while (cal == 0 | flag == 0) {
    cal = Serial1.read()-48; //
    if (cal == 1) {
      delay(50);
      Serial1.println("Ok - calibrating and then running normal protocal");
      Serial1.println("Place the shiny side of the calibration piece face up in the photosynq, and close the lid.");
      Serial1.println("When you're done, press any key to continue");
      Serial1.flush();
      while (Serial1.read() <= 0) {
      }

      Serial1.println("Thanks - calibrating...");
      calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtin()!
      calibrationtin();

      Serial1.println("Now place the black side of the calibration piece face up in the photosynq, and close the lid.");
      Serial1.println("When you're done, press any key to continue");
      Serial1.flush();
      while (Serial1.read() <= 0) {
      }

      Serial1.println("Thanks - calibrating...");
      calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtape()!
      calibrationtape();
      Serial1.println("Calibration finished!");
      cal = 2;
    }
    if (cal == 2) {
      delay(50);
      Serial1.println("Proceeding with normal measurement protocol");
      Serial1.println("Place the sample in front of the light guide and press any key to continue");
      Serial1.flush();
      while (Serial1.read() <= 0) {
      }
      Serial1.println("Thanks - running protocol...");
      while (1) {
        calibrationsample(); // order matters here - calibrationsample() must be BEFORE basicflour()
        Serial1.println("Now running DIRKF");
        dirkf();
        dcalculations();
        //        Uncomment below for Basic Fluor or WASP methods
        //        Serial1.println("Now running Basic Fluor");
        //        basicfluor();
        //        bcalculations();
        //        Serial1.println("Now running WASP");
        //        wasp();
        //        wcalculations();
        Serial1.println("");        
        Serial1.println("Press any key to take another measurement, or press reset button to calibrate or start a new file");
        while (Serial1.read() <= 0) {
        }
      }      
    }

    else if (cal > 2) {
      delay(50);
      Serial1.println("That's not a 1 or a 2!  Please enter either 1 for yes, or 2 for no");
      cal = 0;
    }
  }
}


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
  Serial1.print("Rebel sample value:  ");
  Serial1.println(rebelsamplevalue);
  Serial1.println("");  
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial1.print(rebeldatasample[i]);
    Serial1.print(", ");
    Serial1.print(" ");  
  }
  Serial1.println("");
  Serial1.print("see the data here:  ");  
  Serial1.println(data1);
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
  Serial1.println(irtinvalue);  
  irtinvalue = (float) irtinvalue;
  irtinvalue = (irtinvalue / calpulsecycles); //  Divide by the total number of samples to get the average reading during the calibration - NOTE! The divisor here needs to be equal to the number of analogReads performed above!!!! 
  rebeltinvalue = rebelsamplevalue;
  rebelsamplevalue = (int) rebelsamplevalue; // reset rebelsamplevalue to integer for future operations
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial1.print(caldatatin[i]);
    Serial1.print(", ");
    Serial1.print(" ");  
  }
  Serial1.println(" ");    
  Serial1.print("the baseline high reflectace value from calibration: ");
  Serial1.println(irtinvalue, 7);
  Serial1.print("The last 4 data points from the calibration: ");  
  Serial1.println(caldata1);
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
  Serial1.println(irtapevalue);
  irtapevalue = (float) irtapevalue;
  irtapevalue = (irtapevalue / calpulsecycles); 
  rebeltapevalue = rebelsamplevalue;
  rebelsamplevalue = (int) rebelsamplevalue; // reset rebelsamplevalue to integer for future operations 
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial1.print(caldatatape[i]);
    Serial1.print(", ");
    Serial1.print(" ");  
  }
  Serial1.println(" ");    
  Serial1.print("the baseline low reflectace value from calibration:  ");
  Serial1.println(irtapevalue, 7);
  Serial1.print("The last 4 data points from the calibration: ");  
  Serial1.println(caldata1);



  //CALCULATE AND SAVE CALIBRATION DATA TO SD CARD
  rebelslope = rebeltinvalue - rebeltapevalue;
  irslope = irtinvalue - irtapevalue;

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

  Serial1.print("IR slope: ");  
  Serial1.println(irslope, 8);
  Serial1.print("rebel slope: ");  
  Serial1.println(rebelslope, 8);
  Serial1.print("ir tin value: ");  
  Serial1.println(irtinvalue, 8);
  Serial1.print("ir tape value: ");  
  Serial1.println(irtapevalue, 8);
  Serial1.print("rebel tin value: ");  
  Serial1.println(rebeltinvalue, 8);
  Serial1.print("rebel tape value: ");  
  Serial1.println(rebeltapevalue, 8);

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
  //  Serial1.println(irsamplevalue);
  irsamplevalue = (float) irsamplevalue;
  irsamplevalue = (irsamplevalue / calpulsecycles); 
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial1.print(caldatasample[i]);
    Serial1.print(", ");
    Serial1.print(" ");  
  }
  Serial1.println(" ");    
  Serial1.print("the baseline sample reflectace value from calibration:  ");
  Serial1.println(irsamplevalue, 7);

  // RETRIEVE STORED TIN AND TAPE CALIBRATION VALUES AND CALCULATE BASELINE VALUE

  int c;
  char local[12];

  Serial1.println("Calibration values pulled from SD Card");

  file.open("ISLOP.CSV", O_READ);
  i = 0; // reset counter just in case
  while ((c = file.read()) > 0) {
    local[i] = (char) c;
    i++;
  }
  i = 0; // reset counter
  irslope = strtod(local, NULL);
  Serial1.println(irslope, 8);
  file.close();

  file.open("RSLOP.CSV", O_READ);
  i = 0; // reset counter just in case
  while ((c = file.read()) > 0) {
    local[i] = (char) c;
    i++;
  }
  i = 0; // reset counter
  rebelslope = strtod(local, NULL);
  Serial1.println(rebelslope, 8);
  file.close();

  file.open("ITIN.CSV", O_READ);
  i = 0; // reset counter just in case
  while ((c = file.read()) > 0) {
    local[i] = (char) c;
    i++;
  }
  i = 0; // reset counter
  irtinvalue = strtod(local, NULL);
  Serial1.println(irtinvalue, 8);
  file.close();

  file.open("ITAPE.CSV", O_READ);
  i = 0; // reset counter just in case
  while ((c = file.read()) > 0) {
    local[i] = (char) c;
    i++;
  }
  i = 0; // reset counter
  irtapevalue = strtod(local, NULL);
  Serial1.println(irtapevalue, 8);
  file.close();

  file.open("RTIN.CSV", O_READ);
  i = 0; // reset counter just in case
  while ((c = file.read()) > 0) {
    local[i] = (char) c;
    i++;
  }
  i = 0; // reset counter
  rebeltinvalue = strtod(local, NULL);
  Serial1.println(rebeltinvalue, 8);
  file.close();

  file.open("RTAPE.CSV", O_READ);
  i = 0; // reset counter just in case
  while ((c = file.read()) > 0) {
    local[i] = (char) c;
    i++;
  }
  i = 0; // reset counter
  rebeltapevalue = strtod(local, NULL);
  Serial1.println(rebeltapevalue, 8);

  file.close();
  sub1.close();

  // CALCULATE BASELINE VALUE
  baseline = (rebeltapevalue+((irsamplevalue-irtapevalue)/irslope)*rebelslope);
}













void basicfluor() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

  for (q=0;q<brepeatrun;q++) {

    strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
    strcat(filenamelocal, basicfluorending); // add the variable's extension defined in header
    sub1.open(filenamedir, O_READ);
    file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
    strcpy(filenamelocal,filename); // reset the localfilename variable;

    // SAVE CURRENT TIME TO SD CARD
    printTime();

    Serial1.print("THIS IS THE BASELINE: ");
    Serial1.println(baseline);  

    bdatasample = (int*)malloc((brunlength/(bcyclelength))*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle   

    // ADJUST TIMERS FOR SET 1
//    PITimer0.period(bcyclelength); // Distance between pulses.  Set in Seconds.
//    PITimer1.period(bcyclelength); // set == to PITimer0
//    PITimer2.period(brunlength);

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
  }
}



void bpulseon() {
  //    Serial1.print(z);
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
  Serial1.println(bdatasample[z]);
  file.print(bdatasample[z]);
  file.print(",");  
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
  Serial1.print("Total run time is ~: ");
  Serial1.println(end1-starttimer0);
}

// USER PRINTOUT OF MAIN PARAMETERS
void bprintout() {

  Serial1.println("");
  Serial1.print("Pulse cycles during calibration                    ");
  Serial1.println(calpulsecycles); // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
  Serial1.print("Length of a single measuring pulse in us           ");
  Serial1.println(bpulselengthon); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
  Serial1.print("Base data file name                                ");
  Serial1.println(filename);
  Serial1.println("");
  Serial1.print("Teensy pin for measuring light      ");
  Serial1.println(measuringlight1);
  Serial1.print("Teensy pin for saturating light     ");
  Serial1.println(saturatinglight1);
  Serial1.print("Teensy pin for calibration light    ");
  Serial1.println(calibratinglight1);
  Serial1.print("Teensy pin for detector             ");
  Serial1.println(detector1);
  Serial1.println("");

}

// USER PRINTOUT OF TEST RESULTS
void bcalculations() {

  Serial1.println("DATA - RAW ANALOG VALUES IN uV");
  for (i=0;i<(brunlength/bcyclelength);i++) {
    bdatasample[i] = 1000000*((reference*bdatasample[i])/(analogresolutionvalue*bmeasurements)); 
  }
  for (i=0;i<(brunlength/bcyclelength);i++) { // Print the results!
    Serial1.print(bdatasample[i], 8);
    Serial1.print(",");
  }
  Serial1.println("");

  Serial1.println("ALL DATA IN CURRENT DIRECTORY - BASELINE ADJUSTED VALUES");

  int16_t c;
  sub1.open(filenamedir, O_READ);
  strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
  strcat(filenamelocal, waspending);
  Serial1.print("file directory: ");
  Serial1.println(filenamedir);
  Serial1.print("file name: ");
  Serial1.println(filenamelocal);
  file.open(dir, filenamelocal, O_READ);
  while ((c = file.read()) > 0) Serial1.write((char)c); // copy data to the Serial1 port
  strcpy(filenamelocal,filename); // reset the localfilename variable;

  file.close(); // close out the file
  sub1.close(); // close out the director (you MUST do this - if you don't, the file won't save the data!)

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial1.println("");
  Serial1.print("Size of the baseline:  ");
  Serial1.println(baseline, 8);

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial1.println("");
  Serial1.println("GENERAL INFORMATION");
  Serial1.println("");

  Serial1.print("total run length (measuring pulses):  ");
  Serial1.println(totaltimecheck);

  Serial1.print("expected run length(measuring pulses):  ");
  Serial1.println(brunlength/bcyclelength);

  Serial1.print("total run length (calibration pulses):  ");
  Serial1.println(caltotaltimecheck);

  Serial1.println("");
  Serial1.println("CALIBRATION DATA");
  Serial1.println("");


  Serial1.print("The baseline from the sample is:  ");
  Serial1.println(baseline);
  Serial1.print("The calibration value using the reflective side for the calibration LED and measuring LED are:  ");
  Serial1.print(rebeltinvalue);
  Serial1.print("and ");
  Serial1.println(irtinvalue);
  Serial1.print("The calibration value using the black side for the calibration LED and measuring LED are:  ");
  Serial1.print(rebeltapevalue);
  Serial1.print("and ");
  Serial1.println(irtapevalue);

  delay(50);

}



















void dirkf() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

  analogReadAveraging(dmeasurements); // set analog averaging (ie ADC takes one signal per ~3u)

  for (x=0;x<drepeatrun;x++) {

    strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
    strcat(filenamelocal, dirkfending); // add the variable's extension defined in header
    sub1.open(filenamedir, O_READ);
    file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
    strcpy(filenamelocal,filename); // reset the localfilename variable;

    Serial1.print("THIS IS THE BASELINE: ");
    Serial1.print("<BASELINE>");
    Serial1.print(baseline);  
    Serial1.print("</BASELINE>");

    // ADJUST TIMERS FOR SET 1
//    PITimer0.period(dcyclelength); // Distance between pulses.  Set in Seconds.
//    PITimer1.period(dcyclelength);
//    PITimer2.period(dcyclelength);

    if (x<dnoactruns) {

      file.print("Measuring Pulse 1n: no actinic pulse");
      file.print(",");
      file.print("Measuring Pulse 2n: no actinic pulse");
      file.println("");
      printTime();
      printTime();
      file.println("");
      delay(50);

      // START TIMERS WITH PROPER SEPARATION BETWEEN THEM
      starttimer0 = micros()+100; // This is some arbitrary reasonable value to give the Arduino time before starting
      starttimer1 = starttimer0+dactinicoff;
      while (micros()<starttimer0) {
      }
      timer0.begin(dpulse1noact,dcyclelength); // Leave actinic on for first run
      while (micros()<starttimer1) {
      }
      timer1.begin(dpulse2,dcyclelength); // Second pulse, just before actinic turns back on
      delayMicroseconds(dpulselengthon+30); // wait for dpulse2 to end before saving data - Important!  If too short, Teensy will freeze occassionally
      timer2.begin(ddatasavenoact,dcyclelength); // Save data from each pulse

      // WAIT FOR TIMERS TO END (give it runlength plus a 10ms to be safe)
      delay(drunlength*1000+10);

      end1 = micros();

      // STOP AND RESET TIMERS
      timer0.end();
      timer1.end();
      timer2.end();

      // MAKE SURE ANY REMAINING LIGHTS TURN OFF
      digitalWriteFast(measuringlight1, LOW);
      digitalWriteFast(calibratinglight1, LOW);
      digitalWriteFast(saturatinglight1, LOW);

      i=0; // Reset counter

      Serial1.print("Total run time is ~: ");
      Serial1.println(end1-starttimer0-100-10);  
    }

    else {

      file.print("Measuring Pulse 1a: with actinic pulse");
      file.print(",");
      file.print("Measuring Pulse 2a: with actinic pulse");
      file.println("");
      printTime();
      printTime();
      file.println("");

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
      dpulse1count = 0;
      dpulse2count = 0;
      dpulse1noactcount = 0;
      i=0;

      // MAKE SURE ANY REMAINING LIGHTS TURN OFF
      digitalWriteFast(measuringlight1, LOW);
      digitalWriteFast(calibratinglight1, LOW);
      digitalWriteFast(saturatinglight1, LOW);

      Serial1.print("Total run time is ~: ");
      Serial1.println(end1-starttimer0-100-30); // 30 is estimate of the additional time requires for something (I can't remember!)
    }

    file.println("");
    file.close(); // close out the file
    sub1.close(); // close out the directory (you MUST do this - if you don't, the file won't save the data!)

    delay(ddelayruns); // wait a little bit
  }
  x=0; // Reset counter
}

void dpulse1() {
  //    Serial1.print(z);
  start1 = micros();
  digitalWriteFast(measuringlight1, HIGH);
  // GOT STUCK HERE WITH NO SATURATING LIGHT ON - AFTER MEASURING LIGHT WAS ON
  if (dsaturatingcycleon == dpulse1count) {
    digitalWriteFast(saturatinglight1, HIGH);  // Turn saturating light on   
  }  
  data1 = analogRead(detector1);
  start1=start1+dpulselengthon;
  while (micros()<start1) {
  }
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(actiniclight1, HIGH); // turn actinic off
  //  Serial1.println(PITimer0.count());
  dpulse1count++;
}

void dpulse1noact() {
  //    Serial1.print(z);
  start1 = micros();
  digitalWriteFast(measuringlight1, HIGH);
  if (dsaturatingcycleon == dpulse1noactcount) {
    digitalWriteFast(saturatinglight1, HIGH);  // Turn saturating light on   
  }  
  data1 = analogRead(detector1);
  start1=start1+dpulselengthon;
  while (micros()<start1) {
  }
  digitalWriteFast(measuringlight1, LOW);
  //  Serial1.println(PITimer0.count());
dpulse1noactcount++;
}

void dpulse2() {
  //    Serial1.print(z);
  start1 = micros();
  digitalWriteFast(measuringlight1, HIGH);
  // GOT STUCK HERE, AFTER MEASURING LIGHT WAS ON BUT WHILE THE ACTINIC WAS STILL OFF
  data2 = analogRead(detector1);
  start1=start1+dpulselengthon;
  while (micros()<start1) {
  }
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(actiniclight1, LOW);
  if (dsaturatingcycleoff == dpulse2count) {
    digitalWriteFast(saturatinglight1, LOW);  // Turn saturating light back on if it was off
  }  
  //  Serial1.println(PITimer1.count());
dpulse2count++;
}

void ddatasave() {
  //  start3 = micros();
  file.print(data1-baseline);
  file.print(",");
  file.print(data2-baseline);
  file.println("");
  Serial1.println(data1-baseline);
  //  Serial1.println (data1);
  Serial1.println(data2-baseline);  
  //  Serial1.println (data2);
  //  Serial1.println(PITimer2.count());
  //  end3=micros();
  //  Serial1.println(end3-start3);
}

void ddatasavenoact() {
  start3 = micros();  
  file.print(data1-baseline);
  file.print(",");
  file.print(data2-baseline);
  file.println("");
  Serial1.println(data1-baseline);
  //  Serial1.println (data1);
  Serial1.println(data2-baseline);  
  //  Serial1.println (data2);
  //  Serial1.println(PITimer2.count());
  end3=micros();
  Serial1.println(end3-start3);
}

// USER PRINTOUT OF MAIN PARAMETERS
void dprintout() {

  Serial1.println("");
  Serial1.print("Pulse cycles during calibration                    ");
  Serial1.println(calpulsecycles); // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
  Serial1.print("Length of a single measuring pulse in us           ");
  Serial1.println(dpulselengthon); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
  Serial1.print("Base data file name                                ");
  Serial1.println(filename);
  Serial1.println("");
  Serial1.print("Teensy pin for measuring light      ");
  Serial1.println(measuringlight1);
  Serial1.print("Teensy pin for saturating light     ");
  Serial1.println(saturatinglight1);
  Serial1.print("Teensy pin for calibration light    ");
  Serial1.println(calibratinglight1);
  Serial1.print("Teensy pin for detector             ");
  Serial1.println(detector1);
  Serial1.println("");

}

// USER PRINTOUT OF TEST RESULTS
void dcalculations() {

  Serial1.println("ALL DATA IN CURRENT DIRECTORY - BASELINE ADJUSTED VALUES");

  int16_t c;
  sub1.open(filenamedir, O_READ);
  strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
  strcat(filenamelocal, dirkfending);
  Serial1.print("file directory: ");
  Serial1.println(filenamedir);
  Serial1.print("file name: ");
  Serial1.println(filenamelocal);
  file.open(dir, filenamelocal, O_READ);
  while ((c = file.read()) > 0) Serial1.write((char)c); // copy data to the Serial1 port
  strcpy(filenamelocal,filename); // reset the localfilename variable;

  file.close(); // close out the file
  sub1.close(); // close out the director (you MUST do this - if you don't, the file won't save the data!)

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial1.println("");
  Serial1.print("Size of the baseline:  ");
  Serial1.println(baseline, 8);

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial1.println("");
  Serial1.println("GENERAL INFORMATION");
  Serial1.println("");

  Serial1.print("total run length (measuring pulses):  ");
  Serial1.println(totaltimecheck);

  Serial1.print("expected run length(measuring pulses):  ");
  Serial1.println(drunlength/(dcyclelength*1000000));

  Serial1.print("total run length (calibration pulses):  ");
  Serial1.println(caltotaltimecheck);

  Serial1.println("");
  Serial1.println("CALIBRATION DATA");
  Serial1.println("");


  Serial1.print("The baseline from the sample is:  ");
  Serial1.println(baseline);
  Serial1.print("The calibration value using the reflective side for the calibration LED and measuring LED are:  ");
  Serial1.print(rebeltinvalue);
  Serial1.print("and ");
  Serial1.println(irtinvalue);
  Serial1.print("The calibration value using the black side for the calibration LED and measuring LED are:  ");
  Serial1.print(rebeltapevalue);
  Serial1.print("and ");
  Serial1.println(irtapevalue);

  delay(50);

}




























void wasp() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

  for (y=0;y<wrepeatrun;y++) {

    strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
    strcat(filenamelocal, waspending); // add the variable's extension defined in header
    sub1.open(filenamedir, O_READ);
    file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
    strcpy(filenamelocal,filename); // reset the localfilename variable;

    // SAVE CURRENT TIME TO SD CARD
    printTime();

    Serial1.print("THIS IS THE BASELINE: ");
    Serial1.println(baseline);  

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
    file.close(); // close out the file
    sub1.close(); // close out the director (you MUST do this - if you don't, the file won't save the data!)

    // delay(300000); // wait 5 minutes between measurements

  }
}

void wpulseon() {
  //  Serial1.print(z);
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
  Serial1.println(wdatasample[z]);
  file.print(wdatasample[z]);
  file.print(",");  
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
  Serial1.print("Total run time is ~: ");
  Serial1.println(end1-starttimer0);
  Serial1.println("");
  Serial1.println("Average values for data measurements 1,2,3 and 4:  ");
  Serial1.println(wdatasampleaverage1*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial1.println(wdatasampleaverage2*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial1.println(wdatasampleaverage3*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial1.println(wdatasampleaverage4*3/(wnumberpulses1+wnumberpulses2+wnumberpulses3));
  Serial1.println("");
  wdatasampleaverage1 = 0;
  wdatasampleaverage2 = 0;
  wdatasampleaverage3 = 0;
  wdatasampleaverage4 = 0;
}

// USER PRINTOUT OF MAIN PARAMETERS
void wprintout() {

  Serial1.println("");
  Serial1.print("Pulse cycles during calibration                    ");
  Serial1.println(calpulsecycles); // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
  Serial1.print("Length of a single measuring pulse in us, set1     ");
  Serial1.println(wpulselengthon1); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
  Serial1.println("Length of a single measuring pulse in us, set2           ");
  Serial1.println(wpulselengthon2); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
  Serial1.print("Length of a single measuring pulse in us, set3     ");
  Serial1.println(wpulselengthon3); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
  Serial1.print("Base data file name                                ");
  Serial1.println(filename);
  Serial1.println("");
  Serial1.print("Teensy pin for measuring light      ");
  Serial1.println(measuringlight1);
  Serial1.print("Teensy pin for saturating light     ");
  Serial1.println(saturatinglight1);
  Serial1.print("Teensy pin for calibration light    ");
  Serial1.println(calibratinglight1);
  Serial1.print("Teensy pin for detector             ");
  Serial1.println(detector1);
  Serial1.println("");

}

// USER PRINTOUT OF TEST RESULTS
void wcalculations() {

  Serial1.println("DATA - RAW ANALOG VALUES IN uV");
  for (i=0;i<(wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3);i++) {
    wdatasample[i] = 1000000*((reference*wdatasample[i])/(analogresolutionvalue*wmeasurements)); 
  }
  for (i=0;i<(wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3);i++) { // Print the results!
    Serial1.print(wdatasample[i], 8);
    Serial1.print(",");
  }
  Serial1.println("");

  Serial1.println("ALL DATA IN CURRENT DIRECTORY - BASELINE ADJUSTED VALUES");

  int16_t c;
  sub1.open(filenamedir, O_READ);
  strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
  strcat(filenamelocal, waspending);
  Serial1.print("file directory: ");
  Serial1.println(filenamedir);
  Serial1.print("file name: ");
  Serial1.println(filenamelocal);
  file.open(dir, filenamelocal, O_READ);
  while ((c = file.read()) > 0) Serial1.write((char)c); // copy data to the Serial1 port
  strcpy(filenamelocal,filename); // reset the localfilename variable;

  file.close(); // close out the file
  sub1.close(); // close out the director (you MUST do this - if you don't, the file won't save the data!)

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial1.println("");
  Serial1.print("Size of the baseline:  ");
  Serial1.println(baseline, 8);

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial1.println("");
  Serial1.println("GENERAL INFORMATION");
  Serial1.println("");

  Serial1.print("total run length (measuring pulses):  ");
  Serial1.println(totaltimecheck);

  Serial1.print("expected run length(measuring pulses):  ");
  Serial1.println(wcyclelength1*wnumberpulses1+wcyclelength2*wnumberpulses2+wcyclelength3*wnumberpulses3);

  Serial1.print("total run length (calibration pulses):  ");
  Serial1.println(caltotaltimecheck);

  Serial1.println("");
  Serial1.println("CALIBRATION DATA");
  Serial1.println("");


  Serial1.print("The baseline from the sample is:  ");
  Serial1.println(baseline);
  Serial1.print("The calibration value using the reflective side for the calibration LED and measuring LED are:  ");
  Serial1.print(rebeltinvalue);
  Serial1.print("and ");
  Serial1.println(irtinvalue);
  Serial1.print("The calibration value using the black side for the calibration LED and measuring LED are:  ");
  Serial1.print(rebeltapevalue);
  Serial1.print("and ");
  Serial1.println(irtapevalue);

  delay(50);

}

/*  code to process time sync messages from the Serial1 port   */
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by unix time_t as ten ascii digits
#define TIME_HEADER  'T'   // Header tag for Serial1 time sync message

time_t processSyncMessage() {
  // return the time if a valid sync message is received on the Serial1 port.
  while(Serial1.available() >=  TIME_MSG_LEN ){  // time message consists of a header and ten ascii digits
    char c = Serial1.read() ; 
    Serial1.print(c);  
    if(c == TIME_HEADER ) {       
      time_t pctime = 0;
      for(int i=0; i < TIME_MSG_LEN -1; i++){   
        c = Serial1.read();          
        if( c >= '0' && c <= '9'){   
          pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
        }
      }   
      return pctime;
      Serial1.println("this is pc time");
      Serial1.println(pctime);
    }  
  }
  return 0;
  i=0; // reset i
}

// PRINT CURRENT TIME TO DISPLAY IN Serial1 PORT
void Serial1PrintClock(){
  Serial1.print(month());
  Serial1.print("/");
  Serial1.print(day());
  Serial1.print("/");
  Serial1.print(year()); 
  Serial1.print(" ");
  Serial1.print(hour());
  Serial1printDigits(minute());
  Serial1printDigits(second());
  Serial1.println(); 
}

void Serial1printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial1.print(":");
  if(digits < 10)
    Serial1.print('0');
  Serial1.print(digits);
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
}

void loop(){
}




