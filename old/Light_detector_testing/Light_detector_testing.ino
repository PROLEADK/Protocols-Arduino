// Light and detector test script for Photosynq device using Teensy 3.0
// Written by Greg Austic
// GPL 3.0 license

/*
PIN LOCATIONS AND FUNCTIONS

Lx and Rx are the physical locations of the pins, counting starting
with 1, with 1 on the side witht he USB connection, and the USB
connection facing up.

M1 - measurement light 1 - Amber Luxeon Z
M2 - measurement light 2 - Green Luxeon Z
IR - IR calibration light -  ?????
S1 - saturating light - Red Luxeon Z

      ------------
 --  |            |
|M1| | 7x7.8mm    |
 --  | Detector   |
 --  |            |
|M2| |            |
 --  |            |
      -------------
        --     --
       |IR|   |S1|
        --     --
M1 Measurement Light control pins:
Teensy Pin     Physical Pin      Function
23              R4               intensity setting pwm (same pin sets both M1 and M2 intensity)
15              R12              on/off (HIGH/LOW) toggle

M2 Measurement Light control pins:
23              R4               intensity setting, pwm (same pin sets both M1 and M2 intensity)
16              R11              on/off (HIGH/LOW) toggle

IR Calibration Light control pins:
9               L11              intensity setting, pwm
14              R13              on/off (HIGH/LOW) toggle

Saturating Light control pins:
IR Calibration Light control pins:
3               L5              intensity setting intensity 1, pwm
4               L6              intensity setting intensity 2, pwm (having 2 pwm values allows quick near zero delay switching between two intensities during an experiment... otherwise switching pwm without presents takes a few milliseconds to stabilize)
5               L7              switch between pwm intensity 1 and 2 (pin LOW will use intensity 1, pin HIGH will use intensity 2)
20              R7              on/off (HIGH/LOW) toggle

*/

int measuringlight1 = 15;
int measuringlight2 = 16;
int measuringlight_pwm = 23;
int saturatinglight1 = 20;
int saturatinglight1_intensity2 = 3;
int saturatinglight1_intensity1 = 4;
int saturatinglight1_intensity_switch = 5;
int calibratinglight1 = 14;
int calibratinglight1_pwm = 9;
int detector1 = A10;
int analogresolution = 16;
int analogresolutionvalue;
int i;
int data0;
unsigned long start1;
unsigned long pulselengthon = 25; // in us
unsigned long cyclelength = 10; // in ms
unsigned long cycles = 200;
int wait = 20;
int pwmhigh = 255;
int pwmmed = 100;
int pwmlow = 1;
int x;

void setup() {
  delay(500);
  Serial.begin(115200); // set baud rate for Serial communication to computer via USB

  pinMode(13, OUTPUT);
  pinMode(measuringlight1, OUTPUT); // set pin to output
  pinMode(measuringlight2, OUTPUT); // set pin to output
  pinMode(measuringlight_pwm, OUTPUT); // set pin to output  
  pinMode(saturatinglight1, OUTPUT); // set pin to output
  pinMode(saturatinglight1_intensity2, OUTPUT); // set pin to output
  pinMode(saturatinglight1_intensity1, OUTPUT); // set pin to output
  pinMode(saturatinglight1_intensity_switch, OUTPUT); // set pin to output
  pinMode(calibratinglight1, OUTPUT); // set pin to output
  pinMode(calibratinglight1_pwm, OUTPUT); // set pin to output  
//  pinMode(actiniclight1, OUTPUT); // set pin to output
  analogReadAveraging(1); // set analog averaging to 1 (ie ADC takes only one signal, takes ~3u
  pinMode(detector1, EXTERNAL);
  analogReadRes(analogresolution);
  analogresolutionvalue = pow(2,analogresolution); // calculate the max analogread value of the resolution setting
}

// pwm on 23 is working... but light is always on
// also red light comes on on startup
// set intensity switch to low to access intensity 1, set to high to access intensity 2.

