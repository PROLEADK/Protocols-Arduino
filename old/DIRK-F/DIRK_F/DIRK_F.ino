/*
DESCRIPTION:
  DIRK-F stands for Dark Inra-Red Response Kinetics.  This method has a ~40ms ON, followed by a 10ms OFF cycle.  Measurements occur at the end fo the ON cycle every 10us, during the OFF cycle
  ... every 1ms, and at the beginning of the ON cycle every 1ms.  Currently, there's a total of 25 measurements per ON/OFF cycle
  ... see file "DIRK-F Method.png" for full description.
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


// SPECS USING THIS METHOD: 
// Peak to Peak variability, ON and OFF length variability all is <1us.  Total measurement variability = 0 to +3us (regardless of how many cycles)
// Max pulsecycles is based on the saved array size, and dependent on total number of values which will be saved to the array

// NOTES:
// SD card save, data array, serial print of data and jitter is all good.  see actual versus estimated run time in calculations() to confirm timing
// don't quite get the response curve I'd expect... the analog pin is working fine (as seen using standard pwm file, and on scope).  Also, the reads are saving correctly to the array
// ... and the array is being displayed correct on Serial.print.  There's a large spike at the beginning of the ON cycle, which is visible on the scope, so the readings are measuring real response
// ... so the question is... why is there very little dropoff 
// baseline adjustment is on, and baseline is measured using basicfluor() in the file standard_measurement_pmf_autocal3.  

// SD CARD ENABLE AND SET
#include <SdFat.h>
#include <SdFatUtil.h> // use functions to print strings from flash memory

const uint8_t SD_CHIP_SELECT = SS;

SdFat sd;
#define error(s) sd.errorHalt_P(PSTR(s))  // store error strings in flash to save RAM

// KEY VARIABLES USED IN THE PROTOCOL

int measurementsbeforeoff = 5; // number of analogRead()s which occur before the OFF cycle (MIN = 1)
unsigned long delaybeforeoff = 10; // microsecond delay between measurements before the OFF cycle (MIN = 10us)
int measurementsduringoff = 10; // number of analogRead()s which occur during the OFF cycle (MIN = 1)
unsigned long delayduringoff = 1000; // microsecond delay between measurements during OFF cycle (MIN = 10us)
int measurementsafteroff = 10; // number of analogRead()s which occur after the OFF cycle ends (MIN = 1)
unsigned long delayafteroff = 1000; // microsecond delay between measurements after OFF cycle ends (min = 10us)
unsigned long delaybetweencycles = 49950; // microseconds delay between last measurement of current cycle and beginning of measurement of new cycle (no min)

unsigned long calpulsecycles = 50; // Number of times the "pulselengthon" and "pulselengthoff" cycle during calibration (on/off is 1 cycle)
// data for measuring and saturating pulses --> to calculate total time=pulsecycles*(pulselengthon + pulselengthoff)
int pulsecycles = 20; // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
// int saturatingcycleon = 1; //The cycle number in which the saturating light turns on (set = 0 for no saturating light)
// int saturatingcycleoff = 1; //The cycle number in which the saturating light turns off
//unsigned long pulselengthon = 40000; // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
//unsigned long pulselengthoff = 10000; // Pulse LED off length in uS (minimum = 20us + any additional operations which you may want to call during that time).
unsigned long calpulselengthon = 30; // Pulse LED on length for calibration in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
unsigned long calpulselengthoff = 49970; // Pulse LED off length for calibration in uS (minimum = 20us + any additional operations which you may want to call during that time).
char filename[13] = "DIRKF"; // Base filename used for data files and directories on the SD Card
float reference = 3; // The reference (AREF) supplied to the ADC
int analogresolution = 16; // Set the resolution of the analog to digital converter (max 16 bit, 13 bit usable)  
int measuringlight1 = 3; // Teensy pin for measuring light
int saturatinglight1 = 4; // Teensy pin for saturating light
int calibratinglight1 = 2; // Teensy pin for calibrating light
int detector1 = A10; // Teensy analog pin for detector

// INTERNAL VARIABLES, COUNTERS, ETC.
unsigned long start1,start1orig,end1, calstart1orig, calend1, start2, start3, start5, start6, start7, end5, start2orig, start3orig, start4orig, start5orig;
unsigned long pulselengthoncheck, pulselengthoffcheck, pulsecyclescheck, totaltimecheck, caltotaltimecheck;
float data1f, data2f, data3f, data4f, irtinvalue, irtapevalue, rebeltapevalue, rebeltinvalue, irsamplevalue, rebelsamplevalue, baselineir, dataaverage, caldataaverage1, caldataaverage2, rebelslope, irslope, baseline = 0;
char filenamedir[13];
char filenamelocal[13];
int data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, data13, data14, data15, caldata1, caldata2, caldata3, caldata4, analogresolutionvalue;
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
//char data[8] = "data";
//char datab[8] = "data";

void setup() {

  delay(3000); // give operator time to turn on the serial monitor and the capacitor to charge

  Serial.begin(38400); // set baud rate for serial communication
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
        Serial.println(filename);
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
        dirkf();
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

void dirkf() {
  // Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle

  datasample = (int*)malloc(pulsecycles*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff)*sizeof(int)); // Create the array of proper size to save one value for all each ON/OFF cycle
  noInterrupts();
 
start1orig = micros();
start1 = micros();

for (i=0;i<pulsecycles;i++) { // Main loop based on pulsecycles

start2orig = micros();

//(i*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff)+k)
//analogRead(detector1)

  for (k=0;k<measurementsbeforeoff;k++) {  // measurements just before OFF cycle
    datasample[(i*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff)+k)] = analogRead(detector1);
    start1 = start1 + delaybeforeoff;
    while (micros()<start1) {}    
  }
  start3orig = micros();
  digitalWriteFast(measuringlight1, LOW); 
  for (k=measurementsbeforeoff;k<(measurementsbeforeoff+measurementsduringoff);k++) { // measurements during OFF cycle
    datasample[(i*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff)+k)] = analogRead(detector1);
    start1 = start1 + delayduringoff;
    while (micros()<start1) {}
  } 
  start4orig = micros();
  digitalWriteFast(measuringlight1, HIGH); 
  for (k=(measurementsbeforeoff+measurementsduringoff);k<(measurementsbeforeoff+measurementsduringoff+measurementsafteroff);k++) {  // measurements after OFF cycle
    datasample[(i*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff)+k)] = analogRead(detector1);
    start1 = start1 + delayafteroff;
    while (micros()<start1) {}   
  }
  start5orig = micros();
  start1 = start1 + delaybetweencycles;
  while (micros()<start1) {}
}

end1 = micros();
digitalWriteFast(measuringlight1, LOW); 

interrupts();
free(datasample); // release the memory allocated for the data

// SUBTRACT BASELINE FROM SAMPLE VALUES
for (i=0;i<pulsecycles*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff); i++) {
  datasample[i] = datasample[i] - baseline;
}


Serial.println(end1 - start1orig);
Serial.println(end1 - start2orig);
Serial.println(end1 - start3orig);
Serial.println(end1 - start4orig);
Serial.println(end1 - start5orig);

Serial.println("SAMPLE RESULTS");
for (i=0;i<pulsecycles*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff); i++) {
  Serial.print(datasample[i]);
  Serial.print(",");
  if (i%(measurementsbeforeoff+measurementsduringoff+measurementsafteroff) == ((measurementsbeforeoff+measurementsduringoff+measurementsafteroff)-1)) {
    Serial.println("");
  }
}

// SAVE DATA TO SD CARD

  strcpy(filenamelocal,filename); // set the local filename variable with the extension for this subroutine.
  strcat(filenamelocal, "-I.CSV"); // NOTE! "filename" + calibration name extension (CAN ONLY BE A DASH (-) AND 1 LETTER!).  This is the name of file that this subroutine data is stored in
  SdFile sub1;
  sub1.open(filenamedir, O_READ);
  SdBaseFile* dir = &sub1;
  SdFile file;
  file.open(dir, filenamelocal, O_CREAT | O_WRITE | O_APPEND);
  strcpy(filenamelocal,filename); // reset the localfilename variable;

for (i=0;i<pulsecycles*(measurementsbeforeoff+measurementsduringoff+measurementsafteroff); i++) {
  file.print(datasample[i]);
  file.print(",");
  if (i%(measurementsbeforeoff+measurementsduringoff+measurementsafteroff) == ((measurementsbeforeoff+measurementsduringoff+measurementsafteroff)-1)) {
    file.println("");
  }
}

file.close();
  
}

// USER PRINTOUT OF MAIN PARAMETERS
void printout() {

  Serial.println("");
  Serial.print("Pulse cycles during calibration                    ");
  Serial.println(calpulsecycles); // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
  Serial.print("Pulse cycles during fluorescence                   ");
  Serial.println(pulsecycles); // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
//  Serial.print("Pulse cycle # when saturation pulse turns on       ");
//  Serial.println(saturatingcycleon); //The cycle number in which the saturating light turns on (set = 0 for no saturating light)
//  Serial.print("Pulse cycle # when saturation pulse turns off      ");
//  Serial.println(saturatingcycleoff); //The cycle number in which the saturating light turns off
  Serial.print("Length of a single measuring pulse in us           ");
//  Serial.println(pulselengthon); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
//  Serial.print("Length of the wait time between pulses pulse in us ");
//  Serial.println(pulselengthoff); // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
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
  Serial.println(pulsecycles*(measurementsbeforeoff*delaybeforeoff + measurementsduringoff*delayduringoff + measurementsafteroff*delayafteroff + delaybetweencycles));
    

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

//Serial.println(end5-start7);
//Serial.println(end5-start6);
//Serial.println(end5-start7);
}

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



