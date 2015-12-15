// MMA8653FC accelerometer library
// based on code from hoelig

#include <Wire.h>

// I2C BUS:  already defined in "wire" librairy
// SDA: PIN 2 with pull up 4.7K to 3.3V on arduino Micro
// SCL: PIN 3 with pull up 4.7K to 3.3V on arduino Micro
// Accelerometer connected to +3.3V of arduino DO NOT CONNECT TO 5V (this will destroy the accelerometer!)
// all GND Pin of accelerometer connected to gnd of arduino

/********************ACCELEROMETER DATAS************/

// adresss of accelerometer
#define adress_acc 0X1D		// MMA8653FC and MMA8652FC
// address of registers for MMA8653FC
#define ctrl_reg1 0x2A
#define ctrl_reg2 0x2B
#define ctrl_reg3 0x2C
#define ctrl_reg4 0x2D
#define ctrl_reg5 0x2E
#define int_source 0x0C
#define status_ 0x00
#define f_setup 0x09
#define out_x_msb 0x01
#define out_y_msb 0x03
#define out_z_msb 0x05
#define sysmod 0x0B
#define xyz_data_cfg 0x0E

//******PROGRAM DATAS**********/
#define ctrl_reg_address 0     // to understand why 0x00 and not 0x01, look at the data-sheet p.19 or on the comments of the sub. This is valid only becaus we use auto-increment   

#define TEST
#ifdef TEST

void
setup ()
{
  Wire.begin ();		// start of the i2c protocol
  //Serial.begin (9600);		// start serial for output
  MMA8653FC_init ();			// initialize the accelerometer by the i2c bus. enter the sub to adjust the range (2g, 4g, 8g), and the data rate (800hz to 1,5Hz)
}

//------------------------------------------------------------------

void
loop ()
{
  
int axeXnow;
int axeYnow;
int axeZnow;

  MMA8653FC_read (&axeXnow, &axeYnow, &axeZnow);		
  
  // NOTE: This communication is taking a little less than 1ms to be done. (for data-rate calculation for delay)
  Serial.print (axeXnow);
  Serial.print (";");
  Serial.print (axeYnow);
  Serial.print (";");
  Serial.print (axeZnow);
  Serial.print (";");
  Serial.println ();
  delay (50);
}

#endif

//------------------------------------------------------------------

void
MMA8653FC_init ()
{
  I2C_SEND (ctrl_reg1, 0X00);	// standby to be able to configure
  delay (10);

  I2C_SEND (xyz_data_cfg, B00000000);	// 2G full range mode
  delay (1);
  //   I2C_SEND(xyz_data_cfg ,B00000001); // 4G full range mode
  //   delay(1);
  //   I2C_SEND(xyz_data_cfg ,B00000010); // 8G full range mode
  //   delay(1);

  I2C_SEND (ctrl_reg1, B00000001);	// Output data rate at 800Hz, no auto wake, no auto scale adjust, no fast read mode
  delay (1);
  //   I2C_SEND(ctrl_reg1 ,B00100001); // Output data rate at 200Hz, no auto wake, no auto scale adjust, no fast read mode
  //   delay(1);
  //   I2C_SEND(ctrl_reg1 ,B01000001); // Output data rate at 50Hz, no auto wake, no auto scale adjust, no fast read mode
  //   delay(1);
  //   I2C_SEND(ctrl_reg1 ,B01110001); // Output data rate at 1.5Hz, no auto wake, no auto scale adjust, no fast read mode
  //   delay(1);    
}

//------------------------------------------------------------------

static void
I2C_SEND (unsigned char REG_ADDRESS, unsigned char DATA)	//SEND data to MMA7660
{
  Wire.beginTransmission (adress_acc);
  Wire.write (REG_ADDRESS);
  Wire.write (DATA);
  Wire.endTransmission ();
}

//------------------------------------------------------------------

// read current values from accelerometer in milli-Gs

void
MMA8653FC_read(int *axeXnow, int *axeYnow, int *axeZnow)	
{
 
  Wire.beginTransmission (adress_acc);	//=ST + (Device Adress+W(0)) + wait for ACK
  Wire.write (ctrl_reg_address);	// store the register to read in the buffer of the wire library
  Wire.endTransmission ();	        // actually send the data on the bus -note: returns 0 if transmission OK-
  Wire.requestFrom (adress_acc, 7);	// read a number of byte and store them in wire.read (note: by nature, this is called an "auto-increment register adress")

#ifdef ORIGINAL

  byte REG_ADDRESS[7];
  short int accel[4];
  int i = 0;
 
  for (i = 0; i < 7; i++)	        // 7 because on datasheet p.19 if FREAD=0, on auto-increment, the adress is shifted
    // according to the datasheet, because it's shifted, outZlsb are in adress 0x00
    // so we start reading from 0x00, forget the 0x01 which is now "status" and make the adapation by ourselves
    // this gives:
    // 0 = status
    // 1= X_MSB
    // 2= X_LSB
    // 3= Y_MSB
    // 4= Y_LSB
    // 5= Z_MSB
    // 6= Z_LSB
    {
      REG_ADDRESS[i] = Wire.read ();	//each time you read the write.read it gives you the next byte stored. The counter is reset on requestFrom
    }


  // MMA8653FC gives the answer on 10bits. 8bits are on _MSB, and 2 are on _LSB
  // this part is used to concatenate both, and then put a sign on it (the most significant bit is giving the sign)
  // the explanations are on p.14 of the 'application notes' given by freescale.

  for (i = 1; i < 7; i = i + 2)
    {
      accel[0] = (REG_ADDRESS[i + 1] | ((short int) REG_ADDRESS[i] << 8)) >> 6;	// X
      
      if (accel[0] > 0x01FF)
	  //accel[1] = (((~accel[0]) + 1) - 0xFC00);			// note: with signed int, this code is optional
          // JZ method
          accel[1] = -1024 + accel[0];           // negative value
      else
	  accel[1] = accel[0];                   // positive value
			            
      switch (i)
	{
	case 1:
	  *axeXnow = accel[1];
	  break;
	case 3:
	  *axeYnow = accel[1];
	  break;
	case 5:
	  *axeZnow = accel[1];
	  break;
	}
    }
#else

// JZ cleaner, more portable method - return a value in milli-Gs

      Wire.read();   // discard first byte (status)

      // note:  2G is 32768, -2G is -32768

      *axeXnow = ((int16_t)((Wire.read() << 8) | Wire.read()) * 1000) / (32768/2);  // MSB first
      *axeYnow = ((int16_t)((Wire.read() << 8) | Wire.read()) * 1000) / (32768/2);  // MSB first
      *axeZnow = ((int16_t)((Wire.read() << 8) | Wire.read()) * 1000) / (32768/2);  // MSB first
 
#endif


}  // MMA8653FC_read()

#ifdef UNDEFINED
//------------------------------------------------------------------

// READ number data from i2c slave ctrl-reg register and return the result in a vector

void
I2C_READ_REG (int ctrlreg_address)	
{
  //unsigned char REG_ADDRESS;
  //int i = 0;
  Wire.beginTransmission (adress_acc);	//=ST + (Device Adress+W(0)) + wait for ACK
  Wire.write (ctrlreg_address);	        // register to read
  Wire.endTransmission ();
  Wire.requestFrom (adress_acc, 1);	// read a number of byte and store them in write received
}

#endif
