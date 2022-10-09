#ifndef INC_ACTOR_MOTHERSHIP
#define INC_ACTOR_MOTHERSHIP

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/node.h>
#include <actor/thread.h>
#include <actor/buffer.h>
/* Start of autogenerated OD types */
/* 0x3000: Actor Mothership
   Configuration of global object */
typedef struct actor_mothership_properties {
    uint8_t parameter_count;
    uint32_t timer_index; // Index of a timer used for generic medium-precision scheduling (1us) 
    uint32_t storage_index;
    uint32_t mcu_index; // Main MCU device index 
    uint32_t canopen_index; // Main CANopen device 
    uint8_t phase;
} actor_mothership_properties_t;
/* End of autogenerated OD types */


struct actor_mothership {
    actor_t *actor;
    actor_mothership_properties_t *properties;
    size_t actor_count;
    OD_t *dictionary;
    
    actor_buffer_t *buffers;         /* Pool of buffers available for taking */
    actor_buffer_t *jobs;            /* Pool of reusable jobs*/
    actor_buffer_t *mutexes;         /* Pool of reusable mutexes */

    actor_thread_t *input;           /* Thread with queue of events that need immediate response*/
    actor_thread_t *catchup;         /* Allow actors that were busy to catch up with input events  */
    actor_thread_t *high_priority;   /* Logic that is scheduled by actors themselves */
    actor_thread_t *medium_priority; /* A queue of events that concerns medium_priorityting  outside */
    actor_thread_t *low_priority;    /* Logic that runs periodically that is not very important */
    actor_thread_t *bg_priority;     /* A background thread of sorts for work that can be done in free time */

    actor_t *mcu;
    actor_t *canopen;
    actor_t *database;
    actor_t *timer;


    actor_job_t *job;
    bool_t initialized;
    bool_t sdram;
};


extern actor_interface_t actor_mothership_class;

size_t actor_mothership_enumerate_actors(actor_node_t *node, OD_t *od, actor_t *destination);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif