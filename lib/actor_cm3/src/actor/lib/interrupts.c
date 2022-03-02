#include "interrupts.h"
#define actor_disable_interrupts() cm_disable_interrupts();
#define actor_enable_interrupts() cm_enable_interrupts();