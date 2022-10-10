#ifndef INC_DEBUG_LOG
#define INC_DEBUG_LOG

#include <actor/config.h>


#ifndef actor_assert
#define actor_assert(x)                                                                                                                      \
    if (!(x)) {                                                                                                                            \
        debug_printf("Assert failed\n");                                                                                                   \
        while (1) {                                                                                                                        \
        }                                                                                                                                  \
    }
#endif

#if DEBUG_LOG_LEVEL > 0
#include <stdio.h>
#endif



//#define IS_DEBUGGER_ATTACHED (DBGMCU_CR & 0x07)
#define DEBUG_BUFFER_SIZE 0
#define DEBUG_BUFFER_FLUSH_SIZE DEBUG_BUFFER_SIZE * 0.9

#define DEBUG_NOOP(...) ((void)0)

void buffered_printf_flush(void);

extern volatile int debug_log_inhibited;

#if DEBUG_LOG_LEVEL > 0
#if DEBUG_BUFFER_SIZE > 0
extern char debug_buffer[DEBUG_BUFFER_SIZE];
extern int debug_buffer_position;


#define buffered_printf(...)                                                                                                               \
    (debug_buffer_position += sprintf(&debug_buffer[debug_buffer_position], __VA_ARGS__),                                                  \
     (debug_buffer_position >= DEBUG_BUFFER_FLUSH_SIZE ? buffered_printf_flush() : DEBUG_NOOP()))
#else
#define buffered_printf printf
#define buffered_printf_flush DEBUG_NOOP
#endif
#else
#define buffered_printf DEBUG_NOOP
#define buffered_printf_flush DEBUG_NOOP
#endif

#if DEBUG_LOG_LEVEL > 0
#define conditional_printf(...) (!debug_log_inhibited ? buffered_printf(__VA_ARGS__) : (void) 0)
#define debug_timestamp_printf(...) (log_ccycnt_before(), conditional_printf(__VA_ARGS__), log_ccycnt_after())
#else
#define conditional_printf DEBUG_NOOP
#define debug_timestamp_printf DEBUG_NOOP
#endif 

#if 0 && DEBUG_LOG_LEVEL > 3
#define _trace_printf conditional_printf
#define trace_printf debug_timestamp_printf
#else
#define _trace_printf DEBUG_NOOP
#define trace_printf DEBUG_NOOP
#endif
#if DEBUG_LOG_LEVEL > 2
#define _debug_printf conditional_printf
#define debug_printf debug_timestamp_printf
#else
#define _debug_printf DEBUG_NOOP
#define debug_printf DEBUG_NOOP
#endif

#if DEBUG_LOG_LEVEL > 0
#define _log_printf conditional_printf
#define log_printf debug_timestamp_printf
#else
#define _log_printf DEBUG_NOOP
#define log_printf DEBUG_NOOP
#endif

#define error_printf debug_printf



#ifdef DEBUG
#define traceTASK_SWITCHED_OUT() log_job_out()
#define traceTASK_SWITCHED_IN() log_job_in(pcTaskGetName(xTaskGetCurrentTaskHandle()))

/* For program debug (e.g. printf the memory allocations and memory free) */
// #define traceMALLOC(pvAddress, uiSize)  debug_printf("%p %d malloc\r\n", pvAddress, (unsigned int)uiSize)
// #define traceFREE(pvAddress, uiSize)    debug_printf("%p %d free\r\n", pvAddress, (unsigned int)uiSize)

void log_ccycnt_before(void);
void log_ccycnt_after(void);
void log_job_out(void);

void log_job_in(char *);


#endif

#endif