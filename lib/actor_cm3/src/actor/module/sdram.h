#ifndef INC_SDRAM
#define INC_SDRAM


#define SDRAM_BASE_ADDRESS ((uint8_t *)(0xd0000000))
#define SDRAM_SIZE (1024 * 1024)

void start_sdram(void);
void finish_sdram(void);


#endif