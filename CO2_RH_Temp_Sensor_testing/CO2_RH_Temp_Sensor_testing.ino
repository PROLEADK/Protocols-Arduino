// Control for HTU21D Temp/Hum sensor on Photosynq device using Teensy 3.0
// Written by Greg Austic
// GPL 3.0 license

// HTU21D TEMPERATURE AND HUMIDITY SENSOR:
// for details see - http://www.meas-spec.com/downloads/HTU21D.pdf
// Default resolution for temperature is 14 bits, for RH is 12 bits
// Use soft reset to reset commands without turning off and on the machine.
// take measurement and save results by sending the following I2C commands to the HTU21D Temp/Hum sensor --> 0xF3 - trigger temp measurement, 0xF5 - trigger humidity measurement.  See spec sheet for other commands
// Teensy 3.0 requires pullup resistors. The on-chip pullups are not used. 4.7K resistors are recommended for most applications.

// SENSAIR S8 CO2 SENSOR:
// CO2 sensor code created by Jason Berger, Co2meter.com
// CO2 sensor requires that it sits in regular air - store device in open air
// Additional information available at link below (for K series sensors, but applies to S8 as well
// http://www.co2meters.com/Documentation/AppNotes/AN126-K3x-sensor-arduino-uart.pdf

// TO DO and NOTES:
// still need to implement the checksum for error identification on CO2, temp/hum sensors
// Also, calibration options for CO2 and temp/hum sensors
// Ideally we should use the kSeries.h library, but currently it does not compile with Teensy 3.0 - http://www.co2meters.com/Documentation/AppNotes/AN126-K3x-sensor-arduino-uart.zip
// this issue is being worked on the PJRC forums here: http://forum.pjrc.com/threads/17461-SoftwareSerial-doesn-t-compile-for-Teensy-3-it-seems
// check... do we really need 4.7k pullup resistors for the HTU21D?

#include <Wire.h>
//#include <kSeries.h>
#define temphumid_address 0x40 // HTU21d Temp/hum I2C sensor address

// HTU21D Temp/Humidity variables
int sck = 19; // clock pin
int sda = 18; // data pin
int wait = 200; // typical delay to let device finish working before requesting the data
unsigned int tempval;
unsigned int rhval;
float temperature;
float rh;

// S8 CO2 variables
byte readCO2[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};  //Command packet to read CO2 (see app note)
byte response[] = {0,0,0,0,0,0,0};  //create an array to store CO2 response
float valMultiplier = 0.1;

void setup() { 
  Serial.begin(9600);
  Serial3.begin(9600);
  Wire.begin();
  delay(wait);
}

void loop() {
  Serial.println();
  temp();
  relh();
  Co2();
  delay(1000);
  Serial.println();
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
  Serial.print("Relative humidity in %: ");
  rh = 125*(rhval/pow(2,16))-6;
  Serial.println(rh);
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
}

void Co2() {
  requestCo2(readCO2);
  unsigned long valCO2 = getCo2(response);
  Serial.print("Co2 ppm = ");
  Serial.println(valCO2);
  delay(2000);
}

void requestCo2(byte packet[]) {
  while(!Serial3.available()) { //keep sending request until we start to get a response
    Serial3.write(readCO2,7);
    delay(50);
  }
  int timeout=0;  //set a timeoute counter
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





