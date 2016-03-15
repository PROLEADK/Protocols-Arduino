
#include <Arduino.h>

// to get Serial ports
#include <HardwareSerial.h>
#include <WProgram.h>


char * int32_to_hex (uint32_t i);
void crc32_init ();
uint32_t crc32_value (void);
void crc32_byte (const uint32_t byte);
void crc32_buf (const char * message, int size);
void crc32_string (const char *message);
void Serial_Print (const char *str);
void Serial_Print_Float (float x, int precision);
void Serial_Print_Line (const char *str);
void Serial_Print_Float_Line (float x, int precision);
void Serial_Print_CRC (void);
