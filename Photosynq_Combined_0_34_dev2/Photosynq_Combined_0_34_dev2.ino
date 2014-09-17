// FIRMWARE VERSION OF THIS FILE (SAVED TO EEPROM ON FIRMWARE FLASH)
#define FIRMWARE_VERSION .34

/////////////////////CHANGE LOG/////////////////////
/*

to do:
When we get all devices back ---
put all wait times in milliseconds
 - make all wait times (measurement, protocol, protocol_repeat, average, etc.) are in milliseconds
change intensity of actinic light in baseline calibration (so it doesn't max out).

 Most recent updates (34):
 - added "alert" function between cycles
 - added "prompt" function between cycles, allows user imput which is returned in data JSON as prompt_rsp
 - added "note" which acts as an environmental variable, allows user input at beginning of end of measurement 
 - for all user inputted text (ie "prompt" or "note"), char limit is 999, do not use any of the following: [ ] + " ' 
 - cleaned up unnecessary spaces in output data JSON 
 - added ability to exit any delay (average, protocol, measurement, etc.) using -1+
 - added ability to exit a measurement (between protocols only, not inside a protocol) by using -1+-1+ (all at once, no delay)
 - now temp/relative humidity, co2, and contactless temp return errors (like "no co2 sensor found") instead of 998 or blank or whatever.

 Most recent updates (33):
 - finish ability to perform analog_read, analog_write, digital_read, digital_write during the 'environmental' measurement (before or after spectroscopy).
 - analog_write works - in "environmental", write:  ["analog_read",<location 0/1>,<PWM pin #>,<PWM setting>,<PWM Frequency>,<Time to hold PWM in ms>]
 - Now you can enter the main level number (like 1000+, 1015+ etc.) immediately followed by additional information successfully
 - example - to enter calibration data you can enter 1019+5.432+6.54+-1+
 - Next step is to allow chrome app to auto-push information to the device.
  
 Most recent updates (32):
 - added initial (beta) functionality for analog_read, analog_write, digital_read, and digital_write.
 - added hard-coded firmware firsion (located at top of file) - so now you only enter device_id and manufacture_date with 1013+.
 - fixed memory leak (added additional free(json) and free(data_raw_average) at end of void loop() )
 - NOTE - maximum # of saved (measuring light != 0) data points is 8000 - 10000 depending on the size of the protocol JSON (larger protocol JSON means less memory to save data).
 
 /////////////////////LICENSE/////////////////////
 GPLv3 license
 by Greg Austic
 If you use this code, please be nice and attribute if possible and when reasonable!
 Libraries from Adafruit were used for TMP006 and TCS34725, as were code snippets from their examples for taking the measurements.  Lots of help from PJRC forum as well.  Thanks Adafruit and Paul and PJRC community!
 
 /////////////////////DESCRIPTION//////////////////////
 Using Arduino v1.0.5 w/ Teensyduino installed downloaded from http://www.pjrc.com/teensy/td_download.html, intended for use with Teensy 3.1
 
 For more details on hardware, apps, and other related software, go to https://github.com/Photosynq .  
 
 This file is used for the MultispeQ handheld spectrophotometers which are used in the PhotosynQ platform.  It is capable of taking a wide variety of measurements, including 
 fluorescence, 810 and 940 dirk, electrochromic shift, SPAD (chlorophyll content), and environmental variables like CO2, ambient temperature, contactless object temperature,
 relative humidity, light intensity, and color temperature.  It then outputs the data in a JSON format through the USB (serial) and Bluetooth (serial1).  It is designed to work
 with the PhotosynQ app and website for data storage and analysis.
 
 //////////////////////  TERMINOLOGY: //////////////////////  
 A 'pulse' is a single pulse of an LED, usually a measuring LED.  Pulses last for 5 - 100us
 A 'cycle' is a set of X number of pulses.  During each cycle, the user can set an actinic or other light on (not pulsing, just on throughout the cycle)
 'averages' are the number of times you repeat the same measurement BUT the values are averaged and only a single value is outputted from the device
 'measuring lights' are LEDs which have the ability to very quickly and cleanly turn on and off.
 'actinic lights' are LEDs which turn on and off slightly less quickly (but still very quickly!) but usually have better heat sinks and more capacity to be on without burning out
 and with less burn in artifacts (change in LED intensity due to the LED heating up)
 'calibrating lights' are IR LEDs which are used to calculate the baseline to improve the quality of measurements by calculating the amount of interfering IR there is from the measuring
 light
 
 //////////////////////  FEATURES AND NOTES: //////////////////////  
 USB power alone is not sufficient!  You must also have an additional power supply which can quickly supply power, like a 9V lithium ion battery.  Power supply may be from 5V - 24V
 Calibration is performed to create an accurate reflectance in the sample using a 940nm LED.  You can find a full explanation of the calibration at https://opendesignengine.net/documents/14
 A new calibration should be performed when sample conditions have changed (a new sample is used, the same sample is used in a different position)
 Pin A10 and A11 are 16 bit enabed with some added coding in C - the other pins cannot achieve 16 bit resolution.
 
 The are mutliple calibrations for the unit:
 1) offset calibration - the detectors have a fixed offset due to the hardware, which must be subtracted from any raw outputs.  This is the only calibration which is actively performed on the device - 
 in other works the output has already had the offset subtracted from the signal.  All other calibrations are simply passed along to the app to be applied in post-processing
 2) baseline calibration - 
 
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
 
 */

//#define DEB0UG 1  // uncomment to add full debug features
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
#include <SoftwareSerial.h>
#include "EEPROMAnything.h"
#include <typeinfo>

//////////////////////DEVICE ID FIRMWARE VERSION////////////////////////
float device_id = 0;
float manufacture_date = 0;

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
#define ACTINICLIGHT_INTENSITY_SWITCH 5
#define DETECTOR1 34
#define DETECTOR2 35
#define SAMPLE_AND_HOLD 6
#define PWR_OFF_LIGHTS 22           // teensy shutdown pin auto power off
#define PWR_OFF 21           // teensy shutdown pin auto power off for lights power supply (TL1963)
#define BATT_LEVEL 17               // measure battery level
#define DAC_ON 23

int _meas_light;															 // measuring light to be used during the interrupt
int serial_buffer_size = 5000;                                        // max size of the incoming jsons
int max_jsons = 15;                                                   // max number of protocols per measurement
float all_pins [26] = {
  15,16,11,12,2,20,14,10,34,35,36,37,38,3,4,9,24,25,26,27,28,29,30,31,32,33};
