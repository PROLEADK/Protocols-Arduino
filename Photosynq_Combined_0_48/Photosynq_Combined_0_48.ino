// FIRMWARE VERSION OF THIS FILE (SAVED TO EEPROM ON FIRMWARE FLASH)
#define FIRMWARE_VERSION .48
#define DEVICE_NAME "MultispeQ"

/*
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
//#include <HTU21D.h>
#include <JsonParser.h>
#include <TCS3471.h>
#include <mcp4728.h>
//#include <detector-test2.h>
#include <i2c_t3.h>
#include <SoftwareSerial.h>
#include <EEPROMAnything.h>
#include <typeinfo>
#include <ADC.h>
#include <avr/sleep.h>
#include <LowPower_Teensy3.h>
#include "serial.h"
#include "flasher.h"
#include "crc32.h"
#include <SPI.h>    // include the new SPI library:

//#define ledPin 13

#if defined(__MK20DX128__)
#define FLASH_SIZE              0x20000
#define FLASH_SECTOR_SIZE       0x400
#define FLASH_ID                "fw_teensy3.0"
#elif defined(__MK20DX256__)
#define FLASH_SIZE              0x40000
#define FLASH_SECTOR_SIZE       0x800
#define FLASH_ID                "fw_teensy3.1"
#else
#error NOT SUPPORTED
#endif
// apparently better - thanks to Frank Boesing
#define RAMFUNC  __attribute__ ((section(".fastrun"), noinline, noclone, optimize("Os") ))

#define RESERVE_FLASH (2 * FLASH_SECTOR_SIZE)

//#include <stdio.h>
//#include "flasher.h"

#define S_PORT Serial

#define USE_SPI
#undef USE_LED
#define USE_DISCHARGE

#define GAIN_BITS 1.7        // extra effective bits due to gain being higher than beta detector - example, 4x more gain = 2.0 bits
#define DIV_BITS  1.6        // extra bits due to missing voltage divider

                             // Gain reduces headroom and anything more than 4x will clip with Greg's examples


#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

// Serial.println(analogValue, BIN);  // print as an ASCII-encoded binary

// set up the speed, mode and endianness of each device
// MODE0: SCLK idle low (CPOL=0), MOSI read on rising edge (CPHI=0)
SPISettings AD7689_settings (10000000, MSBFIRST, SPI_MODE0);

// Note: use CPHA = CPOL = 0
// Note: two dummy conversions are required on startup

static uint16_t ad7689_config = 0;

void upgrade_firmware(void);
void boot_check(void);




static int flash_hex_line(const char *line);
int parse_hex_line(const char *theline, uint8_t bytes[], unsigned int *addr, unsigned int *num, unsigned int *code);
static int flash_block(uint32_t address, uint32_t *bytes, int count);

//////////////////////ADC INFORMATION AND LOW POWER MODE////////////////////////
ADC *adc = new ADC();
TEENSY3_LP LP = TEENSY3_LP();

//////////////////////DEVICE ID FIRMWARE VERSION////////////////////////
float device_id = 0;
float manufacture_date = 0;

//////////////////////PIN DEFINITIONS AND TEENSY SETTINGS////////////////////////
//Serial, I2C, SPI...
#define RX       0
#define TX       1

#define MOSI1    11
#define MISO1    12

#define MOSI2    7
#define MISO2    8

#define SDA1     18
#define SCL1     19

#define SCL2     29
#define SDA2     30

#define SS1      22
#define SCK1     13
#define SS2      9

// hall effect sensor
#define HALL_OUT 35

//Lights
#define PULSE1   3
#define PULSE2   4
#define PULSE3   5
#define PULSE4   20
#define PULSE5   10
#define PULSE6   23
#define PULSE7   24
#define PULSE8   25
#define PULSE9   26
#define PULSE10  27

// bluetooth
#define BLERESET 14
#define DEBUG_DC 2
#define DEBUG_DD 36

// sample and hold (hold + release detector cap)
#define HOLDM    6
#define HOLDADD 21

// digital to analog converts (DAC)
// same code as beta... check
// ldac pin, pull high to program address, otherwise keep low
#define LDAC1    15
#define LDAC2    16
#define LDAC3    17

// peripheral USB 3.0 connector
#define DACT     40
#define ADCT     37
#define IOEXT1   31
#define IOEXT2   32

// battery management
// batt_me by default should be pulled high
// to check battery level, pull batt_me low and then measure batt_test
// check data sheet for ISET... pull high or low?
#define LSET     28
#define BATT_ME  33
#define BATT_TEST 34

/*NOTES*/// blank pin (used when no other pin is selected - probably should change this later
#define BLANK    32

// set internal analog reference
// any unused pins? ... organize by function on teensy please!  Make sure thisis updated to teensy 3.2 (right now file == teens 31.sch

/*
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
 
 
 //////////////////////PIN DEFINITIONS FOR CORALSPEQ////////////////////////
 #define SPEC_GAIN      28
 //#define SPEC_EOS       NA
 #define SPEC_ST        26
 #define SPEC_CLK       25
 #define SPEC_VIDEO     A10
 //#define LED530         15
 //#define LED2200k       16
 //#define LED470         20
 //#define LED2200K       2
 */

