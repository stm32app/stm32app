
#include "log.h"
volatile int debug_log_inhibited = 0;

#ifdef DEBUG
#include <stdio.h>

uint32_t lastcyccnt;
int32_t cycntoffset;
uint32_t lasttaskswitch;
uint32_t lasttasksoffset;


#if DEBUG_LOG_LEVEL > 0 && DEBUG_BUFFER_SIZE > 0 
char debug_buffer[DEBUG_BUFFER_SIZE] = {};
int debug_buffer_position = 0;

void conditional_printf_flush(void) {

    printf("\n, %.*s", debug_buffer_position, debug_buffer_position, debug_buffer);
    debug_buffer_position = 0;
    return 0;
}
#endif

void log_ccycnt_before(void) {
    uint32_t cyccnt = DWT_CYCCNT;
    int32_t offset = (cyccnt - lastcyccnt - cycntoffset);
    uint32_t us = offset / (F_CPU / 1e6);
    if (us >= 1000) {
        if (us >= 100000) {
            conditional_printf("+%lums\t", us / 1000);
        } else {
            conditional_printf("+%lu.%02lums\t", us / 1000, us % 1000 / 10);
        }
    } else {
        conditional_printf("+%luus\t", us);
    }
    lastcyccnt = cyccnt;
}

void log_ccycnt_after(void) {
    cycntoffset = (DWT_CYCCNT - lastcyccnt);
    lasttasksoffset += cycntoffset;
}

void log_job_in(void) {
    lasttaskswitch = DWT_CYCCNT;
    lasttasksoffset = 0;
    conditional_printf("\n");
    debug_printf("%s\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
}

void log_job_out(void) {
    uint32_t cyccnt = DWT_CYCCNT;
    int32_t offset = (cyccnt - lasttaskswitch - lasttasksoffset);
    uint32_t us = offset / (F_CPU / 1e6);
    conditional_printf("└ ");
    if (us >= 1000) {
        conditional_printf("%lu.%02lums\t", us / 1000, us % 1000 / 10);
    } else {
        conditional_printf("%luus\t", us);
    }
    conditional_printf("total \n");
}


#endif