float calibration_slope [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_yint [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_slope_factory [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_yint_factory [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_baseline_slope [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};  
float calibration_baseline_yint [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float calibration_blank1 [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float calibration_blank2 [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float calibration_other1 [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float calibration_other2 [26] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float userdef0[2] = {
};
float userdef1[2] = {
};
float userdef2[2] = {
};
float userdef3[2] = {
};
float userdef4[2] = {
};
float userdef5[2] = {
};
float userdef6[2] = {
};
int averages = 1;
int pwr_off_state = 0;
int pwr_off_lights_state = 0;
int pwr_off_ms = 120000;                                                // number of milliseconds before unit auto powers down.
int act_intensity = 0;
int meas_intensity = 0;
int cal_intensity = 0;
JsonArray act_intensities;                         // write to input register of a DAC. channel 0 for low (actinic).  1 step = +3.69uE (271 == 1000uE, 135 == 500uE, 27 == 100uE)
JsonArray meas_intensities;                        // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
JsonArray cal_intensities;                        // write to input register of a DAC. channel 2 calibrating light.  0 (low) - 4095 (high).

//////////////////////Shared Variables///////////////////////////
volatile int off = 0, on = 0;
int analogresolutionvalue;
IntervalTimer timer0, timer1, timer2;
volatile long data1 = 0;
int act_background_light = 13;
float light_y_intercept = 0;
float light_slope = 3;
float offset_34 = 0;                                                          // create detector offset variables
float offset_35 = 0;
float slope_34 = 0;
float yintercept_34 = 0;
float slope_35 = 0;
float yintercept_35 = 0;
char* bt_response = "OKOKlinvorV1.8OKsetPINOKsetnameOK115200"; // Expected response from bt module after programming is done.
float freqtimer0;
float freqtimer1;
float freqtimer2;

//////////////////////HTU21D Temp/Humidity variables///////////////////////////
#define temphumid_address 0x40                                           // HTU21d Temp/hum I2C sensor address
HTU21D htu;                                                              // create class object
unsigned int tempval;
unsigned int rhval;
int temp_error = 0;

//////////////////////S8 CO2 Variables///////////////////////////
byte readCO2[] = {
  0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};                             //Command packet to read CO2 (see app note)
byte response[] = {
  0,0,0,0,0,0,0};                                                        //create an array to store CO2 response
float valMultiplier = 1;
int CO2calibration = 17;                                                 // manual CO2 calibration pin (CO2 sensor has auto-calibration, so manual calibration is only necessary when you don't want to wait for autocalibration to occur - see datasheet for details 
int co2_error = 0;                                                        // error 1 is "no co2 sensor found", error 2 is "no response from co2 sensor"

////////////////////TMP006 variables - address is 1000010 (adr0 on SDA, adr1 on GND)//////////////////////
Adafruit_TMP006 tmp006(0x42);  // start with a diferent i2c address!  ADR1 is GND, ADR0 is SDA so address is set to 42
float tmp006_cal_S = 6.4;
int ctemp_error = 0;

////////////////////ENVIRONMENTAL variables averages (must be global) //////////////////////
float analog_read_average = 0;
float digital_read_average = 0;
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
#ifdef DEBUGSIMPLE
  Serial.println("serial works");
#endif
  Serial1.begin(115200);                                                        // set baud rate for Serial 1 used for bluetooth communication on pins 0,1
#ifdef DEBUGSIMPLE
  Serial.println("serial1 works");
#endif
  Serial3.begin(9600);                                                          // set baud rate for Serial 3 is used to communicate with the CO2 sensor
#ifdef DEBUGSIMPLE
  Serial.println("serial3 works");
#endif
  delay(500);

  Wire.begin(I2C_MASTER,0x00,I2C_PINS_18_19,I2C_PULLUP_EXT,I2C_RATE_1200);      // using alternative wire library
#ifdef DEBUGSIMPLE
  Serial.println("successfully opened I2C");
#endif
  dac.vdd(5000); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout

  TCS3471.setWaitTime(200.0);
  TCS3471.setIntegrationTime(700.0);
  TCS3471.setGain(TCS3471_GAIN_1X);
  TCS3471.enable();

  pinMode(29,OUTPUT);

  pinMode(3,INPUT);
  pinMode(MEASURINGLIGHT1, OUTPUT);                                             // set appropriate pins to output
  pinMode(MEASURINGLIGHT2, OUTPUT);
  pinMode(MEASURINGLIGHT3, OUTPUT);
  pinMode(MEASURINGLIGHT4, OUTPUT);
  pinMode(ACTINICLIGHT1, OUTPUT);
  pinMode(ACTINICLIGHT2, OUTPUT);
  pinMode(CALIBRATINGLIGHT1, OUTPUT);
  pinMode(CALIBRATINGLIGHT2, OUTPUT);
  pinMode(ACTINICLIGHT_INTENSITY_SWITCH, OUTPUT);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);                     // preset the switch to the actinic (high) preset position, DAC channel 0
  pinMode(SAMPLE_AND_HOLD, OUTPUT);
  pinMode(PWR_OFF, OUTPUT);
  pinMode(PWR_OFF_LIGHTS, OUTPUT);
  pinMode(BATT_LEVEL, INPUT); 
  digitalWriteFast(PWR_OFF, LOW);                                               // pull high to power off, pull low to keep power.
  digitalWriteFast(PWR_OFF_LIGHTS, HIGH);                                               // pull high to power on, pull low to power down.
  pinMode(13, OUTPUT);
  analogReadAveraging(1);                                                       // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u) - this gets changed later by each protocol
  pinMode(DETECTOR1, EXTERNAL);                                                 // use external reference for the detectors
  pinMode(DETECTOR2, EXTERNAL);
  analogReadRes(ANALOGRESOLUTION);                                              // set at top of file, should be 16 bit
  analogresolutionvalue = pow(2,ANALOGRESOLUTION);                              // calculate the max analogread value of the resolution setting
  float default_resolution = 488.28;
  int timer0 [8] = {
    5, 6, 9, 10, 20, 21, 22, 23  };
  int timer1 [2] = {
    3, 4  };
  int timer2 [2] = {
    25, 32  }; 
  analogWriteFrequency(3, 187500);                                              // Pins 3 and 5 are each on timer 0 and 1, respectively.  This will automatically convert all other pwm pins to the same frequency.
  analogWriteFrequency(5, 187500);
  pinMode(DAC_ON, OUTPUT);
  digitalWriteFast(DAC_ON, HIGH);                                               // pull high to power off, pull low to keep power.

  float tmp = 0;
  EEPROM_readAnything(16,tmp);
  if (tmp != FIRMWARE_VERSION) {                    // if the current firmware version isn't what's saved in EEPROM memory, then...
    EEPROM_writeAnything(16,(float) FIRMWARE_VERSION);                                  // save the current firmware version
  }
}

void pwr_off() { 
  digitalWriteFast(PWR_OFF, HIGH);
  delay(50);
  digitalWriteFast(PWR_OFF, LOW);
  Serial.println("{\"pwr_off\":\"HIGH\"}");
  Serial1.println("{\"pwr_off\":\"HIGH\"}");
  Serial.println();
  Serial1.println();
}

void pwr_off_lights() {
  Serial.print("{\"pwr_off_lights\": \"");
  Serial1.print("{\"pwr_off_lights\": \"");   
  if (pwr_off_lights_state == 0) {
    Serial.println("HIGH\"}");
    Serial1.println("HIGH\"}");
    digitalWriteFast(PWR_OFF_LIGHTS, HIGH);
    pwr_off_lights_state = 1;
  }
  else if (pwr_off_lights_state == 1) {
    Serial.println("LOW\"}");
    Serial1.println("LOW\"}");
    digitalWriteFast(PWR_OFF_LIGHTS, LOW);    
    pwr_off_lights_state = 0;
  }
  Serial.println();
  Serial1.println();
}

/*
Battery output:
 batt_level:[bv0,bv1,bv2]
 
 battery data intepretation:
 if bv0 - bv1 < .1 - state: "USB Power Only"
 else if bv1 < 5 && bv0 > 7 - state "Low quality batteries, replace with NiMh or Li"
 else if bv1 < 5 - state "Replace batteries"
 else if bv0 - bv1 > .1, then:
 if bv0 < 6.6 - state "Replace batteries"
 if 6.6 - 7.1 - state "Batteries low"
 if 7.1 - 7.6 - state "Batteries OK"
 if 7.6 + - state "Batteries good"  
 
 Please print both the intepretation and the raw values (we'll want to watch them over time to get more info out of them).
 */

void batt_level() {
  dac.analogWrite(0,4095);                                        // if we are setting actinic equal to the light sensor, then do it!
  digitalWriteFast(DAC_ON, LOW);                                               // pull high to power off, pull low to keep power.
  delayMicroseconds(1);
  digitalWriteFast(DAC_ON, HIGH);                                               // pull high to power off, pull low to keep power.
  float bv0 = ((float) analogRead(BATT_LEVEL) / 2358);                // read battery voltage in normal low draw state
  digitalWriteFast(ACTINICLIGHT1, HIGH);                                               // pull high to power off, pull low to keep power.
  delay(10);
  float bv1 = ((float) analogRead(BATT_LEVEL) / 2358);               // read battery voltage in start of high draw state
  delay(110);
  float bv2 = ((float) analogRead(BATT_LEVEL) / 2358);               // read battery voltage after 100ms of high draw state
  digitalWriteFast(ACTINICLIGHT1, LOW);                                               // pull high to power off, pull low to keep power.
  Serial.print("{\"batt_level\": [");
  Serial1.print("{\"batt_level\": ["); 
  Serial.print(bv0);
  Serial1.print(bv0);
  Serial.print(",");
  Serial1.print(",");
  Serial.print(bv1);
  Serial1.print(bv1);
  Serial.print(",");
  Serial1.print(",");
  Serial.print(bv2);
  Serial1.print(bv2);
  Serial.println("]}");
  Serial1.println("]}");
  Serial.println();
  Serial1.println();
}

void get_calibration(float slope[], float yint[],float _slope, float _yint, JsonArray cal,String name) {
  if (cal.getLong(0) > 0) {                                                          // if get_ir_baseline is true, then...    
    Serial.print("\"");
    Serial.print(name);
    Serial.print("\": [");
    Serial1.print("\"");
    Serial1.print(name);
    Serial1.print("\": [");
    if (_slope != 0) {
      Serial.print(_slope);
      Serial1.print(_slope);
      if (yint [0] != 0) {
        Serial.print(",");
        Serial.print(_yint);                                                  // ignore second value if it's 0
        Serial1.print(",");
        Serial1.print(_yint);                                                  // ignore second value if it's 0
      }
    }
    else if (_slope == 0) {
    for (int z = 0;z<cal.getLength();z++) {                                          // check each pins in the get_ir_baseline array, and for each pin...
      Serial.print("[");
      Serial.print(cal.getLong(z));                                                  // print the pin name
      Serial.print(","); 
      Serial1.print("[");
      Serial1.print(cal.getLong(z));                                                  // print the pin name
      Serial1.print(","); 
      for (int i = 0;i<sizeof(all_pins)/sizeof(int);i++) {                          // look through available pins, pull out the baseline slope and yint associated with requested pin
        if (all_pins[i] == cal.getLong(z)) {
          if (_slope == 0) {                                                        // if this is an array, then print this way...
            Serial.print(slope[i]);
            Serial1.print(slope[i]);
            if (yint [0] != 0) {                                                        
              Serial.print(",");
              Serial.print(yint[i]);                                                // ignore second value if it's 0
              Serial1.print(",");
              Serial1.print(yint[i]);                                                // ignore second value if it's 0
            }
          }
          Serial.print("]");
          Serial1.print("]");
          goto cal_end;                                                                            // bail if you found your pin
        }
      }
cal_end:
      delay(1);
      if (z != cal.getLength()-1) {                                                  // add a comma if it's not the last value
        Serial.print(",");
        Serial1.print(",");
      }
    }
    }
    Serial.print("],");
    Serial1.print("],");
  }
}

void measureit() {
  int i;
  int readit;
  pinMode(A14,INPUT);
  Serial.println(A14);
  
  for (i=0;i<100;i++) {
    readit = analogRead(A14);
    Serial.println(readit); 
    delay(2000);
  }
  /*
  for (i=0;i<100;i++) {
   pinMode(3,OUTPUT);
   digitalWrite(3, HIGH);
   delay(2000);
   digitalWrite(3, LOW);
   delay(2000);
   }
   */
}

//////////////////////// MAIN LOOP /////////////////////////
void loop() {

  delay(50);
  int measurements = 1;                                                   // the number of times to repeat the entire measurement (all protocols)
  unsigned long measurements_delay = 0;                                    // number of milliseconds to wait between measurements  
  volatile unsigned long meas_number = 0;                                       // counter to cycle through measurement lights 1 - 4 during the run
  unsigned long end1;
  unsigned long start1 = millis(); 

  // these variables could be pulled from the JSON at the time of use... however, because pulling from JSON is slow, it's better to create a int to save them into at the beginning of a protocol run and use the int instead of the raw hashTable.getLong type call 
  int _act1_light;
  int _act2_light;
  int _alt1_light;
  int _alt2_light;
  int _act1_light_prev;
  int _act2_light_prev;
  int _alt1_light_prev;
  int _alt2_light_prev;
  int act_background_light_prev = 13;

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
  JsonParser<600> root;
  int number_of_protocols = 0;                                                      // reset number of protocols
  int end_flag = 0;
  int which_serial = 0;
  long* data_raw_average = (long*)malloc(4);
  char serial_buffer [serial_buffer_size];
  String json2 [max_jsons];
  memset(serial_buffer,0,serial_buffer_size);                                    // reset buffer to zero
  for (int i=0;i<max_jsons;i++) {
    json2[i] = "";                                                              // reset all json2 char's to zero (ie reset all protocols)
  }
  call_print_calibration(0);                                                                  // recall all data saved in eeprom

  digitalWriteFast(SAMPLE_AND_HOLD,LOW);                                          // discharge sample and hold in case the cap has be
  delay(10);
  digitalWriteFast(SAMPLE_AND_HOLD,HIGH);

  String choose = "0";
  while (Serial.peek() != '[' && Serial1.peek() != '[') {                      // wait till we see a "[" to start the JSON of JSONS
    choose = user_enter_str(50,1);

    if (choose.toInt() > 0 && choose.toInt() < 200) {                    // if it's 0 - 200 then go see full light of device testing
      lighttests(choose.toInt());
      serial_bt_flush();                                                             // flush serial port if it's none of the above things
    }
    else {
      switch (choose.toInt()) {
      case false:                                                                   // if it's not a number, then exit switch and check to see if it's a protocol
        break;
      case 1000:                                                                    // print "MultispeQ Ready" to USB and Bluetooth
        Serial.println("MultispeQ Ready");
        Serial1.println("MultispeQ Ready");
        break;
      case 1001:                                                                    // power off completely (if USB connected, only batteries power down
        pwr_off();
        break;
      case 1002:                                                                    // configure bluetooth name and baud rate
        configure_bluetooth();
        break;
      case 1003:                                                                    // power down lights (TL1963) only
        pwr_off_lights();
        break;
      case 1004:                                                                    // show battery level and associated values
        batt_level();
        break;
      case 1005:                                                                    // print all calibration data
        call_print_calibration(1);
        break;
      case 1006:                                                                    // calibrate tcs light sensor to actual PAR values
        calibrate_light_sensor();
        break;
      case 1007:                                                                   // view device info  
        set_device_info(0);                                                       
        break;
      case 1008:                                                                    // calibrate offset
        calibrate_offset();
        break;
      case 1011:                                                                    // calibrate the lights (eeprom 60 - 300)
        add_calibration(60);
        break;
      case 1012:                                                                    // factory calibration of lights (eeprom 300 - 540)
        add_calibration(300);
        break;
      case 1013:                                                                    // view and set device info      
        set_device_info(1);                                                                                                            
        break;
      case 1014:                                                                    // calibrate the baseline  (eeprom 540 - 880) 
        add_calibration(540);
        break;
      case 1015:                                                                   // calibrate the spad blanks  (eeprom 880 - 1120)
        add_calibration(880);
        break;
      case 1016:                                                                   // calibrate the other user defined calibrations  (eeprom 1120 - 1360)
        add_calibration(1120);
        break;
      case 1017:                                                                   // reset all saved EEPROM values to zero
        reset_all(0);
        break;
      case 1018:                                                                   // reset only the calibration arrays to zero (leave tcs calibration, offset, device info)
        reset_all(1);
        break;
      case 1019:                                                                   // save spad_factor value to EEPROM
        add_userdef(40);
        break;
      case 1020:                                                                   // save user defined value to EEPROM
        add_userdef(1360);
        break;
      case 1021:                                                                   // save user defined value to EEPROM
        add_userdef(1368);
        break;
      case 1022:                                                                   // save user defined value to EEPROM
        add_userdef(1376);
        break;
      case 1023:                                                                   // save user defined value to EEPROM
        add_userdef(1384);
        break;
      case 1024:                                                                   // save user defined value to EEPROM
        add_userdef(1392);
        break;
      case 1025:                                                                   // save user defined value to EEPROM
        add_userdef(1400);
        break;
      case 1026:
        measureit();
      }
    }
  }        
  if (Serial.peek() == 91) {
#ifdef DEBUGSIMPLE
    Serial.println("computer serial");
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
    Serial.setTimeout(2000);                                          // make sure set timeout is 1 second
    Serial.readBytesUntil('!',serial_buffer,serial_buffer_size);
    for (int i=0;i<5000;i++) {                                    // increments through each char in incoming transmission - if it's open curly, it saves all chars until closed curly.  Any other char is ignored.
      if (serial_buffer[i] == '{') {                               // wait until you see a open curly bracket
        while (serial_buffer[i] != '}') {                          // once you see it, save incoming data to json2 until you see closed curly bracket
          json2[number_of_protocols] +=serial_buffer[i];
          i++;
        }
        json2[number_of_protocols] +=serial_buffer[i];            // catch the last closed curly
        number_of_protocols++;
      }        
    }  
    for (int i=0;i<number_of_protocols;i++) {
      if (json2[i] != '0') {
#ifdef DEBUGSIMPLE
        Serial.print("Incoming JSON as received by Teensy:   ");
        Serial.println(json2[i]);
#endif
      }
    }
    break;

  case 2:
    Serial1.setTimeout(1000);                                          // make sure set timeout is 1 second
    Serial1.readBytesUntil('!',serial_buffer,serial_buffer_size);
    for (int i=0;i<5000;i++) {                                    // increments through each char in incoming transmission - if it's open curly, it saves all chars until closed curly.  Any other char is ignored.
      if (serial_buffer[i] == '{') {                               // wait until you see a open curly bracket
        while (serial_buffer[i] != '}') {                          // once you see it, save incoming data to json2 until you see closed curly bracket
          json2[number_of_protocols] +=serial_buffer[i];
          i++;
        }
        json2[number_of_protocols] +=serial_buffer[i];            // catch the last closed curly
        number_of_protocols++;
      }        
    }  

    for (int i=0;i<number_of_protocols;i++) {
      if (json2[i] != '0') {
#ifdef DEBUGSIMPLE
        Serial.print("Incoming JSON as received by Teensy:   ");
        Serial.println(json2[i]);
#endif
      }
    }
    break;
  }

  Serial1.print("{\"device_id\": ");                                                          //Begin JSON file printed to bluetooth on Serial ports
  Serial.print("{\"device_id\": ");
  Serial1.print(device_id,2);
  Serial.print(device_id,2);
  Serial1.print(",\"firmware_version\":\"");
  Serial.print(",\"firmware_version\":\"");
  Serial1.print(FIRMWARE_VERSION);
  Serial.print(FIRMWARE_VERSION);
  Serial.print("\",\"sample\":[");
  Serial1.print("\",\"sample\":[");

  for (int y=0;y<measurements;y++) {                                                              // loop through the all measurements to create a measurement group

    Serial.print("[");                                                                        // print brackets to define single measurement
    Serial1.print("[");

    for (int q = 0;q<number_of_protocols;q++) {                                               // loop through all of the protocols to create a measurement

      free(json);                                                                             // free initial json malloc, make sure this is here! Free before resetting the size according to the serial input
      json = (char*)malloc((json2[q].length()+1)*sizeof(char));
      strncpy(json,json2[q].c_str(),json2[q].length());  
      json[json2[q].length()] = '\0';                                                         // Add closing character to char*
      hashTable = root.parseHashTable(json);
      if (!hashTable.success()) {                                                             // NOTE: if the incomign JSON is too long (>~5000 bytes) this tends to be where you see failure (no response from device)
        Serial.println(); 
        Serial1.println();
        Serial.println("{\"error\":\"JSON not recognized, or other failure with Json Parser\"}, the data JSON we received is: ");
        Serial1.println("{\"error\":\"JSON not recognized, or other failure with Json Parser\"}, the data JSON we received is: ");
        for (int i=0;i<5000;i++) {
          Serial.println(json2[i]);
          Serial1.println(json2[i]);
        }
        serial_bt_flush();
        return;
      }

      int protocols = 1;
      int quit = 0;
      for (int u = 0;u<protocols;u++) {                                                        // the number of times to repeat the current protocol
      
        String protocol_id =      hashTable.getString("protocol_id");                          // used to determine what macro to apply
        int analog_averages =     hashTable.getLong("analog_averages");                          // # of measurements per measurement pulse to be internally averaged (min 1 measurement per 6us pulselengthon) - LEAVE THIS AT 1 for now
        if (analog_averages == 0) {                                                              // if averages don't exist, set it to 1 automatically.
          analog_averages = 1;
        }
        averages =                hashTable.getLong("averages");                               // The number of times to average this protocol.  The spectroscopic and environmental data is averaged in the device and appears as a single measurement.
        if (averages == 0) {                                                                   // if averages don't exist, set it to 1 automatically.
          averages = 1;
        }
        int averages_delay =      hashTable.getLong("averages_delay");
        measurements =            hashTable.getLong("measurements");                            // number of times to repeat a measurement, which is a set of protocols
        measurements_delay =      hashTable.getLong("measurements_delay");                      // delay between measurements in seconds
        int protocols_delay =     hashTable.getLong("protocols_delay");                         // delay between protocols within a measurement
        protocols =               hashTable.getLong("protocols");                               // delay between protocols within a measurement
        if (protocols == 0) {                                                                   // if averages don't exist, set it to 1 automatically.
          protocols = 1;
        }
        if (hashTable.getLong("act_background_light") == 0) {                                    // The Teensy pin # to associate with the background actinic light.  This light continues to be turned on EVEN BETWEEN PROTOCOLS AND MEASUREMENTS.  It is always Teensy pin 13 by default.
          act_background_light =  13;                                                            // change to new background actinic light
        }
        else {
          act_background_light =  hashTable.getLong("act_background_light");
        }
        int act_background_light_intensity = hashTable.getLong("act_background_light_intensity");  // sets intensity of background actinic.  Choose this OR tcs_to_act.
        int tcs_to_act =          hashTable.getLong("tcs_to_act");                               // sets the % of response from the tcs light sensor to act as actinic during the run (values 1 - 100).  If tcs_to_act is not defined (ie == 0), then the act_background_light intensity is set to actintensity1.
        int pulsesize =           hashTable.getLong("pulsesize");                                // Size of the measuring pulse (5 - 100us).  This also acts as gain control setting - shorter pulse, small signal. Longer pulse, larger signal.  
        int pulsedistance =       hashTable.getLong("pulsedistance");                            // distance between measuring pulses in us.  Minimum 1000 us.
        int offset_off =          hashTable.getLong("offset_off");                               // turn off detector offsets (default == 0 which is on, set == 1 to turn offsets off)
        int get_offset =          hashTable.getLong("get_offset");                               // include detector offset information in the output
        // NOTE: it takes about 50us to set a DAC channel via I2C at 2.4Mz.  
        JsonArray get_ir_baseline=hashTable.getArray("get_ir_baseline");                        // include the ir_baseline information from the device for the specified pins
        JsonArray get_tcs_cal =   hashTable.getArray("get_tcs_cal");                            // include the get_tcs_cal information from the device for the specified pins
        JsonArray get_lights_cal= hashTable.getArray("get_lights_cal");                         // include get_lights_cal information from the device for the specified pins
        JsonArray get_blank_cal = hashTable.getArray("get_blank_cal");                          // include the get_blank_cal information from the device for the specified pins
        JsonArray get_other_cal = hashTable.getArray("get_other_cal");                        // include the get_other_cal information from the device for the specified pins
        JsonArray get_userdef0 =  hashTable.getArray("get_userghdef0");                        // include the saved userdef0 information from the device
        JsonArray get_userdef1 =  hashTable.getArray("get_userdef1");                        // include the saved userdef1 information from the device
        JsonArray get_userdef2 =  hashTable.getArray("get_userdef2");                        // include the saved userdef2 information from the device
        JsonArray get_userdef3 =  hashTable.getArray("get_userdef3");                        // include the saved userdef3 information from the device
        JsonArray get_userdef4 =  hashTable.getArray("get_userdef4");                        // include the saved userdef4 information from the device
        JsonArray get_userdef5 =  hashTable.getArray("get_userdef5");                        // include the saved userdef5 information from the device
        JsonArray get_userdef6 =  hashTable.getArray("get_userdef6");                        // include the saved userdef6 information from the device
        JsonArray pulses =        hashTable.getArray("pulses");                                // the number of measuring pulses, as an array.  For example [50,10,50] means 50 pulses, followed by 10 pulses, follwed by 50 pulses.
        JsonArray act1_lights =   hashTable.getArray("act1_lights");
        JsonArray act2_lights =   hashTable.getArray("act2_lights");
        JsonArray alt1_lights =   hashTable.getArray("alt1_lights");
        JsonArray alt2_lights =   hashTable.getArray("alt2_lights");
        act_intensities =         hashTable.getArray("act_intensities");                         // write to input register of a DAC. channel 0 for low (actinic).  1 step = +3.69uE (271 == 1000uE, 135 == 500uE, 27 == 100uE)
        meas_intensities =        hashTable.getArray("meas_intensities");                        // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
        cal_intensities =         hashTable.getArray("cal_intensities");                         // write to input register of a DAC. channel 2 calibrating light.  0 (low) - 4095 (high).
        JsonArray detectors =     hashTable.getArray("detectors");                               // the Teensy pin # of the detectors used during those pulses, as an array of array.  For example, if pulses = [5,2] and detectors = [[34,35],[34,35]] .  
        JsonArray meas_lights =   hashTable.getArray("meas_lights");
        JsonArray environmental = hashTable.getArray("environmental");
        JsonArray alert =         hashTable.getArray("alert");                                // sends the user a message which they must reply -1+ to continue
        JsonArray prompt =        hashTable.getArray("prompt");                                // sends the user a message to which they must reply <answer>+ to continue
        total_cycles =            pulses.getLength()-1;                                          // (start counting at 0!)

        long size_of_data_raw = 0;
        long total_pulses = 0;      
        for (int i=0;i<pulses.getLength();i++) {                                            // count the number of non zero lights and total pulses
          total_pulses += pulses.getLong(i) * meas_lights.getArray(i).getLength();          // count the total number of pulses
          int non_zero_lights = 0;
          for (int j=0;j<meas_lights.getArray(i).getLength();j++) {                         // count the total number of non zero pulses
            if (meas_lights.getArray(i).getLong(j) > 0) {
              non_zero_lights++;
            }
          }
          size_of_data_raw += pulses.getLong(i) * non_zero_lights;
        }
        free(data_raw_average);                                                            // free malloc of data_raw_average
        data_raw_average = (long*)calloc(size_of_data_raw,sizeof(long));                   // get some memory space for data_raw_average, initialize all at zero.

#ifdef DEBUGSIMPLE
        Serial.println();
        Serial.print("size of data raw:  ");
        Serial.println(size_of_data_raw);

        Serial.println();
        Serial.print("total number of pulses:  ");
        Serial.println(total_pulses);

        Serial.println();
        Serial.print("all data in data_raw_average:  ");
        for (int i=0;i<size_of_data_raw;i++) {
          Serial.print(data_raw_average[i]);
        }
        Serial.println();

        Serial.println();
        Serial.print("number of pulses:  ");
        Serial.println(pulses.getLength());

        Serial.println();
        Serial.print("arrays in meas_lights:  ");
        Serial.println(meas_lights.getLength());

        Serial.println();
        Serial.print("length of meas_lights arrays:  ");
        for (int i=0;i<meas_lights.getLength();i++) {
          Serial.print(meas_lights.getArray(i).getLength());
          Serial.print(", ");
        }
        Serial.println();
#endif
        Serial.print("{");
        Serial1.print("{");

        quit = user_enter_long(5);                                                        // check to see if user has quit (send -1 on USB or bluetooth serial)
        if (quit == -1) {
          Serial.print("}]");
          Serial1.print("}]");          
          goto skipall;
        }
        
        Serial.print("\"protocol_id\":\"");
        Serial1.print("\"protocol_id\":\"");
        Serial.print(protocol_id);
        Serial1.print(protocol_id);
        Serial.print("\",");
        Serial1.print("\",");

        if (get_offset == 1) {
          print_offset(1);
        }

        get_calibration(calibration_baseline_slope,calibration_baseline_yint,0,0,get_ir_baseline ,"get_ir_baseline");
        get_calibration(calibration_slope,calibration_yint,0,0,get_lights_cal ,"get_lights_cal");
        get_calibration(calibration_blank1,calibration_blank2,0,0,get_blank_cal,"get_blank_cal");
        get_calibration(calibration_other1,calibration_other2,0,0,get_other_cal,"get_other_cal");
        get_calibration(0,0,light_slope,light_y_intercept,get_tcs_cal,"get_tcs_cal");
        get_calibration(0,0,userdef0[0],userdef0[1],get_userdef0,"get_userdef0");
        get_calibration(0,0,userdef1[0],userdef1[1],get_userdef1,"get_userdef1");
        get_calibration(0,0,userdef2[0],userdef2[1],get_userdef2,"get_userdef2");
        get_calibration(0,0,userdef3[0],userdef3[1],get_userdef3,"get_userdef3");
        get_calibration(0,0,userdef4[0],userdef4[1],get_userdef4,"get_userdef4");
        get_calibration(0,0,userdef5[0],userdef5[1],get_userdef5,"get_userdef5");
        get_calibration(0,0,userdef6[0],userdef6[1],get_userdef6,"get_userdef6");

        if (averages > 1) {      
          Serial1.print("\"averages\":"); 
          Serial.print("\"averages\":"); 
          Serial1.print(averages);  
          Serial.print(averages); 
          Serial1.print(","); 
          Serial.print(","); 
        }

        //        print_sensor_calibration(1);                                               // print sensor calibration data

        // this should be an array, so I can reset it all......
        analog_read_average = 0;
        digital_read_average = 0;
        relative_humidity_average = 0;                                                                    // reset all of the environmental variables
        temperature_average = 0;
        objt_average = 0;
        co2_value_average = 0;
        lux_average = 0;
        r_average = 0;
        g_average = 0;
        b_average = 0;
        calculate_offset(pulsesize);                                                                    // calculate the offset, based on the pulsesize and the calibration values (ax+b)

#ifdef DEBUGSIMPLE
        Serial.println();
        Serial.print("\"offsets\": "); 
        Serial.print(offset_34);
        Serial.print(",");
        Serial.println(offset_35);
#endif
        for (int x=0;x<averages;x++) {                                                       // Repeat the protocol this many times  
          int background_on = 0;
          long data_count = 0;

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
                Serial1.print("\"relative_humidity\":");
                Serial.print("\"relative_humidity\":");
                switch (temp_error) {                                                           // contactless temperature response depends on if there were errors or not
                  case 0:
                    Serial1.print(relative_humidity_average);  
                    Serial1.print(",");
                    Serial.print(relative_humidity_average);  
                    Serial.print(",");
                    break;
                  case 1:
                    Serial.print("\"no temperature / relative humidity sensor found\",");
                    Serial1.print("\"no temperature / relative humidity sensor found\",");
                    break;
                }
              }
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "temperature") {
              Temperature((int) environmental.getArray(i).getLong(1));
              if (x == averages-1) {
                Serial1.print("\"temperature\":");
                Serial.print("\"temperature\":");
                switch (temp_error) {                                                           // contactless temperature response depends on if there were errors or not
                  case 0:
                    Serial1.print(temperature_average);  
                    Serial1.print(",");
                    Serial.print(temperature_average);  
                    Serial.print(",");     
                    break;
                  case 1:
                    Serial.print("\"no temperature / relative humidity sensor found\",");
                    Serial1.print("\"no temperature / relative humidity sensor found\",");
                    break;
                }    
              }  
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "contactless_temperature") {
              Contactless_Temperature( environmental.getArray(i).getLong(1));
              if (x == averages-1) {
                Serial1.print("\"contactless_temperature\":");
                Serial.print("\"contactless_temperature\":");
                switch (ctemp_error) {                                                           // contactless temperature response depends on if there were errors or not
                  case 0:
                    Serial1.print(objt_average);  
                    Serial1.print(",");
                    Serial.print(objt_average);  
                    Serial.print(",");
                    break;
                  case 1:
                    Serial.print("\"no contactless temperature sensor found\",");
                    Serial1.print("\"no contactless temperature sensor found\",");
                    break;
                }    
              }
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "co2") {
              Co2( environmental.getArray(i).getLong(1));
              if (x == averages-1) {                                                                // if it's the last measurement to average, then print the results
                Serial1.print("\"co2\":");
                Serial.print("\"co2\":");
                switch (co2_error) {                                             // co2 response depends on if there were errors or not
                  case 0:
                    Serial1.print(co2_value_average);  
                    Serial1.print(",");
                    Serial.print(co2_value_average);  
                    Serial.print(",");
                    break;
                  case 1:
                    Serial.print("\"no co2 sensor found\",");
                    Serial1.print("\"no co2 sensor found\",");
                    break;
                  case 2:
                    Serial.print("\"no response from co2 sensor\",");
                    Serial1.print("\"no response from co2 sensor\",");
                    break;
                }    
              }
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "light_intensity") {
              Light_Intensity(environmental.getArray(i).getLong(1));
              if (x == averages-1) {
                Serial1.print("\"light_intensity\":");
                Serial.print("\"light_intensity\":");
                Serial1.print(lux_to_uE(lux_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(lux_average));  
                Serial.print(",");                
                Serial1.print("\"r\":");
                Serial.print("\"r\":");
                Serial1.print(lux_to_uE(r_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(r_average));  
                Serial.print(",");  
                Serial1.print("\"g\":");
                Serial.print("\"g\":");
                Serial1.print(lux_to_uE(g_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(g_average));  
                Serial.print(",");  
                Serial1.print("\"b\":");
                Serial.print("\"b\":");
                Serial1.print(lux_to_uE(b_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(b_average));  
                Serial.print(",");  
              }
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "analog_read") {                      // perform analog reads
              int pin = environmental.getArray(i).getLong(2);
              pinMode(pin,INPUT);
              int analog_read = analogRead(pin);
              if (x == averages-1) {
                Serial1.print("\"analog_read\":");
                Serial.print("\"analog_read\":");
                Serial1.print(analog_read);  
                Serial1.print(",");
                Serial.print(analog_read);  
                Serial.print(",");                
              }
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "digital_read") {                      // perform digital reads
              int pin = environmental.getArray(i).getLong(2);
              pinMode(pin,INPUT);
              int digital_read = digitalRead(pin);
              if (x == averages-1) {
                Serial1.print("\"digital_read\":");
                Serial.print("\"digital_read\":");
                Serial1.print(digital_read);  
                Serial1.print(",");
                Serial.print(digital_read);  
                Serial.print(",");                
              }
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "digital_write") {                      // perform digital write
              int pin = environmental.getArray(i).getLong(2);
              int setting = environmental.getArray(i).getLong(3);
              pinMode(pin,OUTPUT);
              digitalWriteFast(pin,setting);
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "analog_write") {                      // perform analog write with length of time to apply the pwm
              int pin = environmental.getArray(i).getLong(2);
              int setting = environmental.getArray(i).getLong(3);
              int freq = environmental.getArray(i).getLong(4);
              int wait = environmental.getArray(i).getLong(5);
#ifdef DEBUGSIMPLE
              Serial.println(pin);
              Serial.println(pin);
              Serial.println(wait);
              Serial.println(setting);
              Serial.println(freq);              
#endif
              pinMode(pin,OUTPUT);
              analogWriteFrequency(pin, freq);                                                           // set analog frequency
              analogWrite(pin,setting);
              delay(wait);
              analogWrite(pin,0);
              reset_freq();                                                                              // reset analog frequencies
            }
            if (environmental.getArray(i).getLong(1) == 0 \
            && (String) environmental.getArray(i).getString(0) == "note") {                      // insert notes or other data.  Limit 999 chars, do not use following chars []+"'
              Serial1.print("\"note\":\"");
              Serial.print("\"note\":\"");
              String note = user_enter_str(3000000,0);                                            // wait for user to enter note
              Serial1.print(note);
              Serial1.print("\",");
              Serial.print(note);  
              Serial.print("\",");                
            }
          }

          analogReadAveraging(analog_averages);                                      // set analog averaging (ie ADC takes one signal per ~3u)

          int actfull = 0;
          int _tcs_to_act = 0;
          float _light_intensity = lux_to_uE(lux_average);
          _tcs_to_act = (uE_to_intensity(act_background_light,_light_intensity)*tcs_to_act)/100;
#ifdef DEBUGSIMPLE
          Serial.println();
          Serial.print("tcs to act: ");
          Serial.println(_tcs_to_act);
          Serial.print("ambient light in uE: ");
          Serial.println(lux_to_uE(lux_average));
          Serial.print("ue to intensity result:  ");
          Serial.println(uE_to_intensity(act_background_light,lux_to_uE(lux_average)));  // NOTE: wait turn flip DAC switch until later.
#endif
          for (int z=0;z<total_pulses;z++) {                                            // cycle through all of the pulses from all cycles
            int first_flag = 0;                                                           // flag to note the first pulse of a cycle
            int act1_on = 0;
            int act2_on = 0;
            int alt1_on = 0;
            int alt2_on = 0;
            if (cycle == 0 && pulse == 0) {                                             // if it's the beginning of a measurement, then...                                                             // wait a few milliseconds so that the actinic pulse presets can stabilize
              Serial.flush();                                                          // flush any remaining serial output info before moving forward          
              unsigned long starttimer0;
              if (act_background_light_prev != act_background_light) {                 // turn off the old actinic background light if it's not equal to the new one.  Otherwise, do nothing.
                digitalWriteFast(act_background_light_prev, LOW);
              }
              starttimer0 = micros();
              timer0.begin(pulse1,pulsedistance);                                       // Begin firsts pulse
              while (micros()-starttimer0 < pulsesize) {
              }                                                                         // wait a full pulse size, then...                                                                                          
              timer1.begin(pulse2,pulsedistance);                                       // Begin second pulse
            }      

#ifdef DEBUGSIMPLE
            Serial.println();
            Serial.print("cycle, measurement number, measurement array size,total pulses ");
            Serial.print(cycle); 
            Serial.print(", ");
            Serial.print(meas_number);
            Serial.print(", ");
            Serial.print(meas_array_size);
            Serial.print(",");
            Serial.println(total_pulses);
            Serial.print("measurement light, detector,data raw average,data point        ");
            Serial.print(_meas_light);
            Serial.print(", ");
            Serial.println(detector);
#endif      
            if (pulse == 0) {                                                                             // if it's the first pulse
              meas_array_size = meas_lights.getArray(cycle).getLength();                                  // get the number of measurement/detector subsets in the new cycle
              first_flag = 1;                                                                             // flip flag indicating that it's the 0th pulse and a new cycle
            }

            _meas_light = meas_lights.getArray(cycle).getLong(meas_number%meas_array_size);               // move to next measurement light
            detector = detectors.getArray(cycle).getLong(meas_number%meas_array_size);                    // move to next detector

            if (pulse < meas_array_size) {                                                                // if it's the first pulse of a cycle, then change act 1 and 2, alt1 and alt2 values as per array's set at beginning of the file
              if (pulse == 0) {

                String al = alert.getString(cycle);
                check_alert(al,cycle,alert,pulses);
                
                String pr = prompt.getString(cycle);
                if (pr != "0" && pr != "") {                                                            // if there's a prompt, wait for user to repond to prompt
                  Serial1.print("\"prompt_msg\":\"");
                  Serial.print("\"prompt_msg\":\"");
                  Serial1.print(prompt.getString(cycle));
                  Serial.print(prompt.getString(cycle));
                  Serial1.print("\",\"prompt_rsp\":\"");
                  Serial.print("\",\"prompt_rsp\":\"");
                  String response = user_enter_str(30000,0);                                                  // wait 50 minutes for user to respond
                  Serial.print(response);
                  Serial1.print(response);
                  Serial.print("\",");
                  Serial1.print("\",");
                }  
                _act1_light_prev = _act1_light;                                                           // save old actinic value as current value for act1,act2,alt1,and alt2
                _act1_light = act1_lights.getLong(cycle);
                act1_on = calculate_intensity(_act1_light,tcs_to_act,cycle,_light_intensity,_tcs_to_act); // calculate the intensities for each light and what light should be on or off.
                _act2_light_prev = _act2_light;
                _act2_light = act2_lights.getLong(cycle);
                act2_on = calculate_intensity(_act2_light,tcs_to_act,cycle,_light_intensity,_tcs_to_act);
                _alt1_light_prev = _alt1_light;
                _alt1_light = alt1_lights.getLong(cycle);
                alt1_on = calculate_intensity(_alt1_light,tcs_to_act,cycle,_light_intensity,_tcs_to_act);
                _alt2_light_prev = _alt2_light;
                _alt2_light = alt2_lights.getLong(cycle);
                alt2_on = calculate_intensity(_alt2_light,tcs_to_act,cycle,_light_intensity,_tcs_to_act);
              }
              calculate_intensity(_meas_light,tcs_to_act,cycle,_light_intensity,_tcs_to_act);          // in addition, calculate the intensity of the current measuring light

              switch (_meas_light) {                                                                    // set the DAC intensity for the measuring light only...
              case 15:
                dac.analogWrite(3,meas_intensity);
              case 16:
                dac.analogWrite(3,meas_intensity);
              case 11:
                dac.analogWrite(3,meas_intensity);
              case 12:
                dac.analogWrite(3,meas_intensity);
              case 20:
                dac.analogWrite(0,act_intensity); 
              case 2:
                dac.analogWrite(0,act_intensity);   
              case 10:
                dac.analogWrite(2,cal_intensity);        
              case 14:
                dac.analogWrite(2,cal_intensity);        
              }
              digitalWriteFast(DAC_ON, LOW);                                                       // and turn it on.
              delayMicroseconds(1);                                                     
              digitalWriteFast(DAC_ON, HIGH);                                              

              if (pulse == 0) {
                dac.analogWrite(0,act_intensity);                                                     // ...also write the new values to the DAC for actinic lights, but don't turn them on yet!     
                dac.analogWrite(3,meas_intensity);
                dac.analogWrite(2,cal_intensity);        
              }            

#ifdef DEBUGSIMPLE
              Serial.print("actinic, measurement, and calibration intensities           ");
              Serial.print(act_intensity);
              Serial.print(",");
              Serial.print(meas_intensity);
              Serial.print(",");
              Serial.println(cal_intensity);

              Serial.println("state of actinic lights                                   ");         
              Serial.print(act1_on);
              Serial.print(",");
              Serial.print(act2_on);
              Serial.print(",");
              Serial.print(alt1_on);
              Serial.print(",");
              Serial.println(alt2_on);
#endif
            }

            while (on == 0 | off == 0) {                                                	 // if ALL pulses happened, then...
            }
            data1 = analogRead(detector);                                                        // save the detector reading as data1    
            digitalWriteFast(SAMPLE_AND_HOLD, HIGH);						 // turn off sample and hold, and turn on lights for next pulse set

            if (first_flag == 1) {                                                                    // if this is the 0th pulse and a therefore new cycle
              digitalWriteFast(_act1_light_prev, LOW);                                                // turn off previous lights, turn on the new ones on (if light setting is zero, then no light on
              if (act1_on == 1) {                                                                      // only turn on if your supposed to!
                digitalWriteFast(_act1_light, HIGH);
              }
              digitalWriteFast(_act2_light_prev, LOW);
              if (act2_on == 1) {
                digitalWriteFast(_act2_light, HIGH);
              }
              digitalWriteFast(_alt1_light_prev, LOW);
              if (alt1_on == 1) {
                digitalWriteFast(_alt1_light, HIGH);
              }
              digitalWriteFast(_alt2_light_prev, LOW);
              if (alt2_on == 1) {
                digitalWriteFast(_alt2_light, HIGH);
              } 

              digitalWriteFast(DAC_ON, LOW);                                               // now turn on the DAC with the new values for the next cycle
              delayMicroseconds(1);                                                     
              digitalWriteFast(DAC_ON, HIGH);                                              
              first_flag = 0;                                                              // reset flag
            }

            float offset = 0;
            if (offset_off == 0) {
              switch (detector) {                                                          // apply offset to whicever detector is being used
              case 34:
                offset = offset_34;
                break;
              case 35:
                offset = offset_35;
                break;
              }
            }

#ifdef DEBUGSIMPLE
            Serial.print("data count, size of raw data                                   ");
            Serial.print(data_count);  
            Serial.print(",");
            Serial.println(size_of_data_raw);

#endif
            if (_meas_light  != 0) {                                                      // save the data, so long as the measurement light is not equal to zero.
              data_raw_average[data_count] += data1 - offset;  
              data_count++;
            }

            noInterrupts();                                                              // turn off interrupts because we're checking volatile variables set in the interrupts
            on = 0;                                                                      // reset pulse counters
            off = 0;  
            pulse++;                                                                     // progress the pulse counter and measurement number counter

#ifdef DEBUGSIMPLE
            Serial.print("data point average, current data                               ");
            Serial.print(data_raw_average[meas_number]);
            Serial.print("!");
            Serial.println(data1); 
#endif
            interrupts();                                                              // done with volatile variables, turn interrupts back on
            meas_number++;                                                              // progress measurement number counters

            if (pulse == pulses.getLong(cycle)*meas_lights.getArray(cycle).getLength()) { // if it's the last pulse of a cycle...
              pulse = 0;                                                               // reset pulse counter      
              cycle++;                                                                 // ...move to next cycle
            }
          }        
          background_on = 0;
          background_on = calculate_intensity_background(act_background_light,tcs_to_act,cycle,_light_intensity,_tcs_to_act,act_background_light_intensity);  // figure out background light intensity and state

          if (_act1_light != act_background_light) {                                  // turn off all lights unless they are the actinic background light  
            digitalWriteFast(_act1_light, LOW);
          }
          if (_act2_light != act_background_light) {
            digitalWriteFast(_act2_light, LOW);
          }
          if (_alt1_light != act_background_light) {
            digitalWriteFast(_alt1_light, LOW);
          }
          if (_alt2_light != act_background_light) {
            digitalWriteFast(_alt2_light, LOW);
          }

          if (background_on == 1) {
            digitalWriteFast(DAC_ON, LOW);        
            delayMicroseconds(1);
            digitalWriteFast(DAC_ON, HIGH);
            digitalWriteFast(act_background_light, HIGH);                                // turn on actinic background light in case it was off previously.
          }
          else {
            digitalWriteFast(act_background_light, LOW);                                // turn on actinic background light in case it was off previously.
          }     

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

            if (environmental.getArray(i).getLong(1) == 1 \                                    
            && (String) environmental.getArray(i).getString(0) == "relative_humidity") {
              Relative_Humidity((int) environmental.getArray(i).getLong(1));                        // if this string is in the JSON and the 2nd component in the array is == 0 (meaning they want this measurement taken prior to the spectroscopic measurement), then call the associated measurement (and so on for all if statements in this for loop)
              if (x == averages-1) {                                                                // if it's the last measurement to average, then print the results
                Serial1.print("\"relative_humidity\":");
                Serial.print("\"relative_humidity\":");
                switch (temp_error) {                                                           // contactless temperature response depends on if there were errors or not
                  case 0:
                    Serial1.print(relative_humidity_average);  
                    Serial1.print(",");
                    Serial.print(relative_humidity_average);  
                    Serial.print(",");
                    break;
                  case 1:
                    Serial.print("\"no temperature / relative humidity sensor found\",");
                    Serial1.print("\"no temperature / relative humidity sensor found\",");
                    break;
                }
              }
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "temperature") {
              Temperature((int) environmental.getArray(i).getLong(1));
              if (x == averages-1) {
                Serial1.print("\"temperature\":");
                Serial.print("\"temperature\":");
                switch (temp_error) {                                                           // contactless temperature response depends on if there were errors or not
                  case 0:
                    Serial1.print(temperature_average);  
                    Serial1.print(",");
                    Serial.print(temperature_average);  
                    Serial.print(",");     
                    break;
                  case 1:
                    Serial.print("\"no temperature / relative humidity sensor found\",");
                    Serial1.print("\"no temperature / relative humidity sensor found\",");
                    break;
                }    
              }  
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "contactless_temperature") {
              Contactless_Temperature( environmental.getArray(i).getLong(1));
              if (x == averages-1) {
                Serial1.print("\"contactless_temperature\":");
                Serial.print("\"contactless_temperature\":");
                switch (ctemp_error) {                                                           // contactless temperature response depends on if there were errors or not
                  case 0:
                    Serial1.print(objt_average);  
                    Serial1.print(",");
                    Serial.print(objt_average);  
                    Serial.print(",");
                    break;
                  case 1:
                    Serial.print("\"no contactless temperature sensor found\",");
                    Serial1.print("\"no contactless temperature sensor found\",");
                    break;
                }    
              }
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "co2") {
              Co2( environmental.getArray(i).getLong(1));
              if (x == averages-1) {                                                                // if it's the last measurement to average, then print the results
                Serial1.print("\"co2\":");
                Serial.print("\"co2\":");
                switch (co2_error) {                                             // co2 response depends on if there were errors or not
                  case 0:
                    Serial1.print(co2_value_average);  
                    Serial1.print(",");
                    Serial.print(co2_value_average);  
                    Serial.print(",");
                    break;
                  case 1:
                    Serial.print("\"no co2 sensor found\",");
                    Serial1.print("\"no co2 sensor found\",");
                    break;
                  case 2:
                    Serial.print("\"no response from co2 sensor\",");
                    Serial1.print("\"no response from co2 sensor\",");
                    break;
                }    
              }
            }
            if (environmental.getArray(i).getLong(1) == 1 \
          && (String) environmental.getArray(i).getString(0) == "light_intensity") {
              Light_Intensity(environmental.getArray(i).getLong(1));
              if (x == averages-1) {
                Serial1.print("\"light_intensity\":");
                Serial.print("\"light_intensity\":");
                Serial1.print(lux_to_uE(lux_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(lux_average));  
                Serial.print(",");                
                Serial1.print("\"r\":");
                Serial.print("\"r\":");
                Serial1.print(lux_to_uE(r_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(r_average));  
                Serial.print(",");  
                Serial1.print("\"g\":");
                Serial.print("\"g\":");
                Serial1.print(lux_to_uE(g_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(g_average));  
                Serial.print(",");  
                Serial1.print("\"b\":");
                Serial.print("\"b\":");
                Serial1.print(lux_to_uE(b_average));  
                Serial1.print(",");
                Serial.print(lux_to_uE(b_average));  
                Serial.print(",");  
              }
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "analog_read") {                      // perform analog reads
              int pin = environmental.getArray(i).getLong(2);
              pinMode(pin,INPUT);
              int analog_read = analogRead(pin);
              if (x == averages-1) {
                Serial1.print("\"analog_read\":");
                Serial.print("\"analog_read\":");
                Serial1.print(analog_read);  
                Serial1.print(",");
                Serial.print(analog_read);  
                Serial.print(",");                
              }
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "digital_read") {                      // perform digital reads
              int pin = environmental.getArray(i).getLong(2);
              pinMode(pin,INPUT);
              int digital_read = digitalRead(pin);
              if (x == averages-1) {
                Serial1.print("\"digital_read\":");
                Serial.print("\"digital_read\":");
                Serial1.print(digital_read);  
                Serial1.print(",");
                Serial.print(digital_read);  
                Serial.print(",");                
              }
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "digital_write") {                      // perform digital write
              int pin = environmental.getArray(i).getLong(2);
              int setting = environmental.getArray(i).getLong(3);
              pinMode(pin,OUTPUT);
              digitalWriteFast(pin,setting);
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "analog_write") {                      // perform analog write with length of time to apply the pwm
              int pin = environmental.getArray(i).getLong(2);
              int setting = environmental.getArray(i).getLong(3);
              int freq = environmental.getArray(i).getLong(4);
              int wait = environmental.getArray(i).getLong(5);
#ifdef DEBUGSIMPLE
              Serial.println(pin);
              Serial.println(pin);
              Serial.println(wait);
              Serial.println(setting);
              Serial.println(freq);              
#endif
              pinMode(pin,OUTPUT);
              analogWriteFrequency(pin, freq);                                                           // set analog frequency
              analogWrite(pin,setting);
              delay(wait);
              analogWrite(pin,0);
              reset_freq();                                                                              // reset analog frequencies
            }
            if (environmental.getArray(i).getLong(1) == 1 \
            && (String) environmental.getArray(i).getString(0) == "note") {                      // insert notes or other data.  Limit 999 chars, do not use following chars []+"'
              Serial1.print("\"note\":\"");
              Serial.print("\"note\":\"");
              String note = user_enter_str(3000000,0);                                            // wait for user to enter note
              Serial1.print(note);
              Serial1.print("\",");
              Serial.print(note);  
              Serial.print("\",");                
            }            
          }
          if (x+1 < averages) {                                                             //  to next average, unless it's the end of the very last run
            user_enter_long(averages_delay);
          }
        }
        Serial1.print("\"data_raw\":[");                                                    // print the averaged results
        Serial.print("\"data_raw\":[");
        for (int i=0;i<size_of_data_raw;i++) {                                                  // print data_raw, divided by the number of averages
          Serial.print(data_raw_average[i]/averages);
          Serial1.print(data_raw_average[i]/averages);
          if (i != size_of_data_raw-1) {
            Serial.print(",");
            Serial1.print(",");
          }
        }
        Serial1.println("]}");                                                              // close out the data_raw and protocol
        Serial.println("]}");

#ifdef DEBUGSIMPLE
        Serial.print("# of protocols repeats, current protocol repeat, number of total protocols, current protocol      ");
        Serial.print(protocols);
        Serial.print(",");
        Serial.print(u);
        Serial.print(",");
        Serial.print(number_of_protocols);
        Serial.print(",");
        Serial.println(q);
#endif      
        if (q < number_of_protocols-1 | u < protocols-1) {                               // if it's not the last protocol in the measurement and it's not the last repeat of the current protocol, add a comma
          Serial.print(",");
          Serial1.print(",");
          user_enter_long(protocols_delay*1000);
        }
        else if (q == number_of_protocols-1 && u == protocols-1) {                      // if it is the last protocol, then close out the data json
          Serial.print("]");
          Serial1.print("]");
        }      

        averages = 1;   			                                        // number of times to repeat the entire run 
        averages_delay = 0;                  	                                                // seconds wait time between averages
        analog_averages = 1;                                                             // # of measurements per pulse to be averaged (min 1 measurement per 6us pulselengthon)
        _act1_light = 0;
        _act2_light = 0;
        _alt1_light = 0;
        _alt2_light = 0;
        detector = 0;
        pulsesize = 0;                                                                // measured in microseconds
        pulsedistance = 0;
        act_intensity = 0;                                                            // intensity at LOW setting below
        meas_intensity = 0;                                                            // 255 is max intensity during pulses, 0 is minimum // for additional adjustment, change resistor values on the board
        cal_intensity = 0;
        relative_humidity_average = 0;                                                // reset all environmental variables to zero
        temperature_average = 0;
        objt_average = 0;
        co2_value_average = 0;
        lux_average = 0;
        r_average = 0;
        g_average = 0;
        b_average = 0;
        act_background_light_prev = act_background_light;                               // set current background as previous background for next protocol
#ifdef DEBUGSIMPLE
        Serial.println("previous light set to:   ");
        Serial.println(act_background_light_prev);
#endif    
      }
    }
    serial_bt_flush();

    if (y < measurements-1) {                                                      // add commas between measurements
      Serial.print(",");
      Serial1.print(",");
      user_enter_long(measurements_delay*1000);
    }
  }
  skipall:
  Serial.println("]}");
  Serial.println("");
  Serial1.println("]}");
  Serial1.println("");
  digitalWriteFast(act_background_light, LOW);                                    // turn off the actinic background light at the end of all measurements
  act_background_light = 13;                                                      // reset background light to teensy pin 13
  free(data_raw_average);                                                         // free the calloc() of data_raw_average
  free(json);                                                                     // free second json malloc
}

void pulse1() {		                                                        // interrupt service routine which turns the measuring light on
  digitalWriteFast(SAMPLE_AND_HOLD, LOW);		            		 // turn on measuring light and/or actinic lights etc., tick counter
  digitalWriteFast(_meas_light, HIGH);						// turn on measuring light
  on=1;
}

void pulse2() {    	                                                        // interrupt service routine which turns the measuring light off									// turn off measuring light, tick counter
  digitalWriteFast(_meas_light, LOW);
  off=1;
}

void lighttests_all() {

  // enter value to increment lights (suggested value is 40) followed by +
  // or press only +0 to exit

  double increment = user_enter_dbl(60000);

  Serial.print("{\"response\":\"");
  Serial1.print("{\"response\":\""); 

  if (increment == 0) {
    //back to main menu
    goto skipit;
  }
  Serial.print(increment);  
  Serial1.print(increment);  
  Serial.print(",");  
  Serial1.print(",");  
  Serial.read();
  Serial.read();

  digitalWriteFast(ACTINICLIGHT1,HIGH);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(0);
  Serial.println("actinic intensity 1 (switch LOW)");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(0,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }

  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("actinic intensity 1 (switch HIGH)");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(1,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(ACTINICLIGHT1,LOW);

  digitalWriteFast(ACTINICLIGHT2,HIGH);
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(0);
  Serial.println("actinic intensity 2 (switch LOW)");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(0,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("actinic intensity 2 (switch HIGH)");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(1,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(ACTINICLIGHT2,LOW);

  digitalWriteFast(CALIBRATINGLIGHT1,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(2);
  Serial.println("calibrating light 1");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(2,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(CALIBRATINGLIGHT1,LOW);
  digitalWriteFast(CALIBRATINGLIGHT2,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(2);
  Serial.println("calibrating light 2");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(2,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(CALIBRATINGLIGHT2,LOW);

  digitalWriteFast(MEASURINGLIGHT1,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("measuring light 1");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(3,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(MEASURINGLIGHT1,LOW);
  digitalWriteFast(MEASURINGLIGHT2,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("measuring light 2");
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(3,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(MEASURINGLIGHT2,LOW);

  digitalWriteFast(MEASURINGLIGHT3,HIGH);
  Serial.println();
  Serial.print("DAC number:   ");
  Serial.println(1);
  Serial.println("measuring light 3");
  for (int i=0;i<4095;i=i+increment) {
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
  for (int i=0;i<4095;i=i+increment) {
    dac.analogWrite(3,i); // write to input register of a DAC. Channel 0-3, Value 0-4095    
    Serial.print(i);
    Serial.print(",");
    delay(50);
  }
  digitalWriteFast(MEASURINGLIGHT4,LOW);
skipit:
  serial_bt_flush();
  Serial.println("\"}");
  Serial.println("");
  Serial1.println("\"}");
  Serial1.println("");
}

void lighttests(int _choose) {

  float sensor_value = 0;
  digitalWriteFast(DAC_ON, LOW);                                               // leave DAC on for light tests.

  dac.analogWrite(0,4095);                                                       // write to input register of a DAC. channel 0 for high (saturating).  0 (low) - 4095 (high).  1 step = +3.654uE  
  dac.analogWrite(1,4095);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, LOW);                         // preset the switch to the actinic (low) preset position
  dac.analogWrite(2,4095);                                                       // write to input register of a DAC. channel 2, calibrating light.  0 (low) - 4095 (high).  1 step = +3.654uE  
  dac.analogWrite(3,4095);                                                       // write to input register of a DAC. channel 3 measuring light.  0 (high) - 4095 (low).  2092 = 0.  From 2092 to zero, 1 step = +.2611uE
  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);

  serial_bt_flush();
  if (_choose == 98) {
    call_print_calibration(0);
    int tcs_to_act = 100;                                                          // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
    int _tcs_to_act = 0;                                                           // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
    double dac_setting = 0;
    while (1) {
      dac_setting = user_enter_dbl(60000);
      if (dac_setting == -1) {
        goto end;
      }
      serial_bt_flush();
      dac.analogWrite(0,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      if (dac_setting != 0) {
        digitalWriteFast(ACTINICLIGHT1, HIGH);
      }
      else {
        digitalWriteFast(ACTINICLIGHT1, LOW);
      }
    }
end:
    serial_bt_flush();
    digitalWriteFast(ACTINICLIGHT1, LOW);
  }

  if (_choose == 99) {
    call_print_calibration(0);
    float ambient_light = 0;
    int _light = user_enter_dbl(60000);                                          // enter the light [15,16,11,2,20]
    int tcs_to_act = 100;                                                          // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
    int _tcs_to_act = 0;                                                          // the actinic value is == to the ambient value (100 is ==, 200 is 2x, 50 is .5x, etc.)
    Serial.print("{\"act_to_tcs\":[");
    Serial1.print("{\"act_to_tcs\":[");
    dac.analogWrite(0,0);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
    dac.analogWrite(3,0);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
    while (Serial.available()<3 && Serial1.available()<3) {
      digitalWriteFast(_light, HIGH);
      sensor_value = lux_to_uE(Light_Intensity(1));
      _tcs_to_act = (uE_to_intensity(_light,sensor_value)*tcs_to_act)/100;
#ifdef DEBUGSIMPLE
      Serial.print(sensor_value);
      Serial1.print(sensor_value);
      Serial.print(",");
      Serial1.print(",");
      Serial.print(lux_to_uE(sensor_value));
      Serial1.print(lux_to_uE(sensor_value));
      Serial.print(",");
      Serial1.print(",");
      Serial.print(tcs_to_act);
      Serial1.print(tcs_to_act);
      Serial.println();
      Serial1.println();
#endif
      if (sensor_value == 0) {                                                              // if the ambient light is zero, then turn off the light completely
        digitalWriteFast(_light, LOW);
      }
      else {
        dac.analogWrite(0,_tcs_to_act);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
        dac.analogWrite(3,_tcs_to_act);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      }
      Serial.print(sensor_value);
      Serial1.print(sensor_value);
      Serial.print(",");
      Serial1.print(",");
      Serial.print(_tcs_to_act);
      Serial1.print(_tcs_to_act);
      Serial.println(",");
      Serial1.println(",");        
      delay(1000);
    }
    serial_bt_flush();
    digitalWriteFast(_light, LOW);      
  }
  serial_bt_flush(); 
  if (_choose == 100) {
    lighttests_all();
  }
  serial_bt_flush(); 
  if (_choose == 101) {
    Serial.print("{\"light_intensity\":[");
    Serial1.print("{\"light_intensity\":[");
    while (Serial.available()<3 && Serial1.available()<3) {
      sensor_value = lux_to_uE(Light_Intensity(1));
      Serial.print(sensor_value);
      Serial1.print(sensor_value);
      Serial.print(",");
      Serial1.print(",");
      delay(2000);
    }
    serial_bt_flush();
  }
  else if (_choose == 102) {                      // co2
    Serial.print("{\"Co2_value\":[");
    Serial1.print("{\"Co2_value\":[");
    while (Serial.available()<3 && Serial1.available()<3) {
      sensor_value = Co2(1);
      Serial.print(sensor_value);
      Serial.print(",");
      Serial1.print(sensor_value);
      Serial1.print(",");
      delay(2000);
    }
    serial_bt_flush();
  }
  else if (_choose == 103) {                      // temperature
    Serial.print("{\"temperature\":[");
    Serial1.print("{\"temperature\":[");
    while (Serial.available()<3 && Serial1.available()<3) {
      sensor_value = Temperature(1);
      Serial.print(sensor_value);
      Serial.print(",");
      Serial1.print(sensor_value);
      Serial1.print(",");
      delay(2000);
    }
    serial_bt_flush();
  }
  else if (_choose == 104) {                      //relative humidity
    Serial.print("{\"relative_humidity\":[");
    Serial1.print("{\"relative_humidity\":[");
    while (Serial.available()<3 && Serial1.available()<3) {
      sensor_value = Relative_Humidity(1);
      Serial.print(sensor_value);
      Serial.print(",");
      Serial1.print(sensor_value);
      Serial1.print(",");
      delay(2000);
    }
    serial_bt_flush();
  }
  else if (_choose == 105) {
    Serial.print("{\"light_intensity\":[");
    Serial1.print("{\"light_intensity\":[");
    while (Serial.available()<3 && Serial1.available()<3) {
      sensor_value = Light_Intensity(1);
      Serial.print(sensor_value);
      Serial1.print(sensor_value);
      Serial.print(",");
      Serial1.print(",");
      delay(2000);
    }
    serial_bt_flush();
  }
  else if (_choose == 106) {
    Serial1.print("{\"contactless_temperature\":[");
    Serial.print("{\"contactless_temperature\":[");
    while (Serial.available()<3 && Serial1.available()<3) {
      sensor_value = Contactless_Temperature(1);
      Serial.print(sensor_value);
      Serial1.print(sensor_value);
      Serial.print(",");
      Serial1.print(",");
      delay(200);
    }
    serial_bt_flush();
  }

  if (_choose<34 && _choose>0) {
    Serial1.print("{[");
    Serial.print("{[");
    int dac_setting = 0;
    while (1) {
      dac_setting = user_enter_dbl(60000);
      if (dac_setting == -1) {
        goto last;
      }
      serial_bt_flush();
      dac.analogWrite(0,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      dac.analogWrite(1,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      dac.analogWrite(2,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      dac.analogWrite(3,dac_setting);                                                       // write to input register of a DAC. channel 1, for low (actinic).  0 (low) - 4095 (high).  1 step = +3.69uE
      digitalWriteFast(_choose, HIGH);      
      if (dac_setting != 0) {
        digitalWriteFast(_choose, HIGH);      
      }
      else {
        digitalWriteFast(_choose, LOW);      
      }
    }
last:
    serial_bt_flush();
    digitalWriteFast(_choose, LOW);      
  }
  else if (_choose>33 && _choose <38) {
    int reading = 0;
    int pulsedistance = 0;
    while (1) {
      Serial.println();
      Serial1.println();
      pulsedistance = user_enter_dbl(60000);
      if (pulsedistance == -1) {
        goto final;
      }
      digitalWriteFast(SAMPLE_AND_HOLD, LOW);
      delayMicroseconds(pulsedistance);
      reading = analogRead(_choose);
      digitalWriteFast(SAMPLE_AND_HOLD, HIGH);
      Serial.print(reading);
      Serial1.print(reading);
      Serial.print(",");
      Serial1.print(",");
    }
final:
    serial_bt_flush();
  }
  digitalWriteFast(DAC_ON, HIGH);                                               // turn DAC back off
  Serial.print(sensor_value);
  Serial1.print(sensor_value);
  Serial.println("]}");
  Serial1.println("]}");
  Serial.println();
  Serial1.println();
}

float Relative_Humidity(int var1) {
  temp_error = 0;                                                                      // reset error flag
  if (var1 == 1 | var1 == 0) {
    float relative_humidity = htu.readHumidity();
    if (relative_humidity == 998) {                                                          // flag error if response is 998
      temp_error = 1;    
    }
#ifdef DEBUGSIMPLE
    Serial.print("\"relative_humidity\":");
    Serial.print(relative_humidity);  
    Serial.print(",");
#endif
    relative_humidity_average += relative_humidity / averages;
    return relative_humidity;
  }
}

float Temperature(int var1) {
  temp_error = 0;                                                                      // reset error flag
  if (var1 == 1 | var1 == 0) {
    float temperature = htu.readTemperature();
    if (temperature == 998) {                                                          // flag error if response is 998
      temp_error = 1;    
    }
#ifdef DEBUGSIMPLE
    Serial.print("\"temperature\":");
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
  #ifdef DEBUGSIMPLE
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
}

void print_sensor_calibration(int _open) {
  if (_open = 0) {
    Serial.print("{");
    Serial1.print("{");
  }    
  Serial.print("\"light_slope\":");
  Serial.print(light_slope);
  Serial.print(",");
  Serial.print("\"light_y_intercept\":");
  Serial.print(light_y_intercept);
  Serial1.print("\"light_slope\":");
  Serial1.print(light_slope);
  Serial1.print(",");
  Serial1.print("\"light_y_intercept\":");
  Serial1.print(light_y_intercept);
  if (_open = 1) {
    Serial.print(",");
    Serial1.print(",");
  }    
  else {
    Serial.println("}");
    Serial1.println("}");
  }
}

String user_enter_str (long timeout,int _pwr_off) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  char serial_buffer [1000] = {0};
  String serial_string;
  //  serial_bt_flush();
  long start1 = millis();
  long end1 = millis();
#ifdef DEBUG
  Serial.println();
  Serial.print(Serial.available());
  Serial.print(",");
  Serial.println(Serial1.available());
#endif  
  while (Serial.available() == 0 && Serial1.available() == 0) {
    if (_pwr_off == 1) {
      end1 = millis();
      if ((end1 - start1) > pwr_off_ms) {
        pwr_off();
        goto skip;
      }
    }
  }
skip:
  if (Serial.available()>0) {                                                          // if it's from USB, then read it.
    if (Serial.peek() != '[') {
      Serial.readBytesUntil('+',serial_buffer, 1000);
      serial_string = serial_buffer;
      //        serial_bt_flush();
    }
    else {
    }
  }
  else if (Serial1.available()>0) {                                                    // if it's from bluetooth, then read it instead.
    if (Serial1.peek() != '[') {
      Serial1.readBytesUntil('+',serial_buffer, 1000);
      serial_string = serial_buffer;
      //        serial_bt_flush();
    }
    else {
    }
  }
  return serial_string;
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

long user_enter_long(long timeout) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  long val;  
  char serial_buffer [32] = {
    0  };
  String serial_string;
  long start1 = millis();
  long end1 = millis();
  while (Serial.available() == 0 && Serial1.available() == 0) {
    end1 = millis();
    if ((end1 - start1) > timeout) {
      goto skip;
    }
  }                        // wait until some info comes in over USB or bluetooth...
skip:
  if (Serial1.available()>0) {                                                    // if it's from bluetooth, then read it instead.
    Serial1.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atol(serial_string.c_str());  
  }
  else if (Serial.available()>0) {                                                          // if it's from USB, then read it.
    Serial.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atol(serial_string.c_str());  
  }
  return val;
  serial_bt_flush();
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

double user_enter_dbl(long timeout) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  double val;
  char serial_buffer [32] = {
    0  };
  String serial_string;
  long start1 = millis();
  long end1 = millis();
  while (Serial.available() == 0 && Serial1.available() == 0) {
    end1 = millis();
    if ((end1 - start1) > timeout) {
      goto skip;
    }
  }                        // wait until some info comes in over USB or bluetooth...
skip:
  if (Serial.available()>0) {                                                          // if it's from USB, then read it.
    Serial.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atof(serial_string.c_str());  
  }
  else if (Serial1.available()>0) {                                                    // if it's from bluetooth, then read it instead.
    Serial1.readBytesUntil('+',serial_buffer, 32);
    serial_string = serial_buffer;
    val = atof(serial_string.c_str());  
  }
  return val;
  serial_bt_flush();
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

float lux_to_uE(float _lux_average) {                                                      // convert the raw signal value to uE, based on a calibration curve
  int uE = (_lux_average-light_y_intercept)/light_slope;           // units 13 outside
#ifdef DEBUGSIMPLE
  Serial.print(_lux_average);
  Serial.print(",");
  Serial.print(light_y_intercept);
  Serial.print(",");
  Serial.print(light_slope);
  Serial.print(",");
  Serial.println(uE);
#endif
  return uE;
}

int uE_to_intensity(int _pin, int _uE) {
  float _slope = 0;
  float _yint = 0;
  unsigned int _intensity = 0;
  for (int i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // loop through all_pins
    if (all_pins[i] == _pin) {                                                                        // when you find the pin your looking for
      _slope = calibration_slope[i];                                                                  // go get the calibration slope and yintercept
      _yint = calibration_yint[i];
      break;
    }
  }
  if (_slope != 0 | _yint != 0) {                                                                      // if calibration values exist then...
    _intensity = (_uE-_yint)/_slope;                                                                    // calculate the resulting intensity DAC value
  }
#ifdef DEBUGSIMPLE
  Serial.print("uE, slope, yint, act_background pin, DAC intensity:   ");
  Serial.print(_uE);
  Serial.print(",");
  Serial.print(_slope);
  Serial.print(",");
  Serial.print(_yint);
  Serial.print(",");
  Serial.println(_intensity);
#endif
  return _intensity;
}

//350 - 1150 - light calibrations
//1150 - 1550 - baseline
//1550 - 1750 - spad blanks
//1750 - 2150 - other additional user defined calibrations

void print_cal_userdef(String name, float array[],int last) {                                                    // little function to clean up printing calibration values
  Serial.print("\"");
  Serial.print(name);
  Serial.print("\":[");
  Serial1.print("\"");
  Serial1.print(name);
  Serial1.print("\":[");
  for (int i=0;i<sizeof(userdef1)/sizeof(float);i++) {                                                      // recall the calibration arrays
    Serial.print(array[i]);
    Serial1.print(array[i]);
    if (i != sizeof(userdef1)/sizeof(float)-1) {        
      Serial.print(",");    
      Serial1.print(",");    
    }
  }
  Serial.print("]");    
  Serial1.print("]");    
  if (last != 1) {                                                                                        // if it's not the last one, then add comma.  otherwise, add curly.
    Serial.println(",");    
    Serial1.println(",");    
  }
  else { 
    Serial.println("}");
    Serial.println("");
    Serial1.println("}");
    Serial1.println("");
  }
}

void print_cal(String name, float array[],int last) {                                                    // little function to clean up printing calibration values
  Serial.print("\"");
  Serial.print(name);
  Serial.print("\":[");
  Serial1.print("\"");
  Serial1.print(name);
  Serial1.print("\":[");
  for (int i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // recall the calibration arrays
    Serial.print(array[i]);
    Serial1.print(array[i]);
    if (i != sizeof(all_pins)/sizeof(int)-1) {        
      Serial.print(",");    
      Serial1.print(",");    
    }
  }
  Serial.print("]");    
  Serial1.print("]");    
  if (last != 1) {                                                                                        // if it's not the last one, then add comma.  otherwise, add curly.
    Serial.println(",");    
    Serial1.println(",");    
  }
  else { 
    Serial.println("}");
    Serial.println("");
    Serial1.println("}");
    Serial1.println("");
  }
}

void reset_all(int which) {
  if (which == 0) {
    int clean [1408] = {
    };
    EEPROM_writeAnything(0,clean);
  }
  else {
    int clean [1348] = {
    };
    EEPROM_writeAnything(60,clean);                                                      // only reset the arrays, leave other calibrations
  }  
}

void call_print_calibration (int _print) {

  EEPROM_readAnything(0,tmp006_cal_S);
  EEPROM_readAnything(4,light_slope);
  EEPROM_readAnything(8,light_y_intercept);
  EEPROM_readAnything(12,device_id);
  // location 16 - 20 is open!
  EEPROM_readAnything(20,manufacture_date);
  EEPROM_readAnything(24,slope_34);
  EEPROM_readAnything(28,yintercept_34);
  EEPROM_readAnything(32,slope_35);
  EEPROM_readAnything(36,yintercept_35);
  EEPROM_readAnything(40,userdef0);
  EEPROM_readAnything(60,calibration_slope);
  EEPROM_readAnything(180,calibration_yint);
  EEPROM_readAnything(300,calibration_slope_factory);
  EEPROM_readAnything(420,calibration_yint_factory);
  EEPROM_readAnything(540,calibration_baseline_slope);
  EEPROM_readAnything(660,calibration_baseline_yint);
  EEPROM_readAnything(880,calibration_blank1);
  EEPROM_readAnything(1000,calibration_blank2);
  EEPROM_readAnything(1120,calibration_other1);
  EEPROM_readAnything(1240,calibration_other2);
  EEPROM_readAnything(1360,userdef1);
  EEPROM_readAnything(1368,userdef2);
  EEPROM_readAnything(1376,userdef3);
  EEPROM_readAnything(1384,userdef4);
  EEPROM_readAnything(1392,userdef5);
  EEPROM_readAnything(1400,userdef6);

  if (_print == 1) {                                                                                      // if this should be printed to COM port --
    Serial.print("{");
    Serial1.print("{");
    print_offset(1);
    Serial.println();
    Serial1.println();
    print_sensor_calibration(1);
    Serial.println();
    Serial1.println();

    print_cal("all_pins", all_pins,0);
    print_cal("calibration_slope", calibration_slope,0);
    print_cal("calibration_yint", calibration_yint ,0);
    print_cal("calibration_slope_factory", calibration_slope_factory ,0);
    print_cal("calibration_yint_factory", calibration_yint_factory ,0);
    print_cal("calibration_baseline_slope", calibration_baseline_slope ,0);
    print_cal("calibration_baseline_yint", calibration_baseline_yint ,0);
    print_cal("calibration_blank1", calibration_blank1 ,0);
    print_cal("calibration_blank2", calibration_blank2 ,0);
    print_cal("calibration_other1", calibration_other1 ,0);
    print_cal("calibration_other2", calibration_other2 ,0);
    print_cal_userdef("userdef0", userdef0 ,0);
    print_cal_userdef("userdef1", userdef1 ,0);
    print_cal_userdef("userdef2", userdef2 ,0);
    print_cal_userdef("userdef3", userdef3 ,0);
    print_cal_userdef("userdef4", userdef4 ,0);
    print_cal_userdef("userdef5", userdef5 ,0);
    print_cal_userdef("userdef6", userdef6 ,1);
  }
}

void save_calibration_slope (int _pin,float _slope_val,int location) {
  for (int i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // loop through all_pins
    if (all_pins[i] == _pin) {                                                                        // when you find the pin your looking for
      EEPROM_writeAnything(location+i*4,_slope_val);
      //        save_eeprom_dbl(_slope_val,location+i*10);                                                                   // then in the same index location in the calibrations array, save the inputted value.
    }
  }
}

float save_calibration_yint (int _pin,float _yint_val,int location) {
  for (int i=0;i<sizeof(all_pins)/sizeof(int);i++) {                                                      // loop through all_pins
    if (all_pins[i] == _pin) {                                                                        // when you find the pin your looking for
      EEPROM_writeAnything(location+i*4,_yint_val);
      //        save_eeprom_dbl(_yint_val,location+i*10);                                                                   // then in the same index location in the calibrations array, save the inputted value.
    }
  }
}

void add_calibration (int location) {                                                                 // here you can save one of the calibration values.  This may be a regular calibration, or factory calibration (which saves both factory and regular values)
  call_print_calibration(1);                                                                            // call and print calibration info from eeprom
  int pin = 0;
  float slope_val = 0;
  float yint_val = 0;
  while (1) {
    pin = user_enter_dbl(60000);                                                                      // define the pin to add a calibration value to
    Serial.println(pin);
    Serial1.println(pin);
    if (pin == -1) {                                                                                    // if user enters -1, exit from this calibration
      goto final;
    }
    slope_val = user_enter_dbl(60000);                                                                  // now enter that calibration value
    Serial.println(slope_val,6);
    Serial1.println(slope_val,6);
    if (slope_val == -1) {
      goto final;
    }
    yint_val = user_enter_dbl(60000);                                                                  // now enter that calibration value
    Serial.println(yint_val,6);
    Serial1.println(yint_val,6);
    if (yint_val == -1) {
      goto final;
    }                                                                                            // THIS IS STUPID... not sure why but for some reason you have to split up the eeprom saves or they just won't work... at leats this works...
    save_calibration_slope(pin,slope_val,location);                                                      // save the value in the calibration string which corresponding pin index in all_pins
    save_calibration_yint(pin,yint_val,location+120);                                                      // save the value in the calibration string which corresponding pin index in all_pins
skipit:
    delay(1);
  }
final:
  serial_bt_flush();
  call_print_calibration(1);
}

void calculate_offset(int _pulsesize) {                                                                    // calculate the offset, based on the pulsesize and the calibration values offset = a*'pulsesize'+b
  offset_34 = slope_34*_pulsesize+yintercept_34;
  offset_35 = slope_35*_pulsesize+yintercept_35;
#ifdef DEBUGSIMPLE
  Serial.print("offset for detector 34, 35   ");
  Serial.print(offset_34,2);
  Serial.print(",");
  Serial.println(offset_35,2);
#endif
}

void print_offset(int _open) {
  if (_open == 0) {
    Serial.print("{");  
    Serial1.print("{");  
  }
  Serial.print("\"slope_34\":");
  Serial.print(slope_34);
  Serial.print(",");
  Serial.print("\"yintercept_34\":");
  Serial.print(yintercept_34);
  Serial.print(",");
  Serial.print("\"slope_35\":");
  Serial.print(slope_35);
  Serial.print(",");
  Serial.print("\"yintercept_35\":");
  Serial.print(yintercept_35);

  Serial1.print("\"slope_34\":");
  Serial1.print(slope_34);
  Serial1.print(",");
  Serial1.print("\"yintercept_34\":");
  Serial1.print(yintercept_34);
  Serial1.print(",");
  Serial1.print("\"slope_35\":");
  Serial1.print(slope_35);
  Serial1.print(",");
  Serial1.print("\"yintercept_35\":");
  Serial1.print(yintercept_35);
  if (_open == 1) {
    Serial.print(",");
    Serial1.print(",");
  }
  else {
    Serial.println("}");
    Serial1.println("}");
  }
}

void add_userdef(int location) {

  call_print_calibration(1);
  float userdef;
  for (int i=0;i<2;i++) {                        // loop twice for 2 values per userdef
    userdef = user_enter_dbl(60000);  
    Serial.print(userdef,6);
    Serial1.print(userdef,6);
    EEPROM_writeAnything(location+i*4,userdef);
  }  
  serial_bt_flush();  
  call_print_calibration(1);
}

void calibrate_offset() {

  call_print_calibration(1);

  slope_34 = user_enter_dbl(60000);  
  Serial.print(slope_34,2);
  Serial1.print(slope_34,2);
  EEPROM_writeAnything(24,slope_34);
  //  serial_bt_flush();

  yintercept_34 = user_enter_dbl(60000);  
  Serial.print(yintercept_34,2);
  Serial1.print(yintercept_34,2);
  EEPROM_writeAnything(28,yintercept_34);
  //  serial_bt_flush();

  slope_35 = user_enter_dbl(60000);  
  Serial.print(slope_35,2);
  Serial1.print(slope_35,2);
  EEPROM_writeAnything(32,slope_35);
  //  serial_bt_flush();

  yintercept_35 = user_enter_dbl(60000);  
  Serial.print(yintercept_35,2);
  Serial1.print(yintercept_35,2);
  EEPROM_writeAnything(36,yintercept_35);
  //  serial_bt_flush();

  serial_bt_flush();
  call_print_calibration(1);
}

void calibrate_light_sensor() {

  call_print_calibration(1);

  light_slope = user_enter_dbl(60000);  
  Serial.print(light_slope,6);
  Serial1.print(light_slope,6);
  EEPROM_writeAnything(4,light_slope);
  //    Serial.print("success");
  //    Serial1.print("success");

  light_y_intercept = user_enter_dbl(60000);  
  Serial.print(light_y_intercept,6);
  Serial1.print(light_y_intercept,6);
  EEPROM_writeAnything(8,light_y_intercept);
  //    Serial.print("success\"");
  //    Serial1.print("success\"");
  //    serial_bt_flush();
  Serial.println("},");                  // close out JSON
  Serial1.println("},");  

  serial_bt_flush();
  call_print_calibration(1);
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
    if (objt < -60 | objt > 1000) {                             // if the value is crazy probably indicating that there's no sensor installed
      objt_average = -99;
      objt = -99;
      Serial.print("\"error\":\"no contactless temp sensor found\"");
      Serial1.print("\"error\":\"no contactless temp sensor found\"");
      Serial.print(",");
      Serial1.print(",");
      return objt;
    }
    else {
      objt_average += objt / averages;
      return objt;
    }
  }
  /*
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
   EEPROM_writeAnything(0,tmp006_cal_S);
   //    save_eeprom(tmp006_cal_S,240);
   Serial.println("Finished calibration - new values saved!");
   }
   */
}

unsigned long Co2(int var1) {
  if (var1 == 1 | var1 == 0) {
    requestCo2(readCO2,2000);
    unsigned long co2_value = getCo2(response);
#ifdef DEBUGSIMPLE
    Serial1.print("\"co2_content\":");
    Serial.print("\"co2_content\":");
    Serial1.print(co2_value);  
    Serial1.print(",");
    Serial.print(co2_value);  
    Serial.print(",");
#endif
    delay(100);
    co2_value_average += co2_value / averages;
    return co2_value;
  }
}

void requestCo2(byte packet[],int timeout) {
  co2_error = 0;                                                    // reset the co2_error flag
  long start1 = millis();
  long end1 = millis();
  while(!Serial3.available()) {                                     //keep sending request until we start to get a response
    Serial3.write(readCO2,7);                                       // send request for co2 from module
    delay(50);
    end1 = millis();
    if (end1 - start1 > timeout) {
      co2_error = 1;                                                // send "no co2 sensor found" error
      break;
    }
  }
  switch (co2_error) {
  case 0:                                                             // if the co2 sensor is present, then...
    while(Serial3.available() < 7 )                                   //Wait to get a 7 byte response
    {
      timeout++;  
      if(timeout > 10) {                                             //if it takes to long there was probably an error, so flush serial and exit
#ifdef DEBUGSIMPLE
        Serial.println("I timed out!");
#endif
        while(Serial3.available()) {
          Serial3.read();
        }
        co2_error = 2;                                            // send "no response from co2 sensor" error
        break;                                                    //exit and try again
      }
      delay(50);
    }
    for (int i=0; i < 7; i++) {
      response[i] = Serial3.read();    
#ifdef DEBUGSIMPLE
      Serial.println("I got it!");
#endif
    }  
    break;
  case 1:
    break;
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

void set_device_info(int _set) {
  //  serial_bt_flush();
  Serial.print("{\"device_id\":");
  Serial.print(device_id,2);
  Serial.println(",");
  Serial.print("\"firmware_version\":");
  Serial.print(FIRMWARE_VERSION);
  Serial.println(",");
  Serial.print("\"manufacture_date\":");
  Serial.print(manufacture_date);
  Serial.println("}");
  Serial.println();

  Serial1.print("{\"device_id\":");
  Serial1.print(device_id,2);
  Serial1.println(",");
  Serial1.print("\"firmware_version\":");
  Serial1.print(FIRMWARE_VERSION);
  Serial1.println(",");
  Serial1.print("\"manufacture_date\":");
  Serial1.print(manufacture_date);
  Serial1.println("}");
  Serial1.println();

  if (_set == 1) {
    // please enter new device ID (integers only) followed by '+'
    device_id = user_enter_dbl(60000);
    Serial.println(device_id);  
    Serial1.println(device_id);  
    if (device_id == -1) {
      goto device_end;
    }
    EEPROM_writeAnything(12,device_id);
    // please enter new date of manufacture (yyyymm) followed by '+'   
    manufacture_date = user_enter_dbl(60000);
    Serial.println(manufacture_date);  
    Serial1.println(manufacture_date);  
    if (manufacture_date == -1) {
      goto device_end;
    }
    EEPROM_writeAnything(20,manufacture_date);
    set_device_info(0);
  }
device_end:
  delay(1);
  serial_bt_flush();
}

void serial_bt_flush() {
  while (Serial.available()>0) {                                                   // Flush incoming serial of the failed command
    Serial.read();
  }
  while (Serial1.available()>0) {                                                   // Flush incoming serial of the failed command
    Serial1.read();
  }
}

void configure_bluetooth () {

  // Bluetooth Programming Sketch for Arduino v1.2
  // By: Ryan Hunt <admin@nayr.net>
  // License: CC-BY-SA
  //
  // Standalone Bluetooth Programer for setting up inexpnecive bluetooth modules running linvor firmware.
  // This Sketch expects a bt device to be plugged in upon start. 
  // You can open Serial Monitor to watch the progress or wait for the LED to blink rapidly to signal programing is complete.
  // If programming fails it will enter command where you can try to do it manually through the Arduino Serial Monitor.
  // When programming is complete it will send a test message across the line, you can see the message by pairing and connecting
  // with a terminal application. (screen for linux/osx, hyperterm for windows)
  //
  // Hookup bt-RX to PIN 11, bt-TX to PIN 10, 5v and GND to your bluetooth module.
  //
  // Defaults are for OpenPilot Use, For more information visit: http://wiki.openpilot.org/display/Doc/Serial+Bluetooth+Telemetry

  Serial.print("{\"response\": \"");
  Serial1.print("{\"response\": \"");
  // Enter bluetooth device name as seen by other bluetooth devices (20 char max), followed by '+'.
  String name = user_enter_str(60000,0);  
  // Enter current bluetooth device baud rate, followed by '+' (if new jy-mcu,it's probably 9600, if it's already had firmware installed by 115200)
  long baud_now = user_enter_dbl(60000);  
  // PLEASE NOTE - the pairing key has been set automatically to '1234'.

  int pin =         1234;                    // Pairing Code for Module, 4 digits only.. (0000-9999)
  int led =         13;                      // Pin of Blinking LED, default should be fine.
  char* testMsg =   "PhotosynQ changing bluetooth name!!"; //
  int x;
  int wait =        1000;                    // How long to wait between commands (1s), dont change this.

  pinMode(led, OUTPUT);
  Serial.begin(115200);                      // Speed of Debug Console

  // Configuring bluetooth module for use with PhotosynQ, please wait.
  Serial1.begin(baud_now);                        // Speed of your bluetooth module, 9600 is default from factory.
  digitalWrite(led, HIGH);                 // Turn on LED to signal programming has started
  delay(wait);
  Serial1.print("AT");
  delay(wait);
  Serial1.print("AT+VERSION");
  delay(wait);
  Serial.print("Setting PIN : ");          // Set PIN
  Serial.println(pin);
  Serial1.print("AT+PIN"); 
  Serial1.print(pin); 
  delay(wait);
  Serial.print("Setting NAME: ");          // Set NAME
  Serial.println(name);
  Serial1.print("AT+NAME");
  Serial1.print(name); 
  delay(wait);
  Serial.println("Setting BAUD: 115200");   // Set baudrate to 115200
  Serial1.print("AT+BAUD8");                   
  delay(wait);

  if (verifyresults()) {                   // Check configuration
    Serial.println("Configuration verified");
  } 
  digitalWrite(led, LOW);                 // Turn off LED to show failure.
  Serial.println("\"}");                  // close out JSON
  Serial1.println("\"}");
  Serial.println();
  Serial1.println();
}

int verifyresults() {                      // This function grabs the response from the bt Module and compares it for validity.
  int makeSerialStringPosition;
  int inByte;
  char serialReadString[50];

  inByte = Serial1.read();
  makeSerialStringPosition=0;
  if (inByte > 0) {                                                // If we see data (inByte > 0)
    delay(100);                                                    // Allow serial data time to collect 
    while (makeSerialStringPosition < 38){                         // Stop reading once the string should be gathered (37 chars long)
      serialReadString[makeSerialStringPosition] = inByte;         // Save the data in a character array
      makeSerialStringPosition++;                                  // Increment position in array
      inByte = Serial1.read();                                          // Read next byte
    }
    serialReadString[38] = (char) 0;                               // Null the last character
    if(strncmp(serialReadString,bt_response,37) == 0) {               // Compare results
      return(1);                                                    // Results Match, return true..
    }
    Serial.print("VERIFICATION FAILED!!!, EXPECTED: ");           // Debug Messages
    Serial.println(bt_response);
    Serial.print("VERIFICATION FAILED!!!, RETURNED: ");           // Debug Messages
    Serial.println(serialReadString);
    return(0);                                                    // Results FAILED, return false..
  } 
  else {                                                                            // In case we haven't received anything back from module
    Serial.println("VERIFICATION FAILED!!!, No answer from the bt module ");        // Debug Messages
    Serial.println("Check your connections and/or baud rate");
    return(0);                                                                      // Results FAILED, return false..
  }
}

int calculate_intensity(int _light,int tcs_on,int _cycle,float _light_intensity,int _tcs) {
#ifdef DEBUGSIMPLE
  Serial.print("calculate intensity vars _light, tcs_on, _light_intensity, _tcs, cycle, act_intensities.getLong(_cycle), meas_intensities.getLong(_cycle), cal_intensities.getLong(_cycle)   ");
  Serial.print(",");
  Serial.print(_light);
  Serial.print(",");
  Serial.print(tcs_on);
  Serial.print(",");
  Serial.print(_cycle);
  Serial.print(",");
  Serial.print(_light_intensity);
  Serial.print(",");
  Serial.print(_tcs);
  Serial.print(",");
  Serial.print(act_intensities.getLong(_cycle));
  Serial.print(",");
  Serial.print(meas_intensities.getLong(_cycle));
  Serial.print(",");
  Serial.println(cal_intensities.getLong(_cycle));
#endif  

  int on = 0;                                                                            // so identify the places to turn the light on by flipping this to 1
  if (_light == 2 | _light == 20) {                                                      // if it's a saturating light, and...
    if (act_intensities.getLong(_cycle) > 0) {                                            // if the actinic intensity is greater than zero then...
      on = 1;
      act_intensity = act_intensities.getLong(_cycle);                                    // turn light on and set intensity equal to the intensity specified in the JSON
    }
    else if (act_intensities.getLong(_cycle) < 0 && tcs_on > 0 && _light_intensity > 0) {      // if the intensity is -1 AND tcs_to_act is on AND the uE value _tcs_to_act is > 0 (ie ambient light is >0)
      on = 1;
      act_intensity = _tcs;                                                       // then turn light on, and set intensity to ambient
    }
  }

  else if (_light == 15 | _light == 16 | _light == 11 | _light == 12) {     // if it's a measuring light, and...  
    if (meas_intensities.getLong(_cycle) > 0) {                                            // if the actinic intensity is greater than zero then...
      on = 1;
      meas_intensity = meas_intensities.getLong(_cycle);                                    // turn light on and set intensity equal to the intensity specified in the JSON
    }
    else if (meas_intensities.getLong(_cycle) < 0 && tcs_on > 0 && _light_intensity > 0) {      // if the intensity is -1 AND tcs_to_act is on AND the uE value _tcs_to_act is > 0 (ie ambient light is >0)
      on = 1;
      meas_intensity = _tcs;                                                       // then turn light on, and set intensity to ambient
    }
  }

  else if (_light == 14 | _light == 10) {                                             // if it's a calibrating light, and...  
    if (cal_intensities.getLong(_cycle) > 0) {                                            // if the actinic intensity is greater than zero then...
      on = 1;
      cal_intensity = cal_intensities.getLong(_cycle);                                    // turn light on and set intensity equal to the intensity specified in the JSON
    }
    else if (cal_intensities.getLong(_cycle) < 0 && tcs_on > 0 && _light_intensity > 0) {      // if the intensity is -1 AND tcs_to_act is on AND the uE value _tcs_to_act is > 0 (ie ambient light is >0)
      on = 1;
      cal_intensity = _tcs;                                                       // then turn light on, and set intensity to ambient
    }
  }
  return on;
}


int calculate_intensity_background(int _light,int tcs_on,int _cycle,float _light_intensity,int _tcs, int _background_intensity) {
#ifdef DEBUGSIMPLE
  Serial.println();
  Serial.print("calculate background intensity vars _light, tcs_on, _light_intensity, _tcs, cycle, act_intensities.getLong(_cycle), meas_intensities.getLong(_cycle), cal_intensities.getLong(_cycle)   ");
  Serial.print(",");
  Serial.print(_light);
  Serial.print(",");
  Serial.print(tcs_on);
  Serial.print(",");
  Serial.print(_cycle);
  Serial.print(",");
  Serial.print(_light_intensity);
  Serial.print(",");
  Serial.print(_tcs);
  Serial.print(",");
  Serial.print(act_intensities.getLong(_cycle));
  Serial.print(",");
  Serial.print(meas_intensities.getLong(_cycle));
  Serial.print(",");
  Serial.println(cal_intensities.getLong(_cycle));
#endif
  int on = 0;                                                                            // so identify the places to turn the light on by flipping this to 1

  if (_light == 2 | _light == 20) {                                                      // if it's a saturating light, and...
    if (_background_intensity > 0) {                                            // if actinic background intensity is preset then
      dac.analogWrite(0,_background_intensity);                                 // set the actinic to that value
      on = 1;                                                                            // so identify the places to turn the light on by flipping this to 1
    }
    else if (tcs_on > 0 && _light_intensity > 0) {                                       // or if tcs_to_act is on and ambient light is greater than zero then...
      dac.analogWrite(0,_tcs);                                                           // set the actinic to that value      
      on = 1;                                                                            // so identify the places to turn the light on by flipping this to 1
    }
  }
  else if (_light == 15 | _light == 16 | _light == 11 | _light == 12) {                       // if it's a measuring light, and...  
    if (_background_intensity > 0) {                                            // if actinic background intensity is preset then
      dac.analogWrite(3,_background_intensity);                                 // set the actinic to that value
      on = 1;                                                                            // so identify the places to turn the light on by flipping this to 1
    }
    else if (tcs_on > 0 && _light_intensity > 0) {                                       // or if tcs_to_act is on and ambient light is greater than zero then...
      dac.analogWrite(3,_tcs);                                                           // set the actinic to that value      
      on = 1;                                                                            // so identify the places to turn the light on by flipping this to 1
    }
  }
  else if (_light == 14 | _light == 10) {                                                     // if it's a calibrating light, and...  
    if (_background_intensity > 0) {                                            // if actinic background intensity is preset then
      dac.analogWrite(2,_background_intensity);                                 // set the actinic to that value
      on = 1;                                                                            // so identify the places to turn the light on by flipping this to 1
    }
    else if (tcs_on > 0 && _light_intensity > 0) {                                       // or if tcs_to_act is on and ambient light is greater than zero then...
      dac.analogWrite(2,_tcs);                                                           // set the actinic to that value      
      on = 1;                                                                            // so identify the places to turn the light on by flipping this to 1
    }
  }
  return on;
}

void reset_freq() {
  analogWriteFrequency(5, 187500);                                               // reset timer 0
  analogWriteFrequency(3, 187500);                                               // reset timer 1
  analogWriteFrequency(25, 488.28);                                              // reset timer 2
/*
Teensy 3.0              Ideal Freq:
16	0 - 65535	732 Hz	        366 Hz
15	0 - 32767	1464 Hz	        732 Hz
14	0 - 16383	2929 Hz	        1464 Hz
13	0 - 8191	5859 Hz	        2929 Hz
12	0 - 4095	11718 Hz	5859 Hz
11	0 - 2047	23437 Hz	11718 Hz
10	0 - 1023	46875 Hz	23437 Hz
9	0 - 511	        93750 Hz	46875 Hz
8	0 - 255	        187500 Hz	93750 Hz
7	0 - 127	        375000 Hz	187500 Hz
6	0 - 63	        750000 Hz	375000 Hz
5	0 - 31	        1500000 Hz	750000 Hz
4	0 - 15	        3000000 Hz	1500000 Hz
3	0 - 7	        6000000 Hz	3000000 Hz
2	0 - 3	        12000000 Hz	6000000 Hz

 */
}


/*
void start_timers(_pulsedistance) {                                                            // restart the timers
  Serial.flush();                                                          // flush any remaining serial output info before moving forward          
  unsigned long starttimer0;
  starttimer0 = micros();
  timer0.begin(pulse1,_pulsedistance);                                       // Begin firsts pulse
  while (micros()-starttimer0 < pulsesize) {
  }                                                                         // wait a full pulse size, then...                                                                                          
  timer1.begin(pulse2,_pulsedistance);                                       // Begin second pulse
}
*/
void pause_timers() {                                                            // pause timers
  timer0.end(); 
  timer1.end();
}

void check_alert(String _al,int _cycle,JsonArray _alert, JsonArray _pulses) {
  if (_al != "") {                                                                         // if there's an alert, create the alert object
    Serial1.print("\"alert\":[");
    Serial.print("\"alert\":[");
    if (_al != "0") {                                                             // if there's an alert, wait for user to respond to alert
//      pause_timers();
      Serial1.print(_alert.getString(_cycle));
      Serial.print(_alert.getString(_cycle));
      while (1) {
        int response = user_enter_long(30000);                                                  // wait 50 minutes for user to respond
        if (response == -1) {
          break;
        }
      }                  
      if (_cycle != _pulses.getLength()) {                                                      // if it's the last cycle in the protocols, then don't add the comma
        Serial1.print("\",");
        Serial.print("\",");                  
      }
    }
  }
}