#define SPEC_CHANNELS    256
uint16_t spec_data[SPEC_CHANNELS];
unsigned long spec_data_average[SPEC_CHANNELS];            // saves the averages of each spec measurement
int idx = 0;
int spec_on = 0;                                           // flag to indicate that spec is being used during this measurement

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
float userdef7[2] = {
};
float userdef8[2] = {
};
float userdef9[2] = {
};
float userdef10[2] = {
};
float userdef11[2] = {
};
float userdef12[2] = {
};
float userdef13[2] = {
};
float userdef14[2] = {
};
float userdef15[2] = {
};
float userdef16[2] = {
};
float userdef17[2] = {
};
float userdef18[2] = {
};
float userdef19[2] = {
};
float userdef20[2] = {
};
float userdef21[2] = {
};
float userdef22[2] = {
};
float userdef23[2] = {
};
float userdef24[2] = {
};
float userdef25[2] = {
};
float userdef26[2] = {
};
float userdef27[2] = {
};
float userdef28[2] = {
};
float userdef29[2] = {
};
float userdef30[2] = {
};
float userdef31[2] = {
};
float userdef32[2] = {
};
float userdef33[2] = {
};
float userdef34[2] = {
};
float userdef35[2] = {
};
float userdef36[2] = {
};
float userdef37[2] = {
};
float userdef38[2] = {
};
float userdef39[2] = {
};
float userdef40[2] = {
};
float userdef41[2] = {
};
float userdef42[2] = {
};
float userdef43[2] = {
};
float userdef44[2] = {
};
float userdef45[2] = {
};
float userdef46[3] = {
};
float userdef47[3] = {
};
float userdef48[3] = {
};
float userdef49[3] = {
};
float userdef50[3] = {
};
float pwr_off_ms[2] = {
  120000,0};                                                // number of milliseconds before unit auto powers down.
//float pwr_off_ms = 120000;                                                // number of milliseconds before unit auto powers down.
int averages = 1;
int pwr_off_state = 0;
int pwr_off_lights_state = 0;
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
unsigned volatile short data1 = 0;
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

/*NOTES*/// GET RID OF ALL THESE AND REPLACE WITH NEW SENSORS

/*
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
 float valMultiplier = 1;
 int CO2calibration = 17;                                                 // manual CO2 calibration pin (CO2 sensor has auto-calibration, so manual calibration is only necessary when you don't want to wait for autocalibration to occur - see datasheet for details 
 
 ////////////////////TMP006 variables - address is 1000010 (adr0 on SDA, adr1 on GND)//////////////////////
 Adafruit_TMP006 tmp006(0x42);  // start with a diferent i2c address!  ADR1 is GND, ADR0 is SDA so address is set to 42
 float tmp006_cal_S = 6.4;
 
 */

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
float lux_average_forpar = 0;
float r_average_forpar = 0;
float g_average_forpar = 0;
float b_average_forpar = 0;
float lux_local = 0;
float r_local = 0;
float g_local = 0;
float b_local = 0;

float tryit [5];

// PUSH THIS AS LOCAL VARIABLE WHEN NEW DETECTOR IS FULLY IMPLEMENTED
int detector;                                                                 // detector used in the current pulse

//////////////////////TCS34725 variables/////////////////////////////////
//void i2cWrite(byte address, byte count, byte* buffer);
//void i2cRead(byte address, byte count, byte* buffer);
//TCS3471 TCS3471(i2cWrite, i2cRead);

//////////////////////DAC VARIABLES////////////////////////
mcp4728 dac1 = mcp4728(0); // instantiate mcp4728 object, Device ID = 0
//mcp4728 dac2 = mcp4728(1); // instantiate mcp4728 object, Device ID = 0
//mcp4728 dac3 = mcp4728(2); // instantiate mcp4728 object, Device ID = 0

