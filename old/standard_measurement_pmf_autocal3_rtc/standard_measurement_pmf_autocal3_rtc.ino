/*
DESCRIPTION:
 This file is used to measure pulse modulated fluorescence (PMF) using a saturating pulse and a measuring pulse.  The measuring pulse LED (for example, Rebel Luxeon Orange) can itself produce some infra-red
 fluorescence which is detected by the detector.  This signal causes the detector response to be higher than expected which causes bias in the results.  So, in order to account for this added infra-red, we 
 perform a calibration.  The calibration is a short set of pulses which occur at the beginning of the program from an infra-red LED (810nm).  The detector response from these flashes indicates how "reflective"
 the sample is to infra-red light in the range produced by our measuring LED.  We can create a calibration curve by measuring a very reflective sample (like tin foil) and a very non-reflective sample (like
 black electrical tape) with both the calibrating LED and the measuring LED.  This curve tells us when we see detector response X for the calibrating LED, that correspond to a baseline response of Y for the
 measuring LED.  Once we have this calibration curve, when we measure an actual sample (like a plant leaf) we automatically calculate and remove the reflected IR produced by the measuring pulse.  Vioala!

// rtc - includes sync to real time clock in ASCII 10 digit format and printed time with each sample

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

// NOTES:
// Chris needs to add an experiment start time stamp for setTime() at beginning of program
// time format: %m/%d/%Y %H:%M:%S
// Currently, saving the time to the sd card (file.print(hour())... etc.) is done within
// the main subroutine.  This should ideally be a separate routine (like serialprintClock) but
// I can't get it to pass along the file. information for the SD card... how to fix?

// SPECS USING THIS METHOD: 
// Peak to Peak variability, ON and OFF length variability all is <1us.  Total measurement variability = 0 to +3us (regardless of how many cycles)

// SD CARD ENABLE AND SET
#include <SdFat.h>
#include <SdFatUtil.h> // use functions to print strings from flash memory
#include <Time.h>   // enable real time clock library
#include <Wire.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a atime_t
#include "PITimer.h"

const uint8_t SD_CHIP_SELECT = SS;

SdFat sd;
#define error(s) sd.errorHalt_P(PSTR(s))  // store error strings in flash to save RAM

// KEY VARIABLES USED IN THE PROTOCOL
unsigned long calpulsecycles = 50; // Number of times the "pulselengthon" and "pulselengthoff" cycle during calibration (on/off is 1 cycle)
// data for measuring and saturating pulses --> to calculate total time=pulsecycles*(pulselengthon + pulselengthoff)
int pulsecycles = 100; // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
int saturatingcycleon = 10; //The cycle number in which the saturating light turns on (set = 0 for no saturating light)
int saturatingcycleoff = 30; //The cycle number in which the saturating light turns off
unsigned long pulselengthon = 25; // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
unsigned long pulselengthoff = 49975; // Pulse LED off length in uS (minimum = 20us + any additional operations which you may want to call during that time).
unsigned long calpulselengthon = 25; // Pulse LED on length for calibration in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
unsigned long calpulselengthoff = 49970; // Pulse LED off length for calibration in uS (minimum = 20us + any additional operations which you may want to call during that time).
char filename[13] = "ALGAE"; // Base filename used for data files and directories on the SD Card
float reference = 3; // The reference (AREF) supplied to the ADC
int analogresolution = 16; // Set the resolution of the analog to digital converter (max 16 bit, 13 bit usable)  
int measuringlight1 = 3; // Teensy pin for measuring light
int saturatinglight1 = 4; // Teensy pin for saturating light
int calibratinglight1 = 2; // Teensy pin for calibrating light
int detector1 = A10; // Teensy analog pin for detector

// INTERNAL VARIABLES, COUNTERS, ETC.
unsigned long start1,start1orig,end1, calstart1orig, calend1, start5, start6, start7, end5;
unsigned long pulselengthoncheck, pulselengthoffcheck, pulsecyclescheck, totaltimecheck, caltotaltimecheck;
float data1f, data2f, data3f, data4f, irtinvalue, irtapevalue, rebeltapevalue, rebeltinvalue, irsamplevalue, rebelsamplevalue, baselineir, dataaverage, caldataaverage1, caldataaverage2, rebelslope, irslope, baseline = 0;
char filenamedir[13];
char filenamelocal[13];
int data1, data2, data3, data4, data5, data6, caldata1, caldata2, caldata3, caldata4, analogresolutionvalue;
int i = 0; // Used as a counter
int j = 0; // Used as a counter
int k = 0; // Used as a counter
int* caldatatape;
int* caldatatin;
int* caldatasample;
int* datasample;
int val = 0;
int cal = 0;
int cal2 = 0;
int cal3 = 0;  
int val2 = 0;
int flag = 0;
int flag2 = 0;
int keypress = 0;
//char c;

void setup() {

  delay(3000); // give operator time to turn on the serial monitor and the capacitor to charge
  Serial.begin(115200); // set baud rate for serial communication

// SET SYSTEM TIME IN ASCII "T" PLUS 10 DIGITS
  setSyncProvider(Teensy3Clock.get);
  while (!Serial);  // Wait for Arduino Serial Monitor to open
  if(timeStatus()!= timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time"); // NOTE! Chris enter experiment start time stamp here
  
//  for (i=0;i<15;i++) { 
  Serial.println("Please type in T followed by 10 digit ASCII time (eg 'T1231231231')");
  Serial.println("(if you make a mistake, restart teensy and reenter)");
//  Serial.println(Serial.available());
  while (Serial.available()<11) {
// c = Serial.read();
//  Serial.print(Serial.available());
//  Serial.print(",");
//  Serial.println(Serial.peek());
//  delay(500);
  }
  atime_t t = processSyncMessage();
  Serial.print("'T' plus ASCII 10 digit time: ");
  Serial.println(t);
  Serial.print("Serial buffer size: ");
  Serial.println(Serial.available());
  setTime(t);          
  Serial.print("UTC time: ");
  SerialPrintClock();  
//  delay(3000);
//  digitalClockDisplay();
//  }
  Serial.println("");

  pinMode(measuringlight1, OUTPUT); // set pin to output
  pinMode(saturatinglight1, OUTPUT); // set pin to output
  pinMode(calibratinglight1, OUTPUT); // set pin to output  
  analogReadAveraging(1); // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u
  pinMode(detector1, EXTERNAL);
  analogReadRes(analogresolution);
  analogresolutionvalue = pow(2,analogresolution); // calculate the max analogread value of the resolution setting
//  Serial.print(analogresolutionvalue);
  
  if (!sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED)) sd.initErrorHalt(); // Set SD Card to full speed ahead!

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
  printout();

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
      cal3 = Serial.read()-48; // for some crazy reason, inputting 0 in the Serial Monitor yields 48 on the Teensy itself, so this just shifts everything by 48 so that when you input X, it's saved in Teensy as X.
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
    SdFile sub1;
    sub1.makeDir(sd.vwd(), filenamedir); // Creating new folder

  }



   // MAIN PROGRAM
  Serial.println("Would you like to run a calibration? -- type 1 for yes or 2 for no");
  while (cal == 0 | flag == 0) {
    cal = Serial.read()-48; //
    if (cal == 1) {
      delay(50);
      Serial.println("Ok - calibrating and then running normal protocal");
      Serial.println("Place the shiny side of the calibration piece face up in the photosynq, and close the lid.");
      Serial.println("When you're done, press any key to continue");
      Serial.flush();
      while (Serial.read() <= 0) {
      }

      Serial.println("Thanks - calibrating...");
      calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtin()!
      calibrationtin();

      Serial.println("Now place the black side of the calibration piece face up in the photosynq, and close the lid.");
      Serial.println("When you're done, press any key to continue");
      Serial.flush();
      while (Serial.read() <= 0) {
      }

      Serial.println("Thanks - calibrating...");
      calibrationrebel(); // order matters here - make sure that calibrationrebel() is BEFORE calibrationtape()!
      calibrationtape();
      Serial.println("Calibration finished!");
      cal = 2;
    }
    if (cal == 2) {
      delay(50);
      Serial.println("Proceeding with normal measurement protocol");
      Serial.println("Place the sample in front of the light guide and press any key to continue");
      Serial.flush();
      while (Serial.read() <= 0) {
      }
      Serial.println("Thanks - running protocol...");
      while (1) {
        calibrationsample(); // order matters here - calibrationsample() must be BEFORE basicflour()
        basicfluor();
        calculations();
        Serial.println("");        
        Serial.println("Press any key to take another measurement, or press reset button to calibrate or start a new file");
        while (Serial.read() <= 0) {
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


void calibrationrebel() {

  // CALIBRATION REBEL
  // Short pulses for calibration using the measuring LED (rebel orange)
  
    datasample = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts();

  start1orig = micros();
  start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(measuringlight1, HIGH); 
    data1 = analogRead(detector1); // NOTE! user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
    data2 = analogRead(detector1);
    data3 = analogRead(detector1);
    data4 = analogRead(detector1);
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(measuringlight1, LOW); // 
    datasample[i] = (data1+data2+data3+data4); // NOTE! ADJUST BASELINE MULTIPLIER BASED ON NUMBER OF DATA SAMPLED DURING MEASURING PULSE!  data is stored as an array, size = calpulsecycles.  Each point in the array is the average of the data points from an ON cycle.  Averaging is done later.
    while (micros()<start1) {
    } 
  }
  end1 = micros();
  interrupts();

  free(datasample); // release the memory allocated for the data

  for (i=0;i<calpulsecycles;i++) {
    rebelsamplevalue += datasample[i]; // totals all of the analogReads taken
  }
  delay(50);
//  Serial.print("THIS IS THE BASELINE: ");
//  Serial.println(baseline);  
  rebelsamplevalue = (float) rebelsamplevalue; // create a float for rebelsamplevalue so it can be saved later
  rebelsamplevalue = (rebelsamplevalue / (calpulsecycles*4)); //  NOTE!  Divide by the total number of samples to get the average reading during the calibration - NOTE! The divisor here needs to be equal to the number of analogReads performed above!!!! 
  Serial.print("Rebel sample value:  ");
  Serial.println(rebelsamplevalue);
  Serial.println("");  
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(datasample[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println("");
  Serial.println("see the data here:");  
  Serial.println(data1);
  Serial.println(data2);
  Serial.println(data3);
  Serial.println(data4);
  Serial.println(data5);
  Serial.println(data6);
 
}


void calibrationtin() {

  // CALIBRATION TIN
  // Flash calibrating light to determine how reflective the sample is to ~850nm light with the tin foil as the sample (low reflectance).  This has been tested with Luxeon Rebel Orange as measuring pulse.

  caldatatin = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts(); // turn off interrupts to reduce interference from other calls

    start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(calibratinglight1, HIGH);
    caldata1 = analogRead(detector1); // NOTE! user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
    caldata2 = analogRead(detector1);
    caldata3 = analogRead(detector1);
    caldata4 = analogRead(detector1);
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(calibratinglight1, LOW); 
    caldatatin[i] = (caldata1+caldata2+caldata3+caldata4); // data is stored as an array, size = calpulsecycles.  Each point in the array is the average of the data points from an ON cycle.  Averaging is done later.
    //    file.print(caldatatin[i]); // (~10us) store data point on the SD Card
    //    file.println(""); // move to next line in .csv (creates new column in spreadsheet) 
    while (micros()<start1) {
    } 
  }

  interrupts();
  //  file.print(","); // move to next line in .csv (creates new column in spreadsheet) 
  //  free(caldatatin); // release the memory allocated for the data

  for (i=0;i<calpulsecycles;i++) {
    irtinvalue += caldatatin[i]; // totals all of the analogReads taken
  }
  Serial.println(irtinvalue);  
  irtinvalue = (float) irtinvalue;
  irtinvalue = (irtinvalue / (calpulsecycles*4)); //  Divide by the total number of samples to get the average reading during the calibration - NOTE! The divisor here needs to be equal to the number of analogReads performed above!!!! 
  rebeltinvalue = rebelsamplevalue;
  rebelsamplevalue = (int) rebelsamplevalue; // reset rebelsamplevalue to integer for future operations
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(caldatatin[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println(" ");    
  Serial.print("the baseline high reflectace value from calibration: ");
  Serial.println(irtinvalue, 7);
  Serial.print("The last 4 data points from the calibration: ");  
  Serial.print(caldata1);
  Serial.print(", ");
  Serial.print(caldata2);
  Serial.print(", ");
  Serial.print(caldata3);
  Serial.print(", ");
  Serial.println(caldata4);
}

void calibrationtape() {
  
  // CALIBRATION TAPE
  // Flash calibrating light to determine how reflective the sample is to ~850nm light with the black tape as the sample (low reflectance).  This has been tested with Luxeon Rebel Orange as measuring pulse.

  caldatatape = (int*)malloc(calpulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts(); // turn off interrupts to reduce interference from other calls

    start1 = micros();
  for (i=0;i<calpulsecycles;i++) {
    digitalWriteFast(calibratinglight1, HIGH);
    caldata1 = analogRead(detector1); // NOTE! user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
    caldata2 = analogRead(detector1);
    caldata3 = analogRead(detector1);
    caldata4 = analogRead(detector1);
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(calibratinglight1, LOW);
    caldatatape[i] = (caldata1+caldata2+caldata3+caldata4); // data is stored as an array, size = calpulsecycles.  Each point in the array is the average of the data points from an ON cycle.  Averaging is done later.
    //    file.print(caldatatape[i]); // (~10us) store data point on the SD Card
    //    file.println(""); // move to next line in .csv (creates new column in spreadsheet) 
    while (micros()<start1) {
    } 
  }

  interrupts();
  //  file.print(","); // move to next line in .csv (creates new column in spreadsheet) 
  //  free(caldatatape); // release the memory allocated for the data

  for (i=0;i<calpulsecycles;i++) {
    irtapevalue += caldatatape[i]; // totals all of the analogReads taken
  }
  Serial.println(irtapevalue);
  irtapevalue = (float) irtapevalue;
  irtapevalue = (irtapevalue / (calpulsecycles*4)); //  Divide by the total number of samples to get the average reading during the calibration - NOTE! The divisor here needs to be equal to the number of analogReads performed above!!!! 
  rebeltapevalue = rebelsamplevalue;
  rebelsamplevalue = (int) rebelsamplevalue; // reset rebelsamplevalue to integer for future operations 
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(caldatatape[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println(" ");    
  Serial.print("the baseline low reflectace value from calibration:  ");
  Serial.println(irtapevalue, 7);
  Serial.print("The last 4 data points from the calibration: ");  
  Serial.print(caldata1);
  Serial.print(", ");
  Serial.print(caldata2);
  Serial.print(", ");
  Serial.print(caldata3);
  Serial.print(", ");
  Serial.println(caldata4);


  //CALCULATE AND SAVE CALIBRATION DATA TO SD CARD
  rebelslope = rebeltinvalue - rebeltapevalue;
  irslope = irtinvalue - irtapevalue;

  SdFile file;

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
  Serial.println(irslope, 8);
  Serial.print("rebel slope: ");  
  Serial.println(rebelslope, 8);
  Serial.print("ir tin value: ");  
  Serial.println(irtinvalue, 8);
  Serial.print("ir tape value: ");  
  Serial.println(irtapevalue, 8);
  Serial.print("rebel tin value: ");  
  Serial.println(rebeltinvalue, 8);
  Serial.print("rebel tape value: ");  
  Serial.println(rebeltapevalue, 8);

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
    caldata1 = analogRead(detector1); // NOTE! user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
    caldata2 = analogRead(detector1);
    caldata3 = analogRead(detector1);
    caldata4 = analogRead(detector1);
    start1=start1+calpulselengthon;
    while (micros()<start1) {
    }
    start1=start1+calpulselengthoff;
    digitalWriteFast(calibratinglight1, LOW); 
    caldatasample[i] = (caldata1+caldata2+caldata3+caldata4); // data is stored as an array, size = calpulsecycles.  Each point in the array is the average of the data points from an ON cycle.  Averaging is done later.
    //    file.print(caldatasample[i]); // (~10us) store data point on the SD Card
    //    file.print(","); // (~10us) store data point on the SD Card   
    while (micros()<start1) {
    } 
  }
  calend1 = micros();

  interrupts();
  //  file.println(""); // move to next line in .csv (creates new column in spreadsheet) 
  //  free(caldatasample); // release the memory allocated for the data

  for (i=0;i<calpulsecycles;i++) {
    irsamplevalue += caldatasample[i]; // totals all of the analogReads taken
  }
  //  Serial.println(irsamplevalue);
  irsamplevalue = (float) irsamplevalue;
  irsamplevalue = (irsamplevalue / (calpulsecycles*4)); //  Divide by the total number of samples to get the average reading during the calibration - NOTE! The divisor here needs to be equal to the number of analogReads performed above!!!! 
  for (i=0;i<calpulsecycles;i++) { // Print the results!
    Serial.print(caldatasample[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println(" ");    
  Serial.print("the baseline sample reflectace value from calibration:  ");
  Serial.println(irsamplevalue, 7);

  // RETRIEVE STORED BASELINE VALUES AND CALCULATE BASELINE VALUE

  SdFile file;
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

  // CALCULATE BASELINE VALUE
  baseline = (rebelslope*(irsamplevalue-irtinvalue)/(irslope-rebeltinvalue));

}

void basicfluor() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

// NOTE! It would be nice to get rid of the filename definition info, as well as posting the time
// stamp and put it into a separate function... but that function won't pass the sd file call
// information along after it's done... how do I do that?  I left the fileprintdigits function commented out at bottom in case I figure this out.
  strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
  strcat(filenamelocal, "-I.CSV"); // NOTE! "filename" + calibration name extension (CAN ONLY BE A DASH (-) AND 1 LETTER!).  This is the name of file that this subroutine data is stored in
  SdFile sub1;
  sub1.open(filenamedir, O_READ);
  SdBaseFile* dir = &sub1;
  SdFile file;
  file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
  strcpy(filenamelocal,filename); // reset the localfilename variable;

// SAVE CURRENT TIME TO SD CARD
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

  datasample = (int*)malloc(pulsecycles*sizeof(int)); // create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts();

  start1orig = micros();
  start1 = micros();
  for (i=0;i<pulsecycles;i++) {
    digitalWriteFast(measuringlight1, HIGH); 
    if (saturatingcycleon == i+1) {
      digitalWriteFast(saturatinglight1, HIGH);  // Turn saturating light on   
    }
    data1 = analogRead(detector1); // NOTE! user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
    data2 = analogRead(detector1);
    data3 = analogRead(detector1);
    data4 = analogRead(detector1);
    start1=start1+pulselengthon;
    while (micros()<start1) {
    }
    start1=start1+pulselengthoff;
    digitalWriteFast(measuringlight1, LOW); // 
    if (saturatingcycleoff == i+1) {
    digitalWriteFast(saturatinglight1, LOW); // Turn saturating light off
    }
    datasample[i] = (data1+data2+data3+data4); // NOTE! ADJUST BASELINE MULTIPLIER BASED ON NUMBER OF DATA SAMPLED DURING MEASURING PULSE!  data is stored as an array, size = calpulsecycles.  Each point in the array is the average of the data points from an ON cycle.  Averaging is done later.
    //    start5 = micros();
    file.print(datasample[i]); // (~10us) store data point on the SD Card
    //    start6 = micros();
    file.print(","); // move to next line in .csv (creates new column in spreadsheet) 
    //    end5 = micros();
    while (micros()<start1) {
    } 
  }
  end1 = micros();
  interrupts();

  file.println(""); // move to next column
  file.close();

  free(datasample); // release the memory allocated for the data

  // TEST SD CARD WRITE SPEED
  //  Serial.print("Time required for 2 write's (sample, comma) to the SD Card: ");
  //  Serial.print(end5 - start5);
  //  Serial.print(", ");
  //  Serial.println(end5 - start6);

  for (i=0;i<pulsecycles;i++) {
    rebelsamplevalue += datasample[i]; // totals all of the analogReads taken
  }
  delay(50);
  Serial.print("THIS IS THE BASELINE: ");
  Serial.println(baseline);  
  rebelsamplevalue = (float) rebelsamplevalue; // create a float for rebelsamplevalue so it can be saved later
  rebelsamplevalue = (rebelsamplevalue / (pulsecycles*4)); //  NOTE!  Divide by the total number of samples to get the average reading during the calibration - NOTE! The divisor here needs to be equal to the number of analogReads performed above!!!! 
  Serial.print("Rebel sample value:  ");
  Serial.println(rebelsamplevalue);
  Serial.println("");  
  for (i=0;i<pulsecycles;i++) { // Print the results!
    Serial.print(datasample[i]);
    Serial.print(", ");
    Serial.print(" ");  
  }
  Serial.println("");

}

// USER PRINTOUT OF MAIN PARAMETERS
void printout() {

  Serial.println("");
  Serial.print("Pulse cycles during calibration                    ");
  Serial.println(calpulsecycles); // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
  Serial.print("Pulse cycles during fluorescence                   ");
  Serial.println(pulsecycles); // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
  Serial.print("Pulse cycle # when saturation pulse turns on       ");
  Serial.println(saturatingcycleon); //The cycle number in which the saturating light turns on (set = 0 for no saturating light)
  Serial.print("Pulse cycle # when saturation pulse turns off      ");
  Serial.println(saturatingcycleoff); //The cycle number in which the saturating light turns off
  Serial.print("Length of a single measuring pulse in us           ");
  Serial.println(pulselengthon); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
  Serial.print("Length of the wait time between pulses pulse in us ");
  Serial.println(pulselengthoff); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
  Serial.print("Base data file name                                ");
  Serial.println(filename);
  Serial.println("");
  Serial.print("Teensy pin for measuring light      ");
  Serial.println(measuringlight1);
  Serial.print("Teensy pin for saturating light     ");
  Serial.println(saturatinglight1);
  Serial.print("Teensy pin for calibration light    ");
  Serial.println(calibratinglight1);
  Serial.print("Teensy pin for detector             ");
  Serial.println(detector1);
  Serial.println("");

}

// USER PRINTOUT OF TEST RESULTS
void calculations() {

  Serial.println("the following two values");
  Serial.println(datasample[5]);
  Serial.println(analogresolutionvalue);
  Serial.println("");
  Serial.println("DATA - RAW ANALOG VALUES (divide by number of samples taken during measurement pulse to get actual value");
  for (i=0;i<pulsecycles;i++) { // Average each of the data points from the sample
    datasample[i] = 1000000*((reference*datasample[i])/(analogresolutionvalue*4)); // NOTE! The divisor here needs to be equal to the number of analogReads performed above, and the other number is the voltage reference (INTERNAL = 1.1, normal = 3.3)
  }
  for (i=0;i<pulsecycles;i++) { // Print the results!
    Serial.print(datasample[i], 8);
    Serial.println("");
  }
  Serial.println("");

  Serial.println("");
  Serial.println("ALL DATA IN CURRENT DIRECTORY - BASELINE ADJUSTED VALUES");
  int16_t c;
  SdFile sub1;
  sub1.open(filenamedir, O_READ);
  SdBaseFile* dir = &sub1;
  SdFile file;
  strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
  strcat(filenamelocal, "-I.CSV"); // NOTE! Make sure this matches the local filename of the output data you'd like to display through Serial.
  file.open(dir, filenamelocal, O_READ);
  while ((c = file.read()) > 0) Serial.write((char)c); // copy data to the serial port
  strcpy(filenamelocal,filename); // reset the localfilename variable;

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial.println("");
  Serial.print("Size of the baseline:  ");
  Serial.println(baseline, 8);

  totaltimecheck = end1 - start1orig;
  caltotaltimecheck = calend1 - calstart1orig;

  Serial.println("");
  Serial.println("GENERAL INFORMATION");
  Serial.println("");

  Serial.print("# of pulse cycles:  ");
  Serial.println(pulsecycles);

  Serial.print("total run length (measuring pulses):  ");
  Serial.println(totaltimecheck);

  Serial.print("expected run length(measuring pulses):  ");
  Serial.println(pulsecycles*(pulselengthon + pulselengthoff));
 
  Serial.print("total run length (calibration pulses):  ");
  Serial.println(caltotaltimecheck);

  Serial.print("value of i:  ");
  Serial.println(i);

  Serial.println("");
  Serial.println("CALIBRATION DATA");
  Serial.println("");


  Serial.print("The baseline from the sample is:  ");
  Serial.println(baseline);
  Serial.print("The calibration value using the reflective side for the calibration LED and measuring LED are:  ");
  Serial.print(rebeltinvalue);
  Serial.println(irtinvalue);
  Serial.print("The calibration value using the black side for the calibration LED and measuring LED are:  ");
  Serial.print(rebeltapevalue);
  Serial.println(irtapevalue);

  delay(50);

}

/*  code to process time sync messages from the serial port   */
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by unix atime_t as ten ascii digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message

