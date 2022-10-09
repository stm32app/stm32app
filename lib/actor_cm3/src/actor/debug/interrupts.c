#include "interrupts.h"
#include <actor/types.h>
#include <libopencm3/cm3/scb.h>
#include <actor/log.h>

#pragma weak hard_fault_handler = my_hard_fault_handler

void hard_fault_handler_inside(void) {
    volatile uint32_t _CFSR;
    volatile uint32_t _HFSR;
    volatile uint32_t _DFSR;
    volatile uint32_t _AFSR;
    volatile uint32_t _BFAR;
    volatile uint32_t _MMAR;

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
