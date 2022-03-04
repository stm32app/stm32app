#ifndef INC_DEBUG_HOOKS
#define INC_DEBUG_HOOKS

#if DEBUG
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
#endif

#endif