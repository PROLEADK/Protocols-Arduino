// Serial routines that can read/write to either Serial device (Serial and Serial1) based on a setting

// to get Serial ports
#include <Arduino.h>



static int Serial_Port = 1;   // 1 == Serial, 2 == Serial1, 3 = both



void Set_Serial(int s)
{
   Serial_Port = s;
}

// Do a printf to either port

void Serial_printf(const char * format, ... )
{
  char string[200];          // Warning: fixed buffer size
  va_list v_List;
  va_start( v_List, format );
  vsnprintf( string, sizeof(string), format, v_List );
  if (Serial_Port & 1)
     Serial.print(string);
  if (Serial_Port & 2)
     Serial1.print(string);
  va_end( v_List );
}


unsigned char Serial_read()
{
  for (;;) {
    if ((Serial_Port & 1) && Serial.available())
      return Serial.read();
    if ((Serial_Port & 2) && Serial1.available())
      return Serial1.read();
  } // for
}  // Serial_Read()


unsigned char Serial_available()
{
    if ((Serial_Port & 1) && Serial.available())
      return 1;
    if ((Serial_Port & 2) && Serial1.available())
      return 1;
    return 0;
}  // Serial_Available()
