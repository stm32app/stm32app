#include <actor/lib/bytes.h>
#include "stdint.h"

uint16_t swap_bytes_ba(uint16_t AB) {
    return (AB>>8)|(AB<<8);
}
uint16_t swap_bytes_ab(uint16_t AB) {
    return AB;
}
uint32_t swap_bytes_abcd(uint32_t ABCD) {
    return ABCD;
}
uint32_t swap_bytes_badc(uint32_t ABCD) {
    return ((ABCD>>8)&0xff0000) | ((ABCD<<8)&0xff000000) | ((ABCD<<8)&0xff00) | ((ABCD>>8)&0xff);
}
uint32_t swap_bytes_cdab(uint32_t ABCD) {
    return ((ABCD>>16)&0xff00) | ((ABCD>>16)&0xff) | ((ABCD<<16)&0xff000000) | ((ABCD<<16)&0xff0000);
}
uint32_t swap_bytes_dcba(uint32_t ABCD) {
    return ((ABCD>>24)&0xff) | ((ABCD<<8)&0xff0000) | ((ABCD>>8)&0xff00) | ((ABCD<<24)&0xff000000);
}

uint8_t get_maximum_byte_alignment(uint32_t address, uint8_t limit) {
    while (limit) {
        if (limit == 1 || address % limit == 0) {
            return limit;
        } else {
            limit /= 2;
        }
    }
    return 1;
}

#define min(a, b) (a > b ? b : a)
#define max(a, b) (a < b ? b : a)

uint32_t get_number_of_bytes_intesecting_page(uint32_t address, uint32_t size, uint32_t page_size) {
    uint32_t page_start_offset = address % page_size;
    uint32_t last_byte_in_range = address + size;
    uint32_t last_byte_on_page = address - page_start_offset + page_size;
    return min(last_byte_on_page, last_byte_in_range) - address;
}



// Software CRC implementation with small lookup table
uint32_t lfs_crc(uint32_t crc, const void *buffer, size_t size) {
    static const uint32_t rtable[16] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
        0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
        0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
        0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c,
    };

    const uint8_t *data = buffer;

    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 0)) & 0xf];
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 4)) & 0xf];
    }

    return crc;
}
