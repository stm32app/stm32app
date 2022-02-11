#ifndef INC_DEBUG
#define INC_DEBUG

#ifdef DEBUG

#define DEBUG_LOG_LEVEL 1

#undef configASSERT
#define configASSERT(x) if (!(x)) { debug_printf("Assert failed\n");  while (1) { } }

#define traceTASK_SWITCHED_OUT()  log_job_out()
#define traceTASK_SWITCHED_IN() log_job_in()

/* For program debug (e.g. printf the memory allocations and memory free) */
//#define traceMALLOC(pvAddress, uiSize)  debug_printf("%x %d malloc\r\n", pvAddress, (unsigned int)uiSize)
//#define traceFREE(pvAddress, uiSize)    debug_printf("%x %d free\r\n", pvAddress, (unsigned int)uiSize)

#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/dwt.h>
#include <stdio.h>

void vApplicationMallocFailedHook(void);
void vApplicationIdleHook( void );;

__attribute__((naked)) void my_hard_fault_handler(void);

__attribute__((used)) void hard_fault_handler_inside(struct scb_exception_stack_frame *frame);

#if DEBUG_LOG_LEVEL > 3
  #define _debug_printf printf
  #define debug_printf(macropar_message, ...) \
    (log_ccycnt_before(), \
    _debug_printf(macropar_message, ##__VA_ARGS__), \
    log_ccycnt_after())
#else
  #define _debug_printf(...) ((void)0)
  #define debug_printf _debug_printf
#endif

#if DEBUG_LOG_LEVEL > 0
  #define _log_printf printf
  #define log_printf(macropar_message, ...) \
    (log_ccycnt_before(), \
    _log_printf(macropar_message, ##__VA_ARGS__), \
    log_ccycnt_after())
#else
  #define _log_printf(...) ((void)0)
  #define log_printf _log_printf
#endif

#define error_printf debug_printf

void log_ccycnt_before(void);
void log_ccycnt_after(void);
void log_job_out(void);
void log_job_in(void);


#else

#define debug_printf printf
#define error_printf printf

#endif


#endif