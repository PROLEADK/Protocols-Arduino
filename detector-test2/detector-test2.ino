

// program to pulse LED, read detector value and report noise

#define USE_SPI
#undef USE_LED
#define USE_DISCHARGE

#define GAIN_BITS 1.7        // extra effective bits due to gain being higher than beta detector - example, 4x more gain = 2.0 bits
#define DIV_BITS  1.6        // extra bits due to missing voltage divider

                             // Gain reduces headroom and anything more than 4x will clip with Greg's examples


// Note: best results came from a resistor divider from 3.3V to AGND and not running the serial monitor.
// 3.3V ext reference.
// Use a median filter and lots of samples if you need more bits


//#include "mcp4728.h"
//#include <i2c_t3.h>
//#include "ADC.h"
//#include <pins_arduino.h>

#define ledPin 23               // pin used to flash LED
#define dischargePin 6          // to discharge capacitor

//mcp4728 dac = mcp4728(0);

#include <SPI.h>    // include the new SPI library:

void setup() {

  delay(1000);

#ifdef USE_LED
  //pinMode(ledPin, OUTPUT);
  //digitalWriteFast(ledPin, HIGH);
#endif

#ifdef USE_DISCHARGE
  pinMode(dischargePin, OUTPUT);
  digitalWriteFast(dischargePin, HIGH);   // high to discharge
#endif

  Serial.println("hello");

  SPI.begin ();
  delay(1000);

  // clock
  //ARM_DEMCR |= ARM_DEMCR_TRCENA;
  //ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

//#define   CLOCK_RATE 96.00000E6

  Serial.println(AD7689_read(0));            // read value with precise capture time 
  Serial.print("temp = ");
  Serial.println(AD7689_read_temp());
  delay(2000);
  
} // setup()


void loop()
{
  
  uint32_t x;
  uint32_t max = 0;
  uint32_t min = 65535;
  uint32_t  sum = 0;  // reset our accumulated sum of input values to zero
  uint32_t n = 0; // count of how many readings so far
  float mean = 0, delta = 0, m2 = 0, variance = 0, stdev = 0;
  static float stdev_min = 999; // to calculate standard deviation

  //uint32_t start_time = ARM_DWT_CYCCNT;
  uint32_t oldT = millis();   // record start time in milliseconds

  AD7689_set (0);        // set ADC channel

#define SAMPLES 1000

  int samples[SAMPLES];              // save values, looking for trends/drft

  for (int i = 0; i < SAMPLES + 1; i++) {

    noInterrupts();

    // always turn off serial monitor
    // subtracting off a baseline doesn't help much - problem is HF noise

    // a cycle just before then 1 usec improves bits by .2 to 10.2 bits
    //digitalWriteFast(SAMPLE_AND_HOLD, LOW);   // open switch, start integrating
    //digitalWriteFast(SAMPLE_AND_HOLD, HIGH);  // turn off integrating, close switch
    //delayMicroseconds(1);

    int base = AD7689_read(0);             // read baseline value just before pulse

    digitalWriteFast(dischargePin, LOW);   // charge the measuring capacitor


    // *********

#ifdef USE_LED
    digitalWriteFast(ledPin, HIGH);   // turn on measuring light
#endif
    delayMicroseconds(10);            // pulse width

    AD7689_sample();                  // start conversion

#ifdef USE_LED
    digitalWriteFast(ledPin, LOW);    // turn off measuring 
#endif

    digitalWriteFast(dischargePin, HIGH);   // charge the measuring capacitor

    delayMicroseconds(3);             // wait for conversion to complete
    x = AD7689_read_sample();         // read value (could subtract off baseline)

    AD7689_read(0);

    // *********

    if (i > 0) {            // skip first one
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
    
    //delay(100);    // needed to allow capacitor/integrator/filter to discharge

  } // for

  interrupts();
  delay(100);

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
  //Serial.print(" St.Dev: ");  Serial.println(stdev, 3);
  //Serial.print(" St.Dev best: ");  Serial.println(stdev_min, 3);
  Serial.print(" PPM: ");  Serial.println((stdev / average) * 1000000, 3);
  Serial.print(" PPM full scale: ");  Serial.println((stdev / 65536) * 1000000, 3);
  Serial.print(" bits (95%) = "); Serial.println(15 - log(stdev * 2) / log(2.0)); // 2 std dev from mean = 95%
  Serial.print(" bits (95%) + Gain = "); Serial.println(GAIN_BITS + DIV_BITS + 15 - log(stdev * 2) / log(2.0)); // 2 std dev from mean = 95%
  Serial.println("");

  delay(2000);

}  // loop


#if 0
int adc_median(int pin)
{
#undef SAMPLES
#define SAMPLES 5
  int data_array[SAMPLES];
  //int sum = 0;
  //int first, last;

  for (int i = 0; i < SAMPLES; i++) {
    data_array[i] = adc->analogRead(pin, ADC_0);
    //sum += data_array[i];
  }

  for (int i = 0; i < SAMPLES; i++) {
    for (int j = 0; j < SAMPLES; j++) {
      if (data_array[i] > data_array[j]) {
        register int temp = data_array[i];
        data_array[i] = data_array[j];
        data_array[j] = temp;
      }
    }
  }

  return data_array[SAMPLES / 2];
  //return sum/SAMPLES;

}

#endif


#ifdef USE_SPI
//*************************************************

// AD7689 16 bit SPI A/D converter interface
// Supports accurate sample time
// Note: sampling/aquisition time is 1.8 usec
// Note: reference can drift +/- 10 ppm per degree C (16 counts from 25C to 0C)

#define AD7689_PIN 22    // CNV and chip select pin to use for SPI (10 is standard)

// for debugging

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

// do conversion and return result - slow and imprecise

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

  pinMode (AD7689_PIN, OUTPUT);      // set the Slave Select Pin as output

  SPI.beginTransaction (AD7689_settings);

  // send config (RAC mode)
  digitalWrite (AD7689_PIN, LOW);
  SPI.transfer (ad7689_config >> 8);  // high byte
  SPI.transfer (ad7689_config & 0xFF);  // low byte, 2 bits ignored
  digitalWrite (AD7689_PIN, HIGH);
  delayMicroseconds(6);

  // dummy
  digitalWrite (AD7689_PIN, LOW);
  SPI.transfer (ad7689_config >> 8);  // high byte
  SPI.transfer (ad7689_config & 0xFF);  // low byte, 2 bits ignored
  digitalWrite (AD7689_PIN, HIGH);
  delayMicroseconds(6);

  SPI.endTransaction ();

}

// start the conversion - very time accurate, takes 1.8 usec

inline void AD7689_sample() __attribute__((always_inline));
void
AD7689_sample()
{
  // do conversion
  digitalWriteFast(AD7689_PIN, LOW);         // chip select
  digitalWriteFast(AD7689_PIN, HIGH);        // chip deselect  (starts conversion)
}

uint16_t
AD7689_read_sample()
{
  delayMicroseconds(6);                      // wait till complete
  SPI.beginTransaction (AD7689_settings);
  digitalWrite (AD7689_PIN, LOW);            // chip select
  uint16_t val = SPI.transfer (ad7689_config  >> 8) << 8;   // high byte
  val |= SPI.transfer (ad7689_config);      // low byte
  digitalWrite (AD7689_PIN, HIGH);          // chip select
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

#endif

