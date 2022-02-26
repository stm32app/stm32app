#ifdef APP_MOTHERSHIP
    #include "app/mothership.h"
    #include "OD.h"
#endif

extern void initialise_monitor_handles(void);

static void app_boot(void *pvParameters) {
    
    app_t **app = (app_t **) pvParameters;
#if APP_MOTHERSHIP
    debug_printf("App - Mothership ...\n");
    debug_printf("App - Enumerating actors ...\n");
    app_allocate(app, OD, app_mothership_enumerate_actors);
#endif
    log_printf("App - Constructing...\n");
    app_set_phase(*app, ACTOR_CONSTRUCTING);

    log_printf("App - Linking...\n");
    app_set_phase(*app, ACTOR_LINKING);

    log_printf("App - Starting...\n");
    app_set_phase(*app, ACTOR_STARTING);
    
    vTaskDelete(NULL);
}



int main(void) {
    debug_log_inhibited = false;
    
#ifdef SEMIHOSTING
    initialise_monitor_handles();
    dwt_enable_cycle_counter();
#endif
    app_t *app;
    scb_set_priority_grouping(SCB_AIRCR_PRIGROUP_GROUP16_NOSUB);
    xTaskCreate( app_boot, "Startup", 5000, &app, tskIDLE_PRIORITY + 7, NULL);

    debug_printf("App - Starting tasks...\n");
    
    vTaskStartScheduler();
    while (true) { }
    return 0;
}
