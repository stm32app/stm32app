
#include "debug.h"
#include <FreeRTOS.h>
#include "task.h"

volatile uint8_t debug_log_inhibited = false;

#ifdef DEBUG
#include <stdio.h>
#pragma weak hard_fault_handler = my_hard_fault_handler

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
    log_printf("%s\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
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

void hard_fault_handler_inside(struct scb_exception_stack_frame *frame) {
    volatile uint32_t _CFSR;
    volatile uint32_t _HFSR;
    volatile uint32_t _DFSR;
    volatile uint32_t _AFSR;
    volatile uint32_t _BFAR;
    volatile uint32_t _MMAR;

    (void)(frame);
    /*
     * - Configurable Fault Status Register, MMSR, BFSR and UFSR
     * - Hard Fault Status Register
     * - Debug Fault Status Register
     * - Auxiliary Fault Status Register
     */
    _CFSR = SCB_CFSR;
    _HFSR = SCB_HFSR;
    _DFSR = SCB_DFSR;
    _AFSR = SCB_AFSR;

    /*
     * Fault address registers
     * - MemManage Fault Address Register
     * - Bus Fault Address Register
     */
    if (_CFSR & SCB_CFSR_MMARVALID)
        _MMAR = SCB_MMFAR;
    else
        _MMAR = 0;

    if (_CFSR & SCB_CFSR_BFARVALID)
        _BFAR = SCB_BFAR;
    else
        _BFAR = 0;

    (void)(_CFSR);
    (void)(_HFSR);
    (void)(_DFSR);
    (void)(_AFSR);
    (void)(_BFAR);
    (void)(_MMAR);


    buffered_printf_flush();
    __asm("BKPT #0\n"); // Break into the debuggezr
    while (1) {
    }
};

__attribute__((naked)) void my_hard_fault_handler(void) {
    __asm volatile("movs r0,#4            \n"
                   "movs r1, lr           \n"
                   "tst r0, r1            \n"
                   "beq _MSP              \n"
                   "mrs r0, psp           \n"
                   "b _HALT               \n"
                   "_MSP:                   \n"
                   "mrs r0, msp           \n"
                   "_HALT:                  \n"
                   "ldr r1,[r0,#20]       \n"
                   "b hard_fault_handler_inside \n"
                   "bkpt #0               \n");
};

/*
void mem_manage_handler(void)
{
    while (1);
}
void usage_fault_handler(void)
{
    while (1);
}
void bus_fault_handler(void)
{
    while (1);
}*/

void vApplicationIdleHook(void) {
    //printf("Idle!\n");
    buffered_printf_flush();
}
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;      /* unused*/
    (void)pcTaskName; /* may be unused*/
    printf("System - Stack overflow! %s", pcTaskName);
    buffered_printf_flush();
    while (1) {
        __asm("BKPT #0\n"); // Break into the debugger
    }
}

void vApplicationMallocFailedHook(void) {
    printf("System - Malloc failed!");
    buffered_printf_flush();
    while (1) {
        __asm("BKPT #0\n"); // Break into the debugger
    }
}


#endif
