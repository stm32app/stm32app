
#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef DEBUG
#include <stdio.h>
#pragma weak hard_fault_handler = my_hard_fault_handler

uint32_t lastcyccnt;
int32_t cycntoffset;
char line[160];

uint32_t lasttaskswitch;
uint32_t lasttasksoffset;

void log_ccycnt_before(void) {
  uint32_t cyccnt = DWT_CYCCNT;
  int32_t offset = (cyccnt - lastcyccnt - cycntoffset);
  uint32_t us = offset / (F_CPU / 1e6);
  if (us >= 1000) {
    if (us >= 100000) {
      printf("+%lums\t", us / 1000);
    } else {
      printf("+%lu.%2lums\t", us / 1000, us % 1000 / 10);
    }
  } else {
    printf("+%luus\t", us);
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
  printf("\n");
  log_printf("%s\n", pcTaskGetName(xTaskGetCurrentTaskHandle()));
}

void log_job_out(void) {
  uint32_t cyccnt = DWT_CYCCNT;
  int32_t offset = (cyccnt - lasttaskswitch - lasttasksoffset);
  uint32_t us = offset / (F_CPU / 1e6);
  log_printf("└ ");
  if (us >= 1000) {
    printf("%lu.%2lums\t", us / 1000, us % 1000 / 10);
  } else {
    printf("%luus\t", us);
  }
  printf("total \n");
}

void hard_fault_handler_inside(struct scb_exception_stack_frame *frame)
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


__attribute__((naked)) void my_hard_fault_handler(void)
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

#endif