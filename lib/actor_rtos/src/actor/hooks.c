#include "hooks.h"
#include <actor/log.h>
#include "FreeRTOS.h"
#include "task.h"

void vApplicationIdleHook(void) {
    //printf("Idle!\n");
    buffered_printf_flush();
}

#if configCHECK_FOR_STACK_OVERFLOW > 0
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask; /* may be unused*/
    (void)pcTaskName; /* may be unused*/
    debug_printf("System - Stack overflow! %s", pcTaskName);
    buffered_printf_flush();
    while (1) {
        __asm("BKPT #0\n"); // Break into the debugger
    }
}
#endif

void vApplicationMallocFailedHook(void) {
    debug_printf("System - Malloc failed!");
    buffered_printf_flush();
    while (1) {
        __asm("BKPT #0\n"); // Break into the debugger
    }
}