#ifndef INC_CORE_ACTOR
#define INC_CORE_ACTOR

#ifdef __cplusplus
extern "C" {
#endif
#include "301/CO_ODinterface.h"
#include <actor/env.h>
#include <actor/node.h>

enum actor_phase {
    ACTOR_PHASE_ENABLED,
    ACTOR_PHASE_DISABLED,

    ACTOR_PHASE_CONSTRUCTION,
    ACTOR_PHASE_DESTRUCTION,

    ACTOR_PHASE_LINKAGE,

    ACTOR_PHASE_START,
    ACTOR_PHASE_CALIBRATION,
    ACTOR_PHASE_PREPARATION,
    ACTOR_PHASE_OPERATION,

    ACTOR_PHASE_WORK,
    ACTOR_PHASE_IDLE,

    ACTOR_PHASE_BUSY,
    ACTOR_PHASE_RESET,

    ACTOR_PHASE_PAUSE,
    ACTOR_PHASE_STOP,
};

struct actor {
    void *object;                   /* Pointer to the actor own struct */
    uint8_t seq;                    /* Sequence number of the actor in its family  */
    int16_t index;                  /* Actual OD address of this actor */
    actor_interface_t *interface;   /* Per-class methods and callbacks */
    actor_node_t *node;             /* Reference to root actor */
    OD_entry_t *entry;              /* OD entry containing propertiesuration for actor*/
    OD_extension_t entry_extension; /* OD IO handlers for properties changes */
    uint32_t event_subscriptions;   /* Mask for different event types that actor recieves */
};

struct actor_interface {
    uint16_t type;          /* OD index of a first actor of this type */
    uint16_t size;          /* Memory requirements for actor struct */
    uint8_t phase_subindex; /* OD subindex containing phase property*/
    uint8_t phase_offset;   /* OD subindex containing phase property*/

    actor_signal_t (*validate)(void *properties); /* Check if properties has all properties */

    actor_signal_t (*message_handled)(void *object, actor_message_t *message, actor_signal_t signal); /* Somebody processed the event */
    actor_signal_t (*phase_changed)(void *object, actor_phase_t phase);                                   /* Handle phase change */
    actor_signal_t (*signal_received)(void *object, actor_signal_t signal, actor_t *caller, void *arg);    /* Send signal to actor */
    actor_signal_t (*value_received)(void *object, actor_t *actor, void *value, void *arg);                /* Accept value from linked actor */
    actor_signal_t (*buffer_allocation)(void *object, actor_buffer_t *buffer, uint32_t size);     /* Accept linking request*/
    actor_worker_callback_t (*worker_assignment)(void *object, actor_thread_t *thread);           /* Assign worker handlers to threads */

    actor_signal_t (*property_before_change)(void *object, uint8_t index, void *value, void *old);
    actor_signal_t (*property_after_change)(void *object, uint8_t index, void *value, void *old);
    actor_signal_t (*property_get)(void *object, uint8_t index, void *value);
    ODR_t (*stream_read)(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *countRead);
    ODR_t (*stream_write)(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten);
};

struct actor_generic_device_t {
    actor_t *actor;
};

// Compute actor's own index
#define actor_index(actor) (actor->seq + actor->interface->type)
// Get struct containing actors OD values
#define actor_get_properties(actor) ((uint8_t *)((actor_node_t *)(actor)->object)->properties)
// Optimized getter for actor phase
#define actor_get_phase(actor) actor_get_properties(actor)[(actor)->interface->phase_offset]
// Optimized setter for actor phase (will not trigger observers)
#define actor_set_phase(actor, phase)                                                                                                      \
    actor_property_numeric_set(actor, (actor)->interface->phase_subindex, (uint32_t)phase, sizeof(actor_phase_t))
// Invokes actor-defined callback to handle phase change 
actor_signal_t actor_callback_phase_changed(actor_t *actor, actor_phase_t phase, actor_phase_t old);

/* Find actor with given index and write it to the given pointer */
int actor_link(actor_t *actor, void **destination, uint16_t index, void *arg);

/* Send value from actor to another */
int actor_send(actor_t *actor, actor_t *target, void *value, void *argument);
/* Sends 4-byte value in the pointer */
#define actor_send_numeric(actor, target, value, argument) actor_send(actor, target, (void *)value, argument);

/* Send signal from actor to another*/
int actor_signal(actor_t *actor, actor_signal_t signal, actor_t *caller, void *argument);

/* Allocate memory for the actor struct */
int actor_object_allocate(actor_t *actor);
/* Free actor struct memory */
int actor_object_free(actor_t *actor);

/* Check if event will invoke input tick on this actor */
void actor_message_subscribe(actor_t *actor, actor_message_type_t type);

/* Assert that actor has given type, and return it */
static inline actor_t *actor_of_type(actor_t *actor, uint16_t index) {
    actor_assert(actor->interface->type == index);
    return actor;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif