#include <SPI.h>		// include the new SPI library:

void
setup ()
{
  // initialize SPI:
  SPI.begin ();
  delay(1000);
  set_AD7689(0); // set channel
} 

void
loop ()
{
//  delayMicroseconds(200);
  Serial.println(read_AD7689());            // read value with precise capture time 
  delay(5000);
  
} // loop()


//*************************************************

// AD7689 16 bit SPI A/D converter interface
// Supports highly accurate sample time

#define AD7689_PIN 10		// chip select pin to use (10 is standard)

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


// set up the speed, mode and endianness of each device
// MODE0: SCLK idle low (CPOL=0), MOSI read on rising edge (CPHI=0)
SPISettings AD7689_settings (10000000, MSBFIRST, SPI_MODE0);

// Note: use CPHA = CPOL = 0 
// Note: two dummy conversions are required on startup

static uint16_t ad7689_config = 0;

void
set_AD7689 (int channel)
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
  unsigned int ad7689_config = 0;

  ad7689_config |= 1 << CFG;		// update config on chip
  ad7689_config |= 0B111 << INCC;	// mode - single ended, differential, ref, etc
  ad7689_config |= channel << INx;	// channel
  ad7689_config |= 0 << BW;		// 1 adds more filtering
  //ad7689_config |= 0B0 << REF;	// use internal 2.5V reference
  ad7689_config |= 0B110 << REF;	// use external reference (maybe ~3.3V)
  ad7689_config |= 0 << SEQ;		// don't auto sequence
  ad7689_config |= 0 << RB;		// don't read back config value

  ad7689_config = ad7689_config << 2;   // convert 14 bits to 16 bits

  //Serial.printf("ADC config: %d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n",BYTETOBINARY(config>>8), BYTETOBINARY(config));  // debug only

  pinMode (AD7689_PIN, OUTPUT);      // set the Slave Select Pin as output
  
  SPI.beginTransaction (AD7689_settings);

  // send config (RAC mode)
  digitalWrite (AD7689_PIN, LOW);
  SPI.transfer (ad7689_config >> 8);	// high byte
  SPI.transfer (ad7689_config & 0xFF);	// low byte, 2 bits ignored
  digitalWrite (AD7689_PIN, HIGH);
  delayMicroseconds(6);
  
  // dummy
  digitalWrite (AD7689_PIN, LOW);
  SPI.transfer (ad7689_config >> 8);	// high byte
  SPI.transfer (ad7689_config & 0xFF);	// low byte, 2 bits ignored
  digitalWrite (AD7689_PIN, HIGH);
  delayMicroseconds(6);

  SPI.endTransaction ();

}

// do conversion and return result
// assume that the channel is already set correctly
// sample & hold and conversion starts immediately

inline uint16_t read_AD7689() __attribute__((always_inline));

uint16_t
read_AD7689 ()            
{

  // do conversion
  digitalWriteFast(AD7689_PIN, LOW);         // chip select
  digitalWriteFast(AD7689_PIN, HIGH);        // chip deselect  (starts conversion)
  delayMicroseconds(6);                      // wait till complete
  
  // read conversion result
  SPI.beginTransaction (AD7689_settings);  
  digitalWrite (AD7689_PIN, LOW);            // chip select
  uint16_t val = SPI.transfer (ad7689_config  >> 8) << 8;   // high byte
  val |= SPI.transfer (ad7689_config);	    // low byte
  digitalWrite (AD7689_PIN, HIGH);          // chip select
  delayMicroseconds(6);                     // wait for second conversion to complete
  SPI.endTransaction ();

  return val;
}