atime_t processSyncMessage() {
  // return the time if a valid sync message is received on the serial port.
  while(Serial.available() >=  TIME_MSG_LEN ){  // time message consists of a header and ten ascii digits
    char c = Serial.read() ; 
    Serial.print(c);  
    if(c == TIME_HEADER ) {       
      atime_t pctime = 0;
      for(int i=0; i < TIME_MSG_LEN -1; i++){   
        c = Serial.read();          
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

// PRINT CURRENT TIME TO DISPLAY IN SERIAL PORT
void SerialPrintClock(){
  Serial.print(month());
  Serial.print("/");
  Serial.print(day());
  Serial.print("/");
  Serial.print(year()); 
  Serial.print(" ");
  Serial.print(hour());
  serialprintDigits(minute());
  serialprintDigits(second());
  Serial.println(); 
}

void serialprintDigits(int digits){
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

void loop(){
}





// GREG'S NOTES

// TROUBLESHOOTING:
// WHY!!!!! when I try to refer to another char when naming a file to O_APPEND, O_CREAT, or O_WRITE the program file.open() fails to return anything
// program views upper and lower case the same in the filename
// you can't do two separate O_CREAT followed by another file.open O_WRITE - you have to have them in the same line like this   file.open(name, O_CREAT | O_WRITE);
// you have to have the char name = "" in void Setup() - you can't declare it globally... why not?  You can't declare another global char and set it equal to the local version... annoying
// Char name itself cannot exceed 12 characters total for some reason, even if you declare it to be longer using [value].  If it is >12 char - it simply will not save to the SD Card
// I can't use variables created in void setup() in any other void sections... I have to create them as global variables... however, if I create a char as a global variable, it cannot be used
// ... as the name of a file.

// TESTING
// Try to write 80k file sizes (the number 42 40k times) and save the files.  Check the files and make sure all of the data is there

// NOTES:
// interrupts (in the form of cli() and sei() OR the standard arduino interrupts / noInterrupts) seems to make no difference in the result... though the Interrupts call itself was better than sei()
// based on looking at the actual oscilliscope but not much
// Adding to a String variable using the =+ or the string.concat() version of the string function takes about 8us!
// BUT, setting a simple int data variable equal to our analog read, that takes no time at all.
// when concatenating a string, this is slowest -   data = data + data1; .  It's better to call the concat function once and concatenate multiple things at a time within it, rather than making multiple calls.
// Even for the very quick pulses when using WASP method or other really short methods, we should be able to use the ADC to get 1us timed flashes and then just run analogRead as fast as possible (~3us).  
// Then, we can drop any of the values which are clearly too low (this should be obvious on a graph)... or perhaps we can speed up our analogRead?

// IMPROVEMENTS:
// Even just getting rid of second string concatenation to add the comma (data += ",") would allow us to shorten the OFF time minimum down to 10us.
// IF you can search saved files and find a calibration value which has the same filename AND same resistor value AND it's been more than X time since last calibration
// THEN ask user to calibrate (via Serial port for now)




