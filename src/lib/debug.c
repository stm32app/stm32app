
#include "debug.h"

#ifdef DEBUG
__attribute__((naked)) void hard_fault_handler(void)
{
  __asm volatile (
    "movs r0,#4            \n"
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
    "bkpt #0               \n"
  );
};


__attribute__((used)) void hard_fault_handler_inside(struct scb_exception_stack_frame *frame)
{
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


    (void) (_CFSR);
    (void) (_HFSR);
    (void) (_DFSR);
    (void) (_AFSR);
    (void) (_BFAR);
    (void) (_MMAR);

    __asm("BKPT #0\n") ; // Break into the debugger
    while (1) { }
};


void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;      /* unused*/
    (void)pcTaskName; /* may be unused*/
    log_printf("System - Stack overflow! %s", pcTaskName);
    while (1) {
    __asm("BKPT #0\n") ; // Break into the debugger
    }
}

void vApplicationMallocFailedHook(void) {
    log_printf("System - Malloc failed!");
    while (1) {
    __asm("BKPT #0\n") ; // Break into the debugger
    }
}

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
}

#endif