// Only intensity 1 works (pin 4) when switch is set low
// There is no pwm when the pin is set to high - so either switch isn't working, or intensity 2 (pin 3) does not oscillate properly
// Checked - pwm output from pins 3 and 4 are fine, but intensity set to high causes an ~50% pwm equivalent output
// Saturating light is still noisy and some of the lights are finicky (shift a cable or touch the board, and they go on or off)
// Noisy saturating light is causing the detector to pick up saturating signal (probably... may be electronic noise)
// Proximity of Z's to the detector - they're too far away!  You'd have to adjust detector board in order to replace Z's with Luxeons.  They should be closer
// if that requires that we move to Zs and no rebels then ok for now.

void loop() {
// testsats();
//testlights();
simpletest();
//measuring();
}

void measuring() {
  Serial.println("turn on 1");
  analogWrite(measuringlight_pwm,200);
  digitalWriteFast(measuringlight1, HIGH);
  digitalWriteFast(13,HIGH);
////  digitalWriteFast(measuringlight2, LOW);
  delay(1500);
  Serial.println("turn off 1");
//  analogWrite(measuringlight_pwm,20);

////  analogWrite(measuringlight_pwm,150);
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(13,LOW);
////  digitalWriteFast(measuringlight2, HIGH);
  delay(1500);
}

void simpletest() {
  Serial.println("measuring lights - low intensity, 1,2");
  analogWrite(measuringlight_pwm,0);
  delay(500);
  digitalWriteFast(measuringlight1, HIGH);
  delay(1000);
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(measuringlight2, HIGH);
  delay(1000);
  digitalWriteFast(measuringlight2, LOW);
  Serial.println("measuring lights - high intensity, 1,2");
  analogWrite(measuringlight_pwm,255);
  delay(500);
  digitalWriteFast(measuringlight1, HIGH);
  delay(1000);
  digitalWriteFast(measuringlight1, LOW);
  digitalWriteFast(measuringlight2, HIGH);
  delay(1000);
  digitalWriteFast(measuringlight2, LOW);

  analogWrite(saturatinglight1_intensity1, 2); // set saturating light intensity
  analogWrite(saturatinglight1_intensity2, 255); // set saturating light intensity
  delay(500);
  digitalWriteFast(saturatinglight1_intensity_switch, HIGH);
  Serial.println("saturating light - intensity 1 - 255");
  delay(500);
  digitalWriteFast(saturatinglight1, HIGH);
  delay(1000);
  digitalWriteFast(saturatinglight1, LOW);
  delay(500);  
  digitalWriteFast(saturatinglight1_intensity_switch, LOW);
  Serial.println("saturating light - intensity 2 - 2");
  delay(500);
  digitalWriteFast(saturatinglight1, HIGH);
  delay(1000);
  digitalWriteFast(saturatinglight1, LOW);
  delay(500);

  Serial.println("calibrating light - low intensity - 2");
  analogWrite(calibratinglight1_pwm,2);
  delay(500);
  digitalWriteFast(calibratinglight1, HIGH);
  delay(1000);
  digitalWriteFast(calibratinglight1, LOW);
  delay(500);

  Serial.println("calibrating light - high intensity - 255");
  analogWrite(calibratinglight1_pwm,255);
  delay(500);
  digitalWriteFast(calibratinglight1, HIGH);
  delay(1000);
  digitalWriteFast(calibratinglight1, LOW);
  delay(500);

}

