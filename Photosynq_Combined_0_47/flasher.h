
#include <kinetis.h>
#include <Arduino.h>
#include "serial.h"

#if defined(__MK20DX128__)
#define FLASH_SIZE              0x20000
#define FLASH_SECTOR_SIZE       0x400
#define FLASH_ID                "fw_teensy3.0"
#define RESERVE_FLASH (2 * FLASH_SECTOR_SIZE)
#elif defined(__MK20DX256__)
#define FLASH_SIZE              0x40000
#define FLASH_SECTOR_SIZE       0x800
#define FLASH_ID                "fw_teensy3.1"
#define RESERVE_FLASH (1 * FLASH_SECTOR_SIZE)
#else
#error NOT SUPPORTED
#endif


void upgrade_firmware(void);
void boot_check(void);

int parse_hex_line (const char *theline, uint8_t bytes[], uint32_t * addr, unsigned int *num,unsigned int *code);
                
static int flash_hex_line(const char *line);
//int parse_hex_line(const char *theline, uint8_t bytes[], unsigned int *addr, unsigned int *num, unsigned int *code);
static int flash_block(uint32_t address, uint32_t *bytes, int count);
void flash_erase_upper(void);
static int flash_sector_erased(uint32_t address);
static void flash_move (uint32_t min_address, uint32_t max_address);
static int flash_erase_sector (uint32_t address, int unsafe);
static int flash_word (uint32_t address, uint32_t word_value);
static int check_compatible(uint32_t min, uint32_t max);
//void delay (int x);


#define RAMFUNC  __attribute__ ((section(".fastrun"), noinline, noclone, optimize("Os") ))



