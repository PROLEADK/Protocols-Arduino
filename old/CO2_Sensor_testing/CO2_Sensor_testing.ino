// Control for HTU21D Temp/Hum sensor using Teensy 3.0
// Written by Greg Austic
// GPL 3.0 license

// TO DO:
// still need to implement the checksum for error identification...

// CO2 is on UART 3 on Teensy pins 7 and 8... make sure that 7 and 8
// aren't already used...

// for details see - http://www.meas-spec.com/downloads/HTU21D.pdf

// to start set SCK to high and wait for 15ms to enter idle state
// ready to accept commands.  Use soft reset to reset commands
// without turning off and on the machine.]

// Default resolution for temperature is 14 bits, for RH is 12 bits

// After performing a 14 bit temp measurement it takes max 50ms to save the data
// After performing a 12 bit HR measurement, it takes max 13ms to save the data
// So, when you poll the HTU21D for data, wait at least this long

// Command start bit - start sck and data high... drop data low, then drop sdk low
// Command stop bit - start sck and data low... pull sck high, then pull data high

  // device address - 0x40
  // plus 1 for read and 0 for write
  // take temperature measurement and save the result by doing -->
  // 0xF3, 11110011 - trigger temp measurement
  // 0xF5, 11110101 - trigger humidity measurement
  // 0xE6, 11100110 - write user register
  // 0xE7, 11100111 - read user register

// device acknolwedges that it succesfully received command by pulling data pin low
// after the 8th communication bit

// NOTE!
// Teensy 3.0 requires pullup resistors. The on-chip pullups are not used. 4.7K resistors are recommended for most applications.

#include <Wire.h>
#define temphumid_address 0x80

int led = 13;
int sck = 19; // clock pin
int sda = 18; // data pin
int wait = 100; // typical delay to let device finish working before requesting the data
byte byte1;
byte byte2;
unsigned int tempval;
unsigned int rhval;
float temperature;
float rh;

void setup() {                
  Serial.begin(9600);
  Wire.begin();
  //  pinMode(sck, OUTPUT); 
  //  pinMode(sda, OUTPUT);  
  //  digitalWrite(sck, HIGH); // turn HTU21D on in idle mode
  delay(wait);
}

void relh() {

  delay(2000);

  Wire.beginTransmission(0x40); // 7 bit address
  Wire.send(0xF5); // trigger temp measurement
  Wire.endTransmission();
  Serial.println("Sent the data");
  delay(wait);

  // Print response and convert to Celsius:
  Wire.requestFrom(0x40, 2);
//  Serial.print("this much data available: ");
//  Serial.println(Wire.available());
  byte1 = Wire.read();
  byte2 = Wire.read();
//  Serial.print("Bytes 1 and 2: ");
//  Serial.print(byte1);
//  Serial.print(", ");
//  Serial.println(byte2);
  rhval = byte1;
//  Serial.println(tempval);
  rhval<<=8; // shift byte 1 to bits 1 - 8
  rhval+=byte2; // put byte 2 into bits 9 - 16
  rhval>>=2; // shift to 14 bit from 16 bit
  Serial.print("14 bit relative humidity value: ");
  Serial.println(rhval);
  Serial.print("Relative humidity in %: ");
  rh = 125*(rhval/pow(2,16))-6;
  Serial.print(rh);
  Serial.println();
  Serial.println();
}


void temp() {

  delay(2000);

  Wire.beginTransmission(0x40); // 7 bit address
  Wire.send(0xF3); // trigger temp measurement
  Wire.endTransmission();
  Serial.println("Sent the data");
  delay(wait);

  // Print response and convert to Celsius:
  Wire.requestFrom(0x40, 2);
//  Serial.print("this much data available: ");
//  Serial.println(Wire.available());
  byte1 = Wire.read();
  byte2 = Wire.read();
//  Serial.print("Bytes 1 and 2: ");
//  Serial.print(byte1);
//  Serial.print(", ");
//  Serial.println(byte2);
  tempval = byte1;
//  Serial.println(tempval);
  tempval<<=8; // shift byte 1 to bits 1 - 8
  tempval+=byte2; // put byte 2 into bits 9 - 16
  tempval>>=2; // shift to 14 bit from 16 bit
  Serial.print("14 bit temperature value: ");
  Serial.println(tempval);
  Serial.print("Temperature in Celsius: ");
  temperature = 175.72*(tempval/pow(2,16))-46.85;
  Serial.print(temperature);
  Serial.println();
  Serial.println();
}

void loop() {

  temp();
  delay(1000);
  rh();
}