void testsats() {
  

/*  
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // intensity 1 on

for (x=0;x<255;x++) {
  analogWrite(saturatinglight1_intensity1, x); // set saturating light intensity
  analogWrite(saturatinglight1_intensity2, x); // set saturating light intensity
  delay(15);
  digitalWriteFast(saturatinglight1, HIGH);
  delay(1);
  digitalWriteFast(saturatinglight1, LOW);
}

delay(1000);

digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 2 on
delay(1000);

for (x=0;x<255;x++) {
  analogWrite(saturatinglight1_intensity1, x); // set saturating light intensity
  analogWrite(saturatinglight1_intensity2, x); // set saturating light intensity
  delay(15);
  digitalWriteFast(saturatinglight1, HIGH);
  delay(1);
  digitalWriteFast(saturatinglight1, LOW);
}

delay(1000);

analogWrite(saturatinglight1_intensity1, 0); // set saturating light intensity
analogWrite(saturatinglight1_intensity2, 0); // set saturating light intensity
delay(100);
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // intensity 1 on


for (x=0;x<255;x++) {
  delay(15);
  digitalWriteFast(saturatinglight1, HIGH);
  delay(1);
  digitalWriteFast(saturatinglight1, LOW);
}
*/


analogWrite(saturatinglight1_intensity1, 255); // set saturating light intensity
delay(500);
analogWrite(saturatinglight1_intensity2, 255); // set saturating light intensity
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 2 on
delay(500);
digitalWriteFast(saturatinglight1, HIGH);
delay(500);
digitalWriteFast(saturatinglight1, LOW);
delay(500);
digitalWriteFast(saturatinglight1, HIGH);
delay(500);
digitalWriteFast(saturatinglight1, LOW);
delay(500);

delay(2000);

analogWrite(saturatinglight1_intensity1, 255); // set saturating light intensity
delay(500);
analogWrite(saturatinglight1_intensity2, 255); // set saturating light intensity
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // intensity 2 on
delay(500);
digitalWriteFast(saturatinglight1, HIGH);
delay(500);
digitalWriteFast(saturatinglight1, LOW);
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 1 on
delay(500);
digitalWriteFast(saturatinglight1, HIGH);
delay(500);
digitalWriteFast(saturatinglight1, LOW);
delay(500);


delay(2000);

/*
delay(1000);

analogWrite(saturatinglight1_intensity1, 1); // set saturating light intensity
analogWrite(saturatinglight1_intensity2, 1); // set saturating light intensity
delay(500);

digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 2 on
digitalWriteFast(saturatinglight1, HIGH);
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // intensity 1 on
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 2 on
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // intensity 1 on
delay(500);

digitalWriteFast(saturatinglight1, LOW);
delay(2000);

analogWrite(saturatinglight1_intensity1, 255); // set saturating light intensity
analogWrite(saturatinglight1_intensity2, 0); // set saturating light intensity
delay(500);

digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 2 on
digitalWriteFast(saturatinglight1, HIGH);
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // intensity 1 on
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 2 on
delay(500);
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // intensity 1 on
delay(500);

digitalWriteFast(saturatinglight1, LOW);
delay(2000);

*/

}

void testlights() {

analogWrite(saturatinglight1_intensity1, pwmlow); // set saturating light intensity
  
digitalWriteFast(saturatinglight1_intensity_switch, LOW); // turn intensity 1 on
pulse1(measuringlight_pwm, measuringlight1, pwmhigh);
pulse1(measuringlight_pwm, measuringlight1, pwmlow);
pulse1(measuringlight_pwm, measuringlight2, pwmhigh);
pulse1(measuringlight_pwm, measuringlight2, pwmlow);
// pulse1(calibratinglight1_pwm, calibratinglight1, pwmlow);
/*
digitalWriteFast(saturatinglight1_intensity_switch, HIGH); // intensity 2 on
pulse1(measuringlight_pwm, measuringlight1, pwmhigh);
pulse1(measuringlight_pwm, measuringlight1, pwmlow);
pulse1(measuringlight_pwm, measuringlight2, pwmhigh);
pulse1(measuringlight_pwm, measuringlight2, pwmlow);
pulse1(calibratinglight1_pwm, calibratinglight1, pwmlow);
*/

delay(1000);

}

void pulse1(int a, int b, int c) { // a=pwm setting, b=on/off toggle, c=pwm setting (and sat pwm1), d=sat pwm 2
  
  analogWrite(a, c); // turn pwm pin a to whatever setting you want d
  delay(10); // give time for pwm to settle
  for (i=0;i<cycles;i++) { // repeat 100 times
    if (i == cycles/2) {
      digitalWriteFast(saturatinglight1, HIGH);
    }
    data0 = analogRead(detector1);
    start1 = micros();
    digitalWriteFast(b, HIGH);
    start1=start1+pulselengthon;
    while (micros()<start1) {}
    digitalWriteFast(b, LOW);
    if (i == cycles*3/4) {
      digitalWriteFast(saturatinglight1, LOW);
    }
    Serial.print(data0);
    Serial.print(",");
    delay(cyclelength);
  }
  i=0;
  delay(50);
  Serial.println();
}

  
