#ifndef INC_ACTOR_ACCESSORS
#define INC_ACTOR_ACCESSORS

#include "actor/types.h"
#include "301/CO_ODinterface.h"

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

#endif