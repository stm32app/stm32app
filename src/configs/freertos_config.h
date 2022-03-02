#ifndef INC_FREERTOS_CONFIG
#define INC_FREERTOS_CONFIG


#ifdef DEBUG
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1
#define configUSE_IDLE_HOOK 1

#define configASSERT actor_assert

#define traceTASK_SWITCHED_OUT() log_job_out()
#define traceTASK_SWITCHED_IN() log_job_in()

#define INCLUDE_uxTaskGetStackHighWaterMark 1
#else
#define configUSE_IDLE_HOOK 0
#endif

#if FREERTOS_PORT_CM3
#define vPortSVCHandler sv_call_handler
#define xPortPendSVHandler pend_sv_handler
#define xPortSysTickHandler sys_tick_handler
#define configCPU_CLOCK_HZ F_CPU
#define configSYSTICK_CLOCK_HZ (configCPU_CLOCK_HZ / 8) /* fix for vTaskDelay() */
#define configPRIO_BITS 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 0xf
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
/* This is the value being used as per the ST library which permits 16
priority values, 0 to 15.  This must correspond to the
configKERNEL_INTERRUPT_PRIORITY setting.  Here 15 corresponds to the lowest
NVIC value of 255. */
/* This is the raw value as per the Cortex-M3 NVIC.  Values can be 255
(lowest) to 0 (1?) (highest). */
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#else

#endif

#endif