void setup() {
  Serial.begin(115200);                                                         // set baud rate for Serial, used for communication to computer via USB
#ifdef DEBUGSIMPLE
  Serial_Print_Line("serial works");
#endif

  /*NOTES*/  // REINITIATE ONCE NEW BLUETOOTH IS CONNECTED

  //  Serial1.begin(115200);                                                        // set baud rate for Serial 1 used for bluetooth communication on pins 0,1
  //#ifdef DEBUGSIMPLE
  //  Serial_Print_Line("serial1 works");
  //#endif
  //  Serial3.begin(9600);                                                          // set baud rate for Serial 3 is used to communicate with the CO2 sensor
  //#ifdef DEBUGSIMPLE
  //  Serial_Print_Line("serial3 works");
  //#endif
  delay(500);

  Wire.begin(I2C_MASTER,0x00,I2C_PINS_18_19,I2C_PULLUP_INT,I2C_RATE_400);      // using alternative wire library
#ifdef DEBUGSIMPLE
  Serial_Print_Line("successfully opened I2C");
#endif
  dac1.vdd(2.0475); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout
  SPI.begin ();
  delay(10);

  /*NOTES*/  // REINITIATE ONCE NEW BLUETOOTH IS CONNECTED
  /*
  TCS3471.setWaitTime(200.0);
   TCS3471.setIntegrationTime(700.0);
   TCS3471.setGain(TCS3471_GAIN_1X);
   TCS3471.enable();
   */

  //  pinMode(29,OUTPUT);

  //  pinMode(3,INPUT);
  pinMode(PULSE1, OUTPUT);                                             // set appropriate pins to output
  pinMode(PULSE2, OUTPUT);
  pinMode(PULSE3, OUTPUT);
  pinMode(PULSE4, OUTPUT);
  pinMode(PULSE5, OUTPUT);
  pinMode(PULSE6, OUTPUT);
  pinMode(PULSE7, OUTPUT);
  pinMode(PULSE8, OUTPUT);
  pinMode(PULSE9, OUTPUT);
  pinMode(PULSE10, OUTPUT);
  //  pinMode(ACTINICLIGHT_INTENSITY_SWITCH, OUTPUT);
  //  digitalWriteFast(ACTINICLIGHT_INTENSITY_SWITCH, HIGH);                     // preset the switch to the actinic (high) preset position, DAC channel 0
  pinMode(HOLDM, OUTPUT);
  pinMode(HOLDADD, OUTPUT);
  // PINmODE(
  //  pinMode(PWR_OFF, OUTPUT);
  //  pinMode(PWR_OFF_LIGHTS, OUTPUT);
  //  pinMode(BATT_LEVEL, INPUT); 
  //  digitalWriteFast(PWR_OFF, LOW);                                               // pull high to power off, pull low to keep power.
  //  digitalWriteFast(PWR_OFF_LIGHTS, HIGH);                                               // pull high to power on, pull low to power down.
  pinMode(BLANK, OUTPUT);                                                            // used as a blank pin to pull high and low if the meas lights is otherwise blank (set to 0)
  //&&
  //  analogReadAveraging(1);                                                       // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u) - this gets changed later by each protocol
  //  pinMode(DETECTOR1, EXTERNAL);                                                 // use external reference for the detectors
  //  pinMode(DETECTOR2, EXTERNAL);
  //&&
  //  analogReadRes(ANALOGRESOLUTION);                                              // set at top of file, should be 16 bit
  //  analogresolutionvalue = pow(2,ANALOGRESOLUTION);                              // calculate the max analogread value of the resolution setting
  float default_resolution = 488.28;
  int timer0 [8] = {
    5, 6, 9, 10, 20, 21, 22, 23    };
  int timer1 [2] = {
    3, 4    };
  int timer2 [2] = {
    25, 32    }; 
  analogWriteFrequency(3, 187500);                                              // Pins 3 and 5 are each on timer 0 and 1, respectively.  This will automatically convert all other pwm pins to the same frequency.
  analogWriteFrequency(5, 187500);
  // power on all three DAC chips (pull high to enable)
  pinMode(LDAC1, OUTPUT);
  digitalWriteFast(LDAC1, HIGH);
  pinMode(LDAC2, OUTPUT);
  digitalWriteFast(LDAC2, HIGH); 
  pinMode(LDAC3, OUTPUT);
  digitalWriteFast(LDAC3, HIGH); 

  /*
// Set pinmodes for the coralspeq
   //pinMode(SPEC_EOS, INPUT);
   pinMode(SPEC_GAIN, OUTPUT);
   pinMode(SPEC_ST, OUTPUT);
   pinMode(SPEC_CLK, OUTPUT);
   
   digitalWrite(SPEC_GAIN, LOW);
   digitalWrite(SPEC_ST, HIGH);
   digitalWrite(SPEC_CLK, HIGH);
   digitalWrite(SPEC_GAIN, HIGH); //High Gain
   //digitalWrite(SPEC_GAIN, LOW); //LOW Gain
   */
  float tmp = 0;
  EEPROM_readAnything(16,tmp);
  if (tmp != (float) FIRMWARE_VERSION) {                                                // if the current firmware version isn't what's saved in EEPROM memory, then...
    EEPROM_writeAnything(16,(float) FIRMWARE_VERSION);                          // save the current firmware version
  }

  /*NOTES*/  // REINITIATE ONCE MAG AND ACCEL ARE CONNECTED
  //  MAG3110_init();                                                                // initialize compass and accelerometer
  //  MMA8653FC_init();

}


































