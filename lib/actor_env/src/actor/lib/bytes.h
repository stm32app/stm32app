#ifndef LIB_BYTES
#define LIB_BYTES


#include <stdint.h>
#include <stddef.h>


uint32_t get_number_of_bytes_intesecting_page(uint32_t address, uint32_t size, uint32_t page_size) ;

// find number of bytes address is aligned to (up to given limit)
uint8_t get_maximum_byte_alignment(uint32_t address, uint8_t limit);


#ifdef __GNUC__
#define bitswap32 __builtin_bswap32
#define bitswap16 __builtin_bswap16
#else
#define bitswap32 __REV
#endif

uint16_t swap_bytes_ba(uint16_t AB);
uint16_t swap_bytes_ab(uint16_t AB);
uint32_t swap_bytes_abcd(uint32_t ABCD);
uint32_t swap_bytes_badc(uint32_t ABCD);
uint32_t swap_bytes_cdab(uint32_t ABCD);
uint32_t swap_bytes_dcba(uint32_t ABCD);

uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size);

#endif