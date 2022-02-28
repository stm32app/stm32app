#ifdef ACTOR_MOTHERSHIP
    #include "node/mothership.h"
    #include "OD.h"
#endif

extern void initialise_monitor_handles(void);

static void actor_boot(void *pvParameters) {
    
    actor_node_t **node = (actor_node_t **) pvParameters;
#if ACTOR_MOTHERSHIP
    debug_printf("App - Mothership ...\n");
    debug_printf("App - Enumerating actors ...\n");
    actor_node_allocate(node, OD, actor_mothership_enumerate_actors);
#endif
    log_printf("App - Constructing...\n");
    actor_set_phase(*node, ACTOR_CONSTRUCTING);

    log_printf("App - Linking...\n");
    actor_set_phase(*node, ACTOR_LINKING);

    log_printf("App - Starting...\n");
    actor_set_phase(*node, ACTOR_STARTING);
    
    vTaskDelete(NULL);
}



int main(void) {
    debug_log_inhibited = false;
    
#ifdef SEMIHOSTING
    initialise_monitor_handles();
    dwt_enable_cycle_counter();
#endif
    actor_node_t *node;
    scb_set_priority_grouping(SCB_AIRCR_PRIGROUP_GROUP16_NOSUB);
    xTaskCreate( actor_boot, "Startup", 5000, &node, tskIDLE_PRIORITY + 7, NULL);

    debug_printf("App - Starting tasks...\n");
    
    vTaskStartScheduler();
    while (true) { }
    return 0;
}
