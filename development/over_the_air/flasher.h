
#include <stdint.h>

#include <kinetis.h>

#if defined(__MK20DX128__)
#define FLASH_SIZE              0x20000
#define FLASH_SECTOR_SIZE       0x400
#define FLASH_ID                "fw_teensy3.0"
#elif defined(__MK20DX256__)
#define FLASH_SIZE              0x40000
#define FLASH_SECTOR_SIZE       0x800
#define FLASH_ID                "fw_teensy3.1"
#else
#error NOT SUPPORTED
#endif


void upgrade_firmware(void);
void boot_check(void);

static int flash_hex_line(const char *line);
int parse_hex_line(const char *theline, uint8_t bytes[], unsigned int *addr, unsigned int *num, unsigned int *code);
static int flash_block(uint32_t address, uint32_t *bytes, int count);

// apparently better - thanks to Frank Boesing
#define RAMFUNC  __attribute__ ((section(".fastrun"), noinline, noclone, optimize("Os") ))

#if 0
extern unsigned long _etext;
extern unsigned long _sdata;
extern unsigned long _edata;
extern unsigned long _eflash;
extern unsigned long _sdtext;
extern unsigned long _edtext;
extern unsigned long _sbss;
extern unsigned long _ebss;
extern unsigned long _estack;
#define FLASH_START  ((uint32_t)_etext + ((uint32_t)_edata - (uint32_t)_sdata))
#endif

#define RESERVE_FLASH (8 * FLASH_SECTOR_SIZE)

