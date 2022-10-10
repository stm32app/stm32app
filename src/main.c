
#include <actor/env.h>
#include <actor/debug/platform.h>
#include <actor/lib/platform.h>
#include <actor/module/sdram.h>
#include "301/CO_ODinterface.h"
#include <OD.h>

#ifdef ACTOR_NODE_MOTHERSHIP 
    #include "mothership.h"
#endif

static void actor_boot(void *pvParameters) {
    
    actor_node_t **node = (actor_node_t **) pvParameters;
#if ACTOR_NODE_MOTHERSHIP 
    debug_printf("App - Enumerating actors for Mothership ...\n");
    actor_node_allocate(node, OD, actor_mothership_enumerate_actors);
#endif
    debug_printf("App - Constructing...\n");
    actor_node_actors_set_phase(*node, ACTOR_PHASE_CONSTRUCTION);

    debug_printf("App - Linking...\n");
    actor_node_actors_set_phase(*node, ACTOR_PHASE_LINKAGE);

    debug_printf("App - Starting...\n");
    actor_node_actors_set_phase(*node, ACTOR_PHASE_START);
    
    vTaskDelete(NULL);
}



int main(void) {
    start_sdram();
    actor_node_t *node;

    debug_log_inhibited = false;
    actor_platform_init();
    xTaskCreate( actor_boot, "Startup", 5000, &node, tskIDLE_PRIORITY + 7, NULL);

    debug_printf("App - Starting tasks...\n");
    
    vTaskStartScheduler();
    while (true) { }
    return 0;
}
