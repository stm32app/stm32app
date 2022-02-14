#include "bytes.h"
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