//////////////////////// MAIN LOOP /////////////////////////
void loop() {

  delay(50);
  int measurements = 1;                                                   // the number of times to repeat the entire measurement (all protocols)
  unsigned long measurements_delay = 0;                                    // number of seconds to wait between measurements  
  unsigned long measurements_delay_ms = 0;                                    // number of milliseconds to wait between measurements  
  volatile unsigned long meas_number = 0;                                       // counter to cycle through measurement lights 1 - 4 during the run
  unsigned long end1;
  unsigned long start1 = millis(); 

  // these variables could be pulled from the JSON at the time of use... however, because pulling from JSON is slow, it's better to create a int to save them into at the beginning of a protocol run and use the int instead of the raw hashTable.getLong type call 
  int _act1_light;
  int _act2_light;
  int _act3_light;
  int _act4_light;
  int _act1_light_prev;
  int _act2_light_prev;
  int _act3_light_prev;
  int _act4_light_prev;
  int act_background_light_prev = BLANK;
  int spec;

  int cycle = 0;                                                                // current cycle number (start counting at 0!)
  int pulse = 0;                                                                // current pulse number
  int total_cycles;	                       	                        	// Total number of cycles - note first cycle is cycle 0
  int meas_array_size = 0;                                                      // measures the number of measurement lights in the current cycle (example: for meas_lights = [[15,15,16],[15],[16,16,20]], the meas_array_size's are [3,1,3].  
  char* json = (char*)malloc(1);
  char w;
  char* name;
  JsonHashTable hashTable;
  JsonParser<600> root;
  int number_of_protocols = 0;                                                      // reset number of protocols
  int end_flag = 0;
  int which_serial = 0;
  unsigned long* data_raw_average = (unsigned long*)malloc(4);
  char serial_buffer [serial_buffer_size];
  String json2 [max_jsons];
  memset(serial_buffer,0,serial_buffer_size);                                    // reset buffer to zero
  for (int i=0;i<max_jsons;i++) {
    json2[i] = "";                                                              // reset all json2 char's to zero (ie reset all protocols)
  }

  /*NOTES*/  // REINSTATE THIS ONCE WE HAVE NEW CALIBRATIONS
  //  call_print_calibration(0);                                                                  // recall all data saved in eeprom

  // discharge sample and hold in case the cap is currently charged (on add on and main board)
  digitalWriteFast(HOLDM,LOW);
  delay(10);
  digitalWriteFast(HOLDM,HIGH);
  digitalWriteFast(HOLDADD,LOW); 
  delay(10);
  digitalWriteFast(HOLDADD,HIGH);


      int _detector = 0;
      int _intensity = 0;
      int _meas_light = 0;
      int _act_intensity = 0;
      long _pulsedistance = 0;
      long _pulselength = 0;
      long _repeats = 0;


  String choose = "0";
  while (Serial.peek() != '[' && Serial1.peek() != '[') {                      // wait till we see a "[" to start the JSON of JSONS
    choose = user_enter_str(50,1);

    /*NOTES*/    // INTEGRATE THIS INTO THE NORMAL SWITCH CASE, OTHERWISE IT'S REALLY STUPID
    /*
    if (choose.toInt() > 0 && choose.toInt() < 200) {                    // if it's 0 - 200 then go see full light of device testing
     lighttests(choose.toInt());
     serial_bt_flush();                                                             // flush serial port if it's none of the above things
     }
     */

int theid = 0;
    //    if {
    switch (choose.toInt()) {
    case false:                                                                   // if it's not a number, then exit switch and check to see if it's a protocol
      break;
    case -1:
      delay(10);
      break;
    case 666:
      Serial.print("wake up");
      dac1.update();
      break;
    case 667:
      Serial.print("get status");
      Serial.print(dac1.getGain(0));
      break;

    case 1000:                                                                    // print "Ready" to USB and Bluetooth
      Serial_Print_Line(" Ready");
      break;
    case 1001:
      dac1.analogWrite(0,500);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      digitalWriteFast(PULSE1, HIGH);
      Serial.println("PULSE1");
      delay(1000);
      digitalWriteFast(PULSE1, LOW);
      dac1.analogWrite(0,0);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      delay(1000);
      break;
    case 1002:
      dac1.analogWrite(1,200);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      digitalWriteFast(PULSE2, HIGH);
      Serial.println("PULSE2");
      delay(1000);
      digitalWriteFast(PULSE2, LOW);
      dac1.analogWrite(1,0);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      delay(1000);
      break;
    case 1003: 
      dac1.analogWrite(2,4000);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      digitalWriteFast(PULSE3, HIGH);
      Serial.println("PULSE3");
      delay(1000);
      digitalWriteFast(PULSE3, LOW);
      dac1.analogWrite(2,0);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      delay(1000);
      break;
    case 1004:
      dac1.analogWrite(3,500);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      digitalWriteFast(PULSE4, HIGH);
      Serial.println("PULSE4");
      delay(1000);
      digitalWriteFast(PULSE4, LOW);
      dac1.analogWrite(3,0);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      dac1.analogWrite(3,0);
      digitalWriteFast(LDAC1, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC1, HIGH);
      delay(1000);
      break;
    case 1005:
      dac1.analogWrite(0,500);
      digitalWriteFast(LDAC2, LOW);
      delayMicroseconds(1);
      digitalWriteFast(LDAC2, HIGH);
      digitalWriteFast(PULSE5, HIGH);
      Serial.println("PULSE5");
      delay(1000);
      digitalWriteFast(PULSE5, LOW);
      break;
    case 1006:
      dac1.analogWrite(1,400);
      break;
    case 1007:
      dac1.analogWrite(1,400);
      break;
    case 1008:
      dac1.analogWrite(1,400);
      break;
    case 1009: 
      dac1.analogWrite(1,400);
      break;
    case 1010:
      dac1.analogWrite(1,400);
      break;
    case 1011:
      select_all();
      break;
    case 1012:
      Serial.print("flurescence trace (10ms between, 10us pulse, 1000 measuring intensity");
      fluorescence(1,1000,1000,10000,10,3);
      break;
    case 1013:
      Serial.print("flurescence trace (10ms between, 100us pulse, 1000 measuring intensity");
      fluorescence(1,1000,1000,10000,100,3);
      break;
    case 1014:
      Serial.print("flurescence trace (10ms between, 1000us pulse, 1000 measuring intensity");
      fluorescence(1,1000,1000,10000,1000,3);
      break;
    case 1015:
      Serial.print("measurement pulses using 950 (detector test) - not saturating light");
      fluorescence(1,200,0,10000,10,4);
      break;
    }
    
    //    }
  }  
}

