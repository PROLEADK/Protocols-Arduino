/*
DESCRIPTION:
This file is used to measure pulse modulated fluorescence (PMF) using a saturating pulse and a measuring pulse.  The measuring pulse LED (for example, Rebel Luxeon Orange) can itself produce some infra-red
fluorescence which is detected by the detector.  This signal causes the detector response to be higher than expected which causes bias in the results.  So, in order to account for this added infra-red, we 
perform a calibration.  The calibration is a short set of pulses which occur at the beginning of the program from an infra-red LED (810nm).  The detector response from these flashes indicates how "reflective"
the sample is to infra-red light in the range produced by our measuring LED.  We can create a calibration curve by measuring a very reflective sample (like tin foil) and a very non-reflective sample (like
black electrical tape) with both the calibrating LED and the measuring LED.  This curve tells us when we see detector response X for the calibrating LED, that correspond to a baseline response of Y for the
measuring LED.  Once we have this calibration curve, when we measure an actual sample (like a plant leaf) we automatically calculate and remove the reflected IR produced by the measuring pulse.  Vioala!
Using Arduino 1.0.3 w/ Teensyduino installed downloaded from http://www.pjrc.com/teensy/td_download.html .   
*/

// NOTES:
// check and see if you can put the =0 right in the int with multiple variables in parenthesis!!
// use Gain = 2 only to simplify calibration.  The resistor vallue MUST be set before calibration, and for each change in resistor value, a new calibration (tin/tape) must be performed.

// data for calibrating pulses (the number of cycles can be shortened to speed up calibration)
unsigned long calpulsecycles = 50; // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)

// data for measuring and saturating pulses --> to calculate total time=pulsecycles*(pulselengthon + pulselengthoff)
unsigned long pulsecycles = 150; // Number of times the "pulselengthon" and "pulselengthoff" cycle (on/off is 1 cycle) (maximum = 1000 until we can store the read data in an SD card or address the interrupts issue)
unsigned long saturatingcycleon = 10; //The cycle number in which the saturating light turns on (set = 0 for no saturating light)
unsigned long saturatingcycleoff = 30; //The cycle number in which the saturating light turns off
unsigned long pulselengthon = 25; // Pulse LED on length in uS (minimum = 5us based on a single ~4us analogRead - +5us for each additional analogRead measurement in the pulse).
unsigned long pulselengthoff = 49975; // Pulse LED off length in uS (minimum = 20us + any additional operations which you may want to call during that time).
int measuringlight1 = 4;
int saturatinglight1 = 3;
int calibratinglight1 = 2;
int detector1 = A0;
unsigned long start1,start1orig,end1, calstart1orig, calend1;
unsigned long pulselengthoncheck, pulselengthoffcheck, pulsecyclescheck, totaltimecheck, caltotaltimecheck;
int data1, data2, data3, data4, dataaverage, caldata1, caldata2, caldata3, caldata4, caldataaverage1, caldataaverage2;
float data1f, data2f, data3f, data4f, baselinetin, baselinetape;
int i = 0; // This detects the number of analogReads() per pulse
String data = 0;
String caldatatin = 0;
String caldatatape = 0;
int val = 0;
int cal = 0;
int val2 = 0;
int flag = 0;

void setup() {
  
Serial.begin(38400); // set baud rate for serial communication
pinMode(measuringlight1, OUTPUT); // set pin to output
pinMode(saturatinglight1, OUTPUT); // set pin to output
analogReadAveraging(1); // set analog averaging to 1 (ie ADC takes only one signal, takes ~3us)]
start1 = 0;
start1orig = 0;
calstart1orig = 0;
end1 = 0;
calend1 = 0;
data1 = 0;
data2 = 0;
data3 = 0;
data4 = 0;
}
  
