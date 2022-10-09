#include <libopencm3/cm3/scb.h>
#include <actor/debug/platform.h>
#include "platform.h"


extern void initialise_monitor_handles(void);

void actor_platform_init(void) {
  scb_set_priority_grouping(SCB_AIRCR_PRIGROUP_GROUP16_NOSUB);
  #ifdef SEMIHOSTING
      initialise_monitor_handles();
      dwt_enable_cycle_counter();
  #endif
}