#ifndef INC_ACTOR_ACCESSORS
#define INC_ACTOR_ACCESSORS

#include "301/CO_ODinterface.h"
#include "actor/types.h"
#include "actor/message.h"


/*
Version of `actor_property_set` that is fully compatible with OD writers, so it is used used
to receieve external writes from the CANopen to actor nodes. It retains behavioral changes
of actor_property_set, but allows values to be streamed in chunks. It converts Actor
signals to ODR return values for compatability with OD interface, so Actor functions 
that use it (like `actor_property_stream_set`) directly do a backward conversion
*/
ODR_t actor_property_writer(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *count_written);
// Handle external reads to OD via CAN
ODR_t actor_property_reader(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *count_read);

/* # Set discrete OD value bypassing OD interface streaming functions. 
This is optimized version of CANopenNode's own setters meant to be used as a base functions changing observable state. Main behavioral differences are: 
- Will fast-track setters that do not change a value. It may make it slightly less usable to enumate commands over CAN, since these invocations will not be visible on the CAN network.
- Streaming is not available, so whole value needs to be sent in one chunk. Streaming however is still available via separate `actor_property_stream_set()` function
- Old value is stored in buffer shared by all nodes, potentially making it prone to thread unsafety. Similarily to CANOpenNode interrupt guards may need to be used to ensure safety. 
- CANOpenNode's own change listeners will not be invoked, so only Actor subscription mechanisms will  be notified of changes.
- It only works with record-type (no arrays or primitive values) and that there are no gaps indexes of record property definition
*/
actor_signal_t actor_property_set(actor_t *actor, uint8_t index, void *value, size_t size);
// Write property partially by streaming it
actor_signal_t actor_property_stream_set(actor_t *actor, uint8_t index, void *value, size_t size, uint32_t *count_written);
// Copy number by value, still needs size in bytes to be given (runs setters)
actor_signal_t actor_property_numeric_set(actor_t *actor, uint8_t index, uint32_t value, size_t size);
// Write value with trailing NULL character
actor_signal_t actor_property_string_set(actor_t *actor, uint8_t index, char *data, size_t size);

// Read value partially
actor_signal_t actor_property_stream_get(actor_t *actor, uint8_t index, void *data, size_t size, uint32_t *count_read);
// Return pointer to value (runs getters)
void *actor_property_pointer_get(actor_t *actor, uint8_t index);

static inline ODR_t actor_signal_to_odr(actor_signal_t signal) {
    switch (signal) {
    case ACTOR_SIGNAL_INCOMPLETE:
        return ODR_PARTIAL;
    case ACTOR_SIGNAL_OUT_OF_MEMORY:
        return ODR_OUT_OF_MEM;
    default:
        if (signal < 0) {
            return ODR_GENERAL;
        } else {
            return ODR_OK;
        }
    }
}

static inline actor_signal_t actor_signal_from_odr(ODR_t signal) {
    switch (signal) {
    case ODR_PARTIAL:
        return ACTOR_SIGNAL_INCOMPLETE;
    case ODR_OUT_OF_MEM:
        return ACTOR_SIGNAL_OUT_OF_MEMORY;
    default:
        if (ODR_OK) {
            return ACTOR_SIGNAL_OK;
        } else {
            return ACTOR_SIGNAL_FAILURE;
        }
    }
}

#endif