void loop() {

// METHOD 1: 
// Using a for loop to define ON/OFF cycles, and using an empty while loop to do the timing

// NOTES:
// interrupts (in the form of cli() and sei() OR the standard arduino interrupts / noInterrupts) seems to make no difference in the result... though the Interrupts call itself was better than sei()
// based on looking at the actual oscilliscope but not much
// Adding to a String variable using the =+ or the string.concat() version of the string function takes about 8us!
// BUT, setting a simple int data variable equal to our analog read, that takes no time at all.
// when concatenating a string, this is slowest -   data = data + data1; .  It's better to call the concat function once and concatenate multiple things at a time within it, rather than making multiple calls.
// Even for the very quick pulses when using WASP method or other really short methods, we should be able to use the ADC to get 1us timed flashes and then just run analogRead as fast as possible (~3us).  
// Then, we can drop any of the values which are clearly too low (this should be obvious on a graph)... or perhaps we can speed up our analogRead?

// IMPROVEMENTS:
// Find a better way to store data than the String function - it's really slow and causes our OFF time to be really slow also.
// Even just getting rid of second string concatenation to add the comma (data += ",") would allow us to shorten the OFF time minimum down to 10us.
// We need a better place to store the data (SD card, for example) data storage limitation is causing the maximum number of cycles to be ~1000.  SD card should take care of that problem.

// SPECS USING THIS METHOD: 
// Peak to Peak variability, ON and OFF length variability all is <1us.  Total measurement variability = 0 to +3us (regardless of how many cycles)

// digitalWriteFast(measuringlight1, HIGH); // OPTIONAL: Used with the RC circuit to measuring analogRead() timing (also, switch LOW and HIGH on the digitalWrite's in the program)
delay(3000); // give operator time to turn on the serial monitor and the capacitor to charge

// CALIBRATION SETUP

// ADD HERE:
// IF you can search saved files and find a calibration value which has the same filename AND same resistor value AND it's been more than X time since last calibration
// THEN ask user to calibrate (via Serial port for now)
// ELSE skip

Serial.println("Would you like to run a calibration? -- type 1 for yes or 2 for no");
while (cal == 0 | flag == 0) {
    cal = Serial.read()-48; // for some crazy reason, 0 = 48 in serial print land, so shift everything by 48
  if (cal == 1) {
    delay(50);
    Serial.println("Ok - calibrating and then running normal protocal");
    flag = 1;
  }
  else if (cal == 2) {
    delay(50);
    Serial.println("Ok - skipping calibration and going straight on to the normal protocol");
    flag = 1;
  }
  else if (cal > 2) {
    delay(50);
    Serial.println("That's not a 1 or a 2.  Please enter either 1 for yes, or 2 for no");
    cal = 0;
  }
}

// CALIBRATION
// Flash calibrating light to determine how reflective the sample is to ~700 - 800nm light.  This has been tested with Luxeon Rebel Orange as measuring pulse.

if (cal=1) { //  User decides whether or not to calibrate based on Serial input command.

Serial.println("Please place the shiny side of the calibration piece face up in the photosynq, and close the lid.");
Serial.println("When you're done, press 1");
while (flag == 0) {
  cal = Serial.read()-48; // for some crazy reason, 0 = 48 in serial print land, so shift everything by 48
  if (cal == 1) {
    delay(50);
    Serial.println("Great, thanks - let me get some data");
    flag = 1;
  }
}
flag = 0;

noInterrupts(); // turn off interrupts to reduce interference from other calls

calstart1orig = micros();
start1 = micros();
for (i=0;i<calpulsecycles;i++) {
  digitalWriteFast(calibratinglight1, HIGH); // During the ON phase, keep things as fast as possible - so simple data1 and data2 - no strings or anything confusing!
  caldata1 = analogRead(detector1); // user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
  caldata2 = analogRead(detector1);
  caldata3 = analogRead(detector1);
  caldata4 = analogRead(detector1);
  start1=start1+pulselengthon;
  while (micros()<start1) {}
  start1=start1+pulselengthoff;
  digitalWriteFast(measuringlight1, LOW); // Now, during OFF phase, we can average that data and save it as a string!
  caldataaverage1 = (caldata1+caldata2+caldata3+caldata4)/4; // Again, if you are using less than 4 analogReads, change the value that the points are divided by here.
  caldatatin += caldataaverage1;
  caldatatin += ",";
  baselinetin += (caldataaverage1/calpulsecycles); // if you are using less than 4 analogReads, change the value that the points are devided by.
  while (micros()<start1) {} 
}
calend1 = micros();

interrupts();

Serial.println("Now please place the black side of the calibration piece face up in the photosynq, and close the lid.");
Serial.println("When you're done, press 1");
while (flag == 0) {
  cal = Serial.read()-48; // for some crazy reason, 0 = 48 in serial print land, so shift everything by 48
  if (cal == 1) {
    delay(50);
    flag = 1;
  }
}
flag = 0;

noInterrupts(); // turn off interrupts to reduce interference from other calls

calstart1orig = micros();
start1 = micros();
for (i=0;i<calpulsecycles;i++) {
  digitalWriteFast(calibratinglight1, HIGH); // During the ON phase, keep things as fast as possible - so simple data1 and data2 - no strings or anything confusing!
  caldata1 = analogRead(detector1); // user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
  caldata2 = analogRead(detector1);
  caldata3 = analogRead(detector1);
  caldata4 = analogRead(detector1);
  start1=start1+pulselengthon;
  while (micros()<start1) {}
  start1=start1+pulselengthoff;
  digitalWriteFast(measuringlight1, LOW); // Now, during OFF phase, we can average that data and save it as a string!
  caldataaverage2 = (caldata1+caldata2+caldata3+caldata4)/4; // Again, if you are using less than 4 analogReads, change the value that the points are divided by here.
  caldatatape += caldataaverage2;
  caldatatape += ",";
  baselinetape+= (caldataaverage2/calpulsecycles); // if you are using less than 4 analogReads, change the value that the points are devided by.
  while (micros()<start1) {} 
}
calend1 = micros();

interrupts();

Serial.println("Great, thanks - done with calibration!");


// CALIBRATION CALCULATIONS
/*
CALIBRATION
ASK USER to put in the calibration piece, foil side up
RUN calibration and normal sample, save results
ASK USER to put in the calibration piece, black side up
RUN calibration and normal sample, save results
CALCULATE 
on X axis: tape = 0, tin = 1
calculate rebelslope == rebeltapevalue – rebeltinvalue
calculate IRslope == IRtapevalue – IRtinvalue
save values IRslope, rebelslope, rebeltape, rebeltin, IRtape, IRtin
baseline = rebelslope*(samplevalue-IRtinvalue)/IRslope – rebeltin
*/

int rebeltapevalue
int rebeltinvalue
int irtapevalue
int irtinvalue
int rebelslope
int irslope

rebeltapevalue
rebeltinvalue
irtapevalue
irtinvalue
rebelslope
irslope

rebelslope = baselinetin

left off here - try to make programs within the program! - should make things easier!






















}

// MEASUREMENTS
// Flash the LED in a cycle with defined ON and OFF times, and take analogRead measurements as fast as possible on the ON cycle
start1orig = micros();
start1 = micros();
for (i=0;i<pulsecycles;i++) {
  digitalWriteFast(measuringlight1, HIGH); // During the ON phase, keep things as fast as possible - so simple data1 and data2 - no strings or anything confusing!
  if (saturatingcycleon == i+1) {
    digitalWriteFast(saturatinglight1, HIGH); // During the ON phase, keep things as fast as possible - so simple data1 and data2 - no strings or anything confusing!    
  }
  data1 = analogRead(detector1); // user needs to set the number of analogReads() per ON cycle - add more for more analogReads()
  data2 = analogRead(detector1);
  data3 = analogRead(detector1);
  data4 = analogRead(detector1);
  start1=start1+pulselengthon;
  while (micros()<start1) {}
  start1=start1+pulselengthoff;
  digitalWriteFast(measuringlight1, LOW); // Now, during OFF phase, we can average that data and save it as a string!
  if (saturatingcycleoff == i+1) {
    digitalWriteFast(saturatinglight1, LOW); // Turn saturating light off on the proper cycle
  }
  dataaverage = ((data1+data2+data3+data4)/4)-baseline; // Again, if you are using less than 4 data points, change the value that the points are divided by here.  The baseline detector response is subtracted here
  data += dataaverage;
  data += ",";
  while (micros()<start1) {} 
}
end1 = micros();

interrupts();

// Measure the total time of the run, the time of one ON cycle, and the time of one OFF cycle 
totaltimecheck = end1 - start1orig;
caltotaltimecheck = calend1 = calstart1orig;

data1f = data1*3.3/1023; // convert the analog read into a voltage reading to correspond to the oscilliscope
data2f = data2*3.3/1023; // convert the analog read into a voltage reading to correspond to the oscilliscope
data3f = data3*3.3/1023; // convert the analog read into a voltage reading to correspond to the oscilliscope
data4f = data4*3.3/1023; // convert the analog read into a voltage reading to correspond to the oscilliscope

// Print values:
delay(50);

Serial.print("# of pulse cycles:  ");
Serial.println(pulsecycles);

Serial.print("total run length (measuring pulses):  ");
Serial.println(totaltimecheck);

Serial.print("total run length (calibration pulses):  ");
Serial.println(caltotaltimecheck);

Serial.print("value of i:  ");
Serial.println(i);

// You can try to add start and end variables to measure the pulse on/off time, but it's better and more accurate to use an oscilliscope
//Serial.print("pulse length on actual:  ");
//Serial.println(pulselengthoncheck);
//Serial.print("pulse length off actual:  ");
//Serial.println(pulselengthoffcheck);

Serial.println("averaged data points from each ON measurement cycle");
Serial.println(data);

Serial.println("averaged data points from each ON cycle during calibration - baseline reflectance");
Serial.println(caldata);

delay(50);

while (1) {};

}  