void select_all() {
      int _detector = 0;
      int _intensity = 0;
      int _meas_light = 0;
      int _act_intensity = 0;
      long _pulsedistance = 0;
      long _pulselength = 0;
      long _repeats = 0;
  Serial.println("");
  Serial.println("enter detector (1 for main, 2 for reference)");
  _detector = user_enter_long(300000);
  Serial.println("enter measuring intensity 0 - 750");
  _intensity = user_enter_long(300000);
  Serial.println("enter measuring light PULSE1(605) = 3 PULSE2(940) = 4 PULSE3(535) = 5 PULSE4(650) = 20 PULSE5(430) = 10 PULSE6 = 23 PULSE7 = 24 PULSE8 = 25 PULSE9 = 26 PULSE10 = 27");
  _meas_light = user_enter_long(300000);
  Serial.println("enter measuring intensity 0 - 750");
  _act_intensity = user_enter_long(300000);
  Serial.println("enter distance between pulses in us");
  _pulsedistance = user_enter_long(300000);
  Serial.println("enter pulse length in us");
  int repeatit = 1;
  _pulselength = user_enter_long(300000);
  while (repeatit == 1) {
    fluorescence(_detector,_intensity,_act_intensity,_pulsedistance,_pulselength,_meas_light);        
    Serial.println("");
    Serial.println("repeat with same settings or return to main menu (1 to repeat, 0 to return)");
    repeatit = user_enter_long(300000);
  }
}





























String user_enter_str (long timeout,int _pwr_off) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  char serial_buffer [1000] = {
    0  };
  String serial_string;
  //  serial_bt_flush();
  long start1 = millis();
  long end1 = millis();
#ifdef DEBUG
  Serial_Print_Line("");
  Serial_Print(Serial.available());
  Serial_Print(",");
  Serial_Print_Line(Serial1.available());
#endif  
  while (Serial.available() == 0 && Serial1.available() == 0) {
    if (_pwr_off == 1) {
      end1 = millis();
      //      delay(100);
      //      Serial_Print_Line(end1 - start1);
      if ((end1 - start1) > pwr_off_ms[0]) {
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
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
  return serial_string;
}

long user_enter_long(long timeout) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  long val;  
  char serial_buffer [32] = {
    0    };
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
  while (Serial.available()>0) {                                                     // flush any remaining serial data before returning
    Serial.read();
  }
  while (Serial1.available()>0) {
    Serial1.read();
  }
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

double user_enter_dbl(long timeout) {
  Serial.setTimeout(timeout);
  Serial1.setTimeout(timeout);
  double val;
  char serial_buffer [32] = {
    0    };
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
  while (Serial.available()>0) {                                                     // flush any remaining serial data before returning
    Serial.read();
  }
  while (Serial1.available()>0) {
    Serial1.read();
  }
  Serial.setTimeout(1000);
  Serial1.setTimeout(1000);
}

void pwr_off() { 
  //  digitalWriteFast(PWR_OFF, HIGH);
  //  delay(50);
  //  digitalWriteFast(PWR_OFF, LOW);
  Serial_Print("{\"pwr_off\":\"does nothing!\"}");
  Serial_Print_CRC();
}



























uint16_t
AD7689_read(int chan)
{
  // set up
  AD7689_set (chan);

  // do conversion
  AD7689_sample();

  delayMicroseconds(3);

  // read conversion result
  return AD7689_read_sample();
}

// use set, sample and read_sample for control over timing

void
AD7689_set (int chan)
{
  // bit shifts needed for config register values
#define CFG 13
#define INCC 10
#define INx 7
#define BW  6
#define REF 3
#define SEQ 1
#define RB 0

  // select channel and other config
  ad7689_config = 0;

  ad7689_config |= 1 << CFG;    // update config on chip

  if (chan >= 8)
    ad7689_config |= 0B011 << INCC;  // temperature
  else {
    ad7689_config |= 0B111 << INCC;  // unipolar referenced to ground
    //ad7689_config |= 0B110 << INCC;  // unipolar referenced to com
    //ad7689_config |= 0B100 << INCC;  // unipolar diff pairs
    //ad7689_config |= 0B000 << INCC;  // bipolar diff pairs

    ad7689_config |= chan << INx;    // channel
  }

  ad7689_config |= 0 << BW;       // 1 adds more filtering
  ad7689_config |= 0B001 << REF;  // B000 = use internal 2.5V reference, B001 for 4.096V
  //ad7689_config |= 0B011 << REF;  // use external reference (maybe ~3.3V)
  ad7689_config |= 0 << SEQ;      // don't auto sequence
  ad7689_config |= 0 << RB;       // don't read back config value

  ad7689_config = ad7689_config << 2;   // convert 14 bits to 16 bits

  //Serial.printf("ADC config: %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n",BYTETOBINARY(config>>8), BYTETOBINARY(config));  // debug only

  pinMode (SS1, OUTPUT);      // set the Slave Select Pin as output

  SPI.beginTransaction (AD7689_settings);

  // send config (RAC mode)
  digitalWrite (SS1, LOW);
  SPI.transfer (ad7689_config >> 8);  // high byte
  SPI.transfer (ad7689_config & 0xFF);  // low byte, 2 bits ignored
  digitalWrite (SS1, HIGH);
  delayMicroseconds(6);

  // dummy
  digitalWrite (SS1, LOW);
  SPI.transfer (ad7689_config >> 8);  // high byte
  SPI.transfer (ad7689_config & 0xFF);  // low byte, 2 bits ignored
  digitalWrite (SS1, HIGH);
  delayMicroseconds(6);

  SPI.endTransaction ();

}

// start the conversion - very time accurate, takes 1.8 usec

inline void AD7689_sample() __attribute__((always_inline));
void
AD7689_sample()
{
  // do conversion
  digitalWriteFast(SS1, LOW);         // chip select
  digitalWriteFast(SS1, HIGH);        // chip deselect  (starts conversion)
}

uint16_t
AD7689_read_sample()
{
  delayMicroseconds(6);                      // wait till complete
  SPI.beginTransaction (AD7689_settings);
  digitalWrite (SS1, LOW);            // chip select
  uint16_t val = SPI.transfer (ad7689_config  >> 8) << 8;   // high byte
  val |= SPI.transfer (ad7689_config);      // low byte
  digitalWrite (SS1, HIGH);          // chip select
  delayMicroseconds(6);                     // wait for second conversion to complete
  SPI.endTransaction ();

  return val;
}

// chip has an internal temp sensor - might be useful

uint16_t
AD7689_read_temp()
{
  AD7689_set (99);   // set to read temp
  AD7689_sample();
  // note: automatically switches back to reading inputs after this
  return AD7689_read_sample();
}









void test_adc(int intensity, long pulsedistance, long pulselength, long repeats) {
 
//  Serial.println("ok turn off USB now");
//  delay(5000);
  
  dac1.analogWrite(0,intensity);  // PULSE 1 (orange)
  dac1.analogWrite(1,intensity);  // PULSE 2 (ir)
  dac1.analogWrite(2,intensity);  // PULSE 3 (green)
  dac1.analogWrite(3,900);        // PULSE 4 (red)
  dac1.analogWrite(4,intensity);  // PULSE 5 (blue)
  digitalWriteFast(LDAC1, LOW);
  delayMicroseconds(1);
  digitalWriteFast(LDAC1, HIGH);

  uint32_t x;
  uint32_t max = 0;
  uint32_t min = 65535;
  uint32_t  sum = 0;  // reset our accumulated sum of input values to zero
  uint32_t n = 0; // count of how many readings so far
  float mean = 0, delta = 0, m2 = 0, variance = 0, stdev = 0;
  static float stdev_min = 999; // to calculate standard deviation

  //uint32_t start_time = ARM_DWT_CYCCNT;
  uint32_t oldT = millis();   // record start time in milliseconds
  
  

  AD7689_set (0);        // set ADC channel to main detector
//  AD7689_set (1);        // set ADC channel to add-on detector

  int samples[repeats];              // save values, looking for trends/drft

  for (int i = 0; i < repeats + 1; i++) {

    int y = 0;
    
    noInterrupts();

    // always turn off serial monitor
    // subtracting off a baseline doesn't help much - problem is HF noise

    // a cycle just before then 1 usec improves bits by .2 to 10.2 bits
    //digitalWriteFast(SAMPLE_AND_HOLD, LOW);   // open switch, start integrating
    //digitalWriteFast(SAMPLE_AND_HOLD, HIGH);  // turn off integrating, close switch
    //delayMicroseconds(1);

//    int base = AD7689_read(0);             // read baseline value just before pulse

// digitalWriteFast(PULSE4, HIGH);

    digitalWriteFast(PULSE1, HIGH);
//    digitalWriteFast(PULSE2, HIGH);
//    digitalWriteFast(PULSE4, HIGH);
// This delay is important!  It addresses increasing signal due to actopulser heating up.  Tested and 12us is the minimum value required to avoid non-linear light intenisty changes due to the slow op amp in the actopulser circuit
    delayMicroseconds(12);
    digitalWriteFast(HOLDM, LOW);   // begin charge the measuring capacitor
//    digitalWriteFast(HOLDADD, LOW);   // begin charge the measuring capacitor
    delayMicroseconds(pulselength);            // pulse width
//    digitalWriteFast(PULSE1, LOW);
    digitalWriteFast(PULSE2, LOW);
    for (int j=0;j<30;j++) {
      AD7689_sample();                  // start conversion
      y = y + AD7689_read_sample();         // read value (could subtract off baseline)              
    }  
//    digitalWriteFast(PULSE4, LOW);
    digitalWriteFast(HOLDM, HIGH);   // discharge the measuring capacitor
//    digitalWriteFast(HOLDADD, HIGH);   // discharge the measuring capacitor
//    Serial.println(x);
 //   AD7689_read(0);

    // *********

// digitalWriteFast(PULSE4, LOW);

//x = (x + y + z)/ 3;
x = y / 10;

    if (i > 2) {            // skip first one
      sum += x;
      if (x > max) max = x;
      if (x < min) min = x;
      // from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
      n++;
      delta = x - mean;
      mean += delta / n;
      m2 += (delta * (x - mean));
    } // if

    samples[i] = x;              // save value
    
    delayMicroseconds(pulsedistance);    // measurement distance between pulses

  } // for
  
  
  
  
  
  
  
  
  
/*
  interrupts();
  Serial.print("Ok delay again");
  delay(5000);
  Serial.print("Ok delay again 3 more seconds");
  delay(3000);
*/
  Serial.print("all samples: ");
  for (int i=0;i<repeats;i++) {
    Serial.print(samples[i]);
    Serial.print(",");
  }
  Serial.println("");
  uint32_t delta_time = millis() - oldT;
  Serial.printf("%d samples, # samples/sec: %d, ms %d\n", n, (n * 1000) / delta_time, delta_time);
  variance = m2 / (n - 1); // (n-1):Sample Variance  (n): Population Variance
  stdev = sqrt(variance);  // Calculate standard deviation
  if (stdev < stdev_min)
    stdev_min = stdev;
  float average = (float)sum / n;
  Serial.print(" Avg: ");     Serial.println(average);
  Serial.print(" Volts: ");     Serial.println(average * 2.5 / 65536, 5);
  Serial.print(" Count: ");     Serial.println(n);
  Serial.print(" P-P noise: ");  Serial.println(max - min);
  Serial.print(" St.Dev: ");  Serial.println(stdev, 3);
  //Serial.print(" St.Dev best: ");  Serial.println(stdev_min, 3);
  Serial.print(" PPM: ");  Serial.println((stdev / average) * 1000000, 3);
  Serial.print(" PPM full scale: ");  Serial.println((stdev / 65536) * 1000000, 3);
  Serial.print(" bits (95%) = "); Serial.println(15 - log(stdev * 2) / log(2.0)); // 2 std dev from mean = 95%
  Serial.print(" bits (95%) + Gain = "); Serial.println(GAIN_BITS + DIV_BITS + 15 - log(stdev * 2) / log(2.0)); // 2 std dev from mean = 95%
  Serial.println("");

  delay(2000);
}

















void fluorescence(int detector, int intensity, int act_intensity, long pulsedistance, long pulselength, int meas_light) {
 
  Serial.println("ok turn off USB now");
//  delay(5000);
  
  dac1.analogWrite(0,intensity); // PULSE 1 (orange)
  dac1.analogWrite(1,intensity);  // PULSE 2 (ir)
  dac1.analogWrite(2,intensity);  // PULSE 3 (green)
  dac1.analogWrite(3, act_intensity); // PULSE 4 (red)
//  dac1.analogWrite(4,intensity);  // PULSE 5 (blue)
  digitalWriteFast(LDAC1, LOW);
  delayMicroseconds(1);
  digitalWriteFast(LDAC1, HIGH);

  int fs = 0;
  int fm = 0;
  uint32_t x;
  uint32_t max = 0;
  uint32_t min = 65535;
  uint32_t  sum = 0;  // reset our accumulated sum of input values to zero
  uint32_t  fssum = 0;  // reset our accumulated sum of input values to zero
  uint32_t  fmsum = 0;  // reset our accumulated sum of input values to zero
  uint32_t n = 0; // count of how many readings so far
  float mean = 0, delta = 0, m2 = 0, variance = 0, stdev = 0;
  float fsmean = 0, fsdelta = 0, fsm2 = 0, fsvariance = 0, fsstdev = 0;
  float fmmean = 0, fmdelta = 0, fmm2 = 0, fmvariance = 0, fmstdev = 0;

  static float stdev_min = 999; // to calculate standard deviation
  static float fsstdev_min = 999; // to calculate standard deviation
  static float fmstdev_min = 999; // to calculate standard deviation

  //uint32_t start_time = ARM_DWT_CYCCNT;
  uint32_t oldT = millis();   // record start time in milliseconds
  
  
  if (detector == 1) {
    AD7689_set (0);        // set ADC channel to main detector
  }
  else {
    AD7689_set (1);        // set ADC channel to main detector
  }
  
  int samples[300];              // save values, looking for trends/drft

  for (int i = 0; i < 300 + 1; i++) {

     if (i == 100) {
      digitalWriteFast(PULSE4, HIGH);
    }
   
    
    int y = 0;
    
    noInterrupts();

    // always turn off serial monitor
    // subtracting off a baseline doesn't help much - problem is HF noise

    // a cycle just before then 1 usec improves bits by .2 to 10.2 bits
    //digitalWriteFast(SAMPLE_AND_HOLD, LOW);   // open switch, start integrating
    //digitalWriteFast(SAMPLE_AND_HOLD, HIGH);  // turn off integrating, close switch
    //delayMicroseconds(1);

//    int base = AD7689_read(0);             // read baseline value just before pulse

// digitalWriteFast(PULSE4, HIGH);

  switch (meas_light) {
    case PULSE1:
      digitalWriteFast(PULSE1, HIGH);
      break;
    case PULSE2:
      digitalWriteFast(PULSE2, HIGH);
      break;
    case PULSE3:
      digitalWriteFast(PULSE3, HIGH);
      break;
    case PULSE4:
      digitalWriteFast(PULSE4, HIGH);
      break;
    case PULSE5:    
      digitalWriteFast(PULSE5, HIGH);
      break;
    }
// This delay is important!  It addresses increasing signal due to actopulser heating up.  Tested and 12us is the minimum value required to avoid non-linear light intenisty changes due to the slow op amp in the actopulser circuit.  At very low light intensities (dac = 5), this can be as long as 30us
//    delayMicroseconds(12);
    delayMicroseconds(4);
    digitalWriteFast(HOLDM, LOW);   // begin charge the measuring capacitor
    delayMicroseconds(pulselength);            // pulse width

  switch (meas_light) {
    case PULSE1:
      digitalWriteFast(PULSE1, LOW);
      break;
    case PULSE2:
      digitalWriteFast(PULSE2, LOW);
      break;
    case PULSE3:
      digitalWriteFast(PULSE3, LOW);
      break;
    case PULSE4:
      digitalWriteFast(PULSE4, LOW);
      break;
    case PULSE5:    
      digitalWriteFast(PULSE5, LOW);
      break;
    }    
    
  for (int j=0;j<30;j++) {
      AD7689_sample();                  // start conversion
      y = y + AD7689_read_sample();         // read value (could subtract off baseline)              
    }  
    
  if (detector == 1) {    digitalWriteFast(HOLDM, HIGH);   // begin charge the measuring capacitor
  
x = y / 30;

    if (i > 10 && i < 30) {            // skip first one
      sum += x;
      if (x > max) max = x;
      if (x < min) min = x;
      // from http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
      n++;
      delta = x - mean;
      mean += delta / n;
      m2 += (delta * (x - mean));
    } // if

/*
    if (i>10 && i<30) {
      fssum += x;
      if (x > max) max = x;
      if (x < min) min = x;
      fsdelta = x - fsmean;
      fsmean += fsdelta / n;
      fsm2 += (fsdelta * (x - fsmean));
    } // if


     if(i>140 && i<160) {
      fmsum += x;
      if (x > max) max = x;
      if (x < min) min = x;
      fmdelta = x - fmmean;
      fmmean += fmdelta / n;
      fmm2 += (fmdelta * (x - fmmean));
    } // if

*/

    samples[i] = x;              // save value

    if (i == 200) {
      digitalWriteFast(PULSE4, LOW);
    }

        
    delayMicroseconds(pulsedistance);    // measurement distance between pulses

  } // for

  
/*
  interrupts();
  Serial.print("Ok delay again");
  delay(5000);
  Serial.print("Ok delay again 3 more seconds");
  delay(3000);
*/

  Serial.println("");
  uint32_t delta_time = millis() - oldT;
//  Serial.printf("%d samples, # samples/sec: %d, ms %d\n", n, (n * 1000) / delta_time, delta_time);
  variance = m2 / (n - 1); // (n-1):Sample Variance  (n): Population Variance
  stdev = sqrt(variance);  // Calculate standard deviation
  if (stdev < stdev_min)
    stdev_min = stdev;
  float average = (float)sum / n;

/*
  fsvariance = fsm2 / (n - 1); // (n-1):Sample Variance  (n): Population Variance
  fsstdev = sqrt(fsvariance);  // Calculate standard deviation
  if (fsstdev < fsstdev_min)
    fsstdev_min = fsstdev;
  float fsaverage = (float)fssum / n;

  fmvariance = fmm2 / (n - 1); // (n-1):Sample Variance  (n): Population Variance
  fmstdev = sqrt(fmvariance);  // Calculate standard deviation
  if (fmstdev < fmstdev_min)
    fmstdev_min = fmstdev;
  float fmaverage = (float)fmsum / n;
*/



  Serial.print("{\"sample\":[[{");
  Serial.print(" \"Avg\": ");     Serial.print(average);    Serial.print(",");
  Serial.print(" \"STD TO SIGNAL\" : "); 
  Serial.print(100*stdev/average,4);    Serial.print(",");  // 2 std dev from mean = 95% 
/*
  Serial.print(" \"fs\": ");     Serial.print(fsaverage);    Serial.print(",");
  Serial.print(" \"FS STD TO SIGNAL\" : "); 
  Serial.print(100*fsstdev/fsaverage,2);    Serial.print(",");  // 2 std dev from mean = 95% 
  Serial.print(" \"fm\": ");     Serial.print(fmaverage);    Serial.print(",");
  Serial.print(" \"FM STD TO SIGNAL\" : "); 
  Serial.print(100*fmstdev/fmaverage,2);    Serial.print(",");  // 2 std dev from mean = 95% 
*/
  Serial.print(" \"bits (95%)\" : "); Serial.print(15 - log(stdev * 2) / log(2.0));   Serial.print(",");  // 2 std dev from mean = 95% 
  Serial.print(" \"STD TO SIGNAL\" : "); 
  Serial.print(100*stdev/average,2);    Serial.print(",");  // 2 std dev from mean = 95% 
  Serial.print(" \"St.Dev\": ");  Serial.print(stdev, 3);    Serial.print(",");
  Serial.print(" \"PPM\": ");  Serial.print((stdev / average) * 1000000, 3);    Serial.print(",");
  Serial.print(" \"Volts\": ");     Serial.print(average * 2.5 / 65536, 5);    Serial.print(",");
  Serial.print(" \"Count\": ");     Serial.print(n);    Serial.print(",");
  Serial.print(" \"P-P noise\": ");  Serial.print(max - min);    Serial.print(",");
  //Serial.print(" St.Dev best: ");  Serial.print(stdev_min, 3);
  Serial.print(" \"PPM full scale\": ");  Serial.print((stdev / 65536) * 1000000, 3);    Serial.print(",");
  Serial.print(" \"bits (95%) + Gain\" : "); Serial.print(GAIN_BITS + DIV_BITS + 15 - log(stdev * 2) / log(2.0));    Serial.print(",");// 2 std dev from mean = 95% 
  Serial.print("\"data_raw\": [");
  for (int i=0;i<300;i++) {
    Serial.print(samples[i]);
    if (i != 299) {
    Serial.print(",");
    }
  }
    Serial.print("]}]]}");
  delay(2000);
}

// 1012+1000+10000+10+
