#include "actor/thread.h"
#include "interrupts.h"


bool actor_thread_is_interrupted(actor_thread_t *thread) {
  return (SCB_ICSR & SCB_ICSR_VECTACTIVE);
} 

void actor_disable_interrupts(void) {
  cm_disable_interrupts();
}

void actor_enable_interrupts(void) {
  cm_enable_interrupts();
}
