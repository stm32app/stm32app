#ifndef INC_CORE_ACTOR
#define INC_CORE_ACTOR

#ifdef __cplusplus
extern "C" {
#endif

#include "301/CO_ODinterface.h"
#include <actor/node.h>
#include <actor/env.h>
#include <actor/thread.h>

enum actor_phase {
    ACTOR_ENABLED,
    ACTOR_CONSTRUCTING,
    ACTOR_LINKING,

    ACTOR_STARTING,
    ACTOR_CALIBRATING,
    ACTOR_PREPARING,
    ACTOR_RUNNING,

    ACTOR_REQUESTING,
    ACTOR_RESPONDING,

    ACTOR_WORKING,
    ACTOR_IDLE,

    ACTOR_BUSY,
    ACTOR_RESETTING,

    ACTOR_PAUSING,
    ACTOR_PAUSED,
    ACTOR_RESUMING,

    ACTOR_STOPPING,
    ACTOR_STOPPED,

    ACTOR_DISABLED,
    ACTOR_DESTRUCTING
};

struct actor {
    void *object;                   /* Pointer to the actor own struct */
    uint8_t seq;                    /* Sequence number of the actor in its family  */
    int16_t index;                  /* Actual OD address of this actor */
    actor_class_t *class;           /* Per-class methods and callbacks */
    actor_node_t *node;                     /* Reference to root actor */
    OD_entry_t *entry;              /* OD entry containing propertiesuration for actor*/
    OD_extension_t entry_extension; /* OD IO handlers for properties changes */
    uint32_t event_subscriptions;   /* Mask for different event types that actor recieves */
#if DEBUG
    actor_phase_t previous_phase;
#endif
};

struct actor_class {
    uint16_t type;          /* OD index of a first actor of this type */
    uint8_t phase_subindex; /* OD subindex containing phase property*/
    uint8_t phase_offset;   /* OD subindex containing phase property*/
    size_t size;            /* Memory requirements for actor struct */

    actor_signal_t (*validate)(void *properties); /* Check if properties has all properties */
    actor_signal_t (*construct)(void *object);    /* Initialize actor at given pointer*/
    actor_signal_t (*link)(void *object);         /* Link related actors together*/
    actor_signal_t (*destruct)(void *object);     /* Destruct actor at given pointer*/
    actor_signal_t (*start)(void *object);        /* Prepare periphery and run initial logic */
    actor_signal_t (*stop)(void *object);         /* Reset periphery and deinitialize */
    actor_signal_t (*pause)(void *object);        /* Put actor to sleep temporarily */
    actor_signal_t (*resume)(void *object);       /* Wake actor up from sleep */

    actor_signal_t (*on_job_complete)(void *object, actor_job_t *job);                            /* Task has been complete */
    actor_signal_t (*on_event_report)(void *object, actor_event_t *event);                        /* Somebody processed the event */
    actor_signal_t (*on_phase)(void *object, actor_phase_t phase);                              /* Handle phase change */
    actor_signal_t (*on_signal)(void *object, actor_t *origin, actor_signal_t signal, void *arg); /* Send signal to actor */
    actor_signal_t (*on_value)(void *object, actor_t *actor, void *value, void *arg);           /* Accept value from linked actor */
    actor_signal_t (*on_link)(void *object, actor_t *origin, void *arg);                        /* Accept linking request*/
    actor_signal_t (*on_buffer_allocation)(void *object, actor_buffer_t *buffer, uint32_t size);  /* Accept linking request*/
    actor_worker_callback_t (*on_worker_assignment)(void *object, actor_thread_t *thread);              /* Assign worker handlers to threads */

    ODR_t (*property_read)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*property_write)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
    uint32_t property_read_handlers;  // bit masks of properties that have custom reader logic
    uint32_t property_write_handlers; // bit mask of properties that have custom writer logic
};

struct actor_generic_device_t {
    actor_t *actor;
};

// Faster versions of OD_set_value that has assumptions:
// - Value is not streamed
// - Address is record
// - There're no gaps in record definition
// - If value hasnt changed, there is no need to run setters
ODR_t actor_set_property(actor_t *actor, uint8_t index, void *value, size_t size);
// Copy number by value, still needs size in bytes to be given (runs setters)
ODR_t actor_set_property_numeric(actor_t *actor, uint8_t index, uint32_t value, size_t size);
// Write value with trailing NULL character
ODR_t actor_set_property_string(actor_t *actor, uint8_t index, char *data, size_t size);
// Run getters to stream property (will initialize stream if needed)
ODR_t actor_compute_property_stream(actor_t *actor, uint8_t index, uint8_t *data, OD_size_t size, OD_stream_t *stream,
                                    OD_size_t *count_read);
// Run getters to compute property and store it in given buffer (or fall back to internal memory)
ODR_t actor_compute_property_into_buffer(actor_t *actor, uint8_t index, uint8_t *data, OD_size_t size);
// Run getters for the property
ODR_t actor_compute_property(actor_t *actor, uint8_t index);
// Return pointer to value (runs getters)
void *actor_get_property_pointer(actor_t *actor, uint8_t index);

#define actor_class_property_mask(class, property) (property - class->phase_subindex + 1)

// Compute actor's own index
#define actor_index(actor) (actor->seq + actor->class->type)
// Get struct containing actors OD values
#define actor_get_properties(actor) ((uint8_t *)((actor_node_t *)actor->object)->properties)
// Optimized getter for actor phase
#define actor_get_phase(actor) actor_get_properties(actor)[actor->class->phase_offset]
// Optimized setter for actor phase (will not trigger observers)
#define actor_set_phase(actor, phase)                                                                                                      \
    actor_set_property_numeric(actor, (actor)->class->phase_subindex, (uint32_t)phase, sizeof(actor_phase_t))
// Default state machine for basic phases
void actor_on_phase_change(actor_t *actor, actor_phase_t phase);

int actor_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick);

/* Find actor with given index and write it to the given pointer */
int actor_link(actor_t *actor, void **destination, uint16_t index, void *arg);

/* Send value from actor to another */
int actor_send(actor_t *actor, actor_t *target, void *value, void *argument);

/* Send signal from actor to another*/
int actor_signal(actor_t *actor, actor_t *origin, actor_signal_t value, void *argument);

int actor_object_allocate(actor_t *actor);
int actor_object_free(actor_t *actor);

/* Check if event will invoke input tick on this actor */
bool actor_event_is_subscribed(actor_t *actor, actor_event_t *event);
void actor_event_subscribe(actor_t *actor, actor_event_type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif