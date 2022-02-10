#ifndef INC_DEBUG
#define INC_DEBUG

#ifdef DEBUG

#undef configASSERT
#define configASSERT(x) if (!(x)) { log_printf("Assert failed\n");  while (1) { } }


#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/dwt.h>
#include <stdio.h>

void vApplicationMallocFailedHook(void);

__attribute__((naked)) void my_hard_fault_handler(void);

__attribute__((used)) void hard_fault_handler_inside(struct scb_exception_stack_frame *frame);


#define log_printf(macropar_message, ...) \
  log_ccycnt_before(); \
  printf(macropar_message, ##__VA_ARGS__); \
  log_ccycnt_after() \

#define error_printf log_printf

void log_ccycnt_before(void);
void log_ccycnt_after(void);
void log_task_out(void);
void log_task_in(void);


#else

#define log_printf printf
#define error_printf printf

#endif


#endif