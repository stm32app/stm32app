#ifndef INC_DEBUG_INTERRUPTS
#define INC_DEBUG_INTERRUPTS

__attribute__((naked)) void my_hard_fault_handler(void);
__attribute__((used)) void hard_fault_handler_inside(void);

#endif