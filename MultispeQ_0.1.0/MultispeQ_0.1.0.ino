// FIRMWARE VERSION OF THIS FILE (SAVED TO EEPROM ON FIRMWARE FLASH)
#define FIRMWARE_VERSION .46
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
#include <HTU21D.h>
#include <JsonParser.h>
#include <TCS3471.h>
#include <mcp4728.h>
#include <i2c_t3.h>
#include <SoftwareSerial.h>
#include <EEPROMAnything.h>
//#include <typeinfo>
#include <ADC.h>
#include <avr/sleep.h>
#include <LowPower_Teensy3.h>
#include "serial.h"
#include "flasher.h"
#include "crc32.h"


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

//undefined as of yet!
#define DEBUG_DC 2
#define HOLDM    6
#define SS2      9
#define SCK1     13
#define UNUSED1  14
#define BLERESET 15
#define LDAC2    16
#define LDAC3    17
#define HOLD_ADD 21
#define SS1      22
#define LSET     28
#define IOEXT1   31
#define IOEXT2   32
#define UNUSED2  33



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
float pwr_off_ms[2] = {120000,0};                                                // number of milliseconds before unit auto powers down.
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

/*NOTES*/ // GET RID OF ALL THESE AND REPLACE WITH NEW SENSORS

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
mcp4728 dac = mcp4728(0); // instantiate mcp4728 object, Device ID = 0

void setup() {
  Serial.begin(115200);                                                         // set baud rate for Serial, used for communication to computer via USB
#ifdef DEBUGSIMPLE
  Serial_Print_Line("serial works");
#endif

/*NOTES*/ // REINITIATE ONCE NEW BLUETOOTH IS CONNECTED

//  Serial1.begin(115200);                                                        // set baud rate for Serial 1 used for bluetooth communication on pins 0,1
//#ifdef DEBUGSIMPLE
//  Serial_Print_Line("serial1 works");
//#endif
//  Serial3.begin(9600);                                                          // set baud rate for Serial 3 is used to communicate with the CO2 sensor
//#ifdef DEBUGSIMPLE
//  Serial_Print_Line("serial3 works");
//#endif
  delay(500);

  Wire.begin(I2C_MASTER,0x00,I2C_PINS_18_19,I2C_PULLUP_EXT,I2C_RATE_400);      // using alternative wire library
//#ifdef DEBUGSIMPLE
  Serial_Print_Line("successfully opened I2C");
//#endif
  dac.vdd(5000); // set VDD(mV) of MCP4728 for correct conversion between LSB and Vout

/*NOTES*/ // REINITIATE ONCE NEW BLUETOOTH IS CONNECTED
/*
  TCS3471.setWaitTime(200.0);
  TCS3471.setIntegrationTime(700.0);
  TCS3471.setGain(TCS3471_GAIN_1X);
  TCS3471.enable();
*/


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
#define BLERESET 15
#define DEBUG_DC 2
#define DEBUG_DD 11


// sample and hold (hold + release detector cap)
#define HOLDM    6
#define HOLDADD 21

#define SS2      9
#define SCK1     13
#define LDAC2    16
#define LDAC3    17
#define SS1      22
#define LSET     28

#define IOEXT1   31
#define IOEXT2   32

#define UNUSED2  33
#define UNUSED1  14

  pinMode(29,OUTPUT);

  pinMode(3,INPUT);
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
  pinMode(BATT_LEVEL, INPUT); 
  digitalWriteFast(PWR_OFF, LOW);                                               // pull high to power off, pull low to keep power.
  digitalWriteFast(PWR_OFF_LIGHTS, HIGH);                                               // pull high to power on, pull low to power down.
  pinMode(13, OUTPUT);
//&&
//  analogReadAveraging(1);                                                       // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u) - this gets changed later by each protocol
  pinMode(DETECTOR1, EXTERNAL);                                                 // use external reference for the detectors
  pinMode(DETECTOR2, EXTERNAL);
//&&
//  analogReadRes(ANALOGRESOLUTION);                                              // set at top of file, should be 16 bit
//  analogresolutionvalue = pow(2,ANALOGRESOLUTION);                              // calculate the max analogread value of the resolution setting
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

  float tmp = 0;
  EEPROM_readAnything(16,tmp);
  if (tmp != (float) FIRMWARE_VERSION) {                                                // if the current firmware version isn't what's saved in EEPROM memory, then...
    EEPROM_writeAnything(16,(float) FIRMWARE_VERSION);                          // save the current firmware version
  }

//  MAG3110_init();                                                                // initialize compass and accelerometer
//  MMA8653FC_init();


}
