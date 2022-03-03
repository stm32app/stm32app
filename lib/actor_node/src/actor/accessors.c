#include "actor/actor.h"
#include "301/CO_ODinterface.h"

typedef struct {
    void *dataOrig;       /**< Pointer to data */
    uint8_t subIndex;     /**< Sub index of element. */
    OD_attr_t attribute;  /**< Attribute bitfield, see @ref OD_attributes_t */
    OD_size_t dataLength; /**< Data length in bytes */
} OD_obj_record_t;

ODR_t actor_set_property(actor_t *actor, uint8_t index, void *value, size_t size) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];

    // bail out quickly if value hasnt changed
    if (memcmp(odo->dataOrig, value, size) == 0) {
        return 0;
    }

    // special case of phase handler
    if (index == actor->class->phase_subindex) {
        memcpy(odo->dataOrig, value, size);
        actor_on_phase_change(actor, *((actor_phase_t *)value));
        return ODR_OK;
    }

    // quickly copy the value if there is no custom observer
    if (actor->class->property_write == NULL) {
        memcpy(odo->dataOrig, value, size);
        return ODR_OK;
    }

    OD_size_t count_written = 0;
    OD_stream_t stream = {
        .dataOrig = odo->dataOrig,
        .dataLength = odo->dataLength,
        .attribute = odo->attribute,
        .object = actor->object,
        .subIndex = index,
        .dataOffset = 0,
    };

    return actor->class->property_write(&stream, value, size, &count_written);
}

ODR_t actor_set_property_numeric(actor_t *actor, uint8_t index, uint32_t value, size_t size) {
    return actor_set_property(actor, index, &value, size);
}

ODR_t actor_set_property_string(actor_t *actor, uint8_t index, char *data, size_t size) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
    if (size < odo->dataLength) {
        (&odo->dataOrig)[size + 1] = 0;
    }
    return actor_set_property(actor, index, data, size);
}

ODR_t actor_compute_property_stream(actor_t *actor, uint8_t index, uint8_t *data, OD_size_t size, OD_stream_t *stream,
                                    OD_size_t *count_read) {
    actor_assert(stream);
    if (stream->dataOrig == NULL) {
        OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
        *stream = (OD_stream_t){
            .dataOrig = odo->dataOrig,
            .dataLength = odo->dataLength,
            .attribute = odo->attribute,
            .object = actor->object,
            .subIndex = index,
            .dataOffset = 0,
        };
    }
    if (data == NULL)
        data = stream->dataOrig;
    if (size == 0)
        size = stream->dataLength;
    return actor->class->property_read(stream, data, size, count_read);
}

ODR_t actor_compute_property_into_buffer(actor_t *actor, uint8_t index, uint8_t *data, OD_size_t size) {
    OD_size_t count_read = 0;
    OD_stream_t stream;
    return actor_compute_property_stream(actor, index, data, size, &stream, &count_read);
}

ODR_t actor_compute_property(actor_t *actor, uint8_t index) {
    return actor_compute_property_into_buffer(actor, index, NULL, 0);
}

void *actor_get_property_pointer(actor_t *actor, uint8_t index) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];

    // allow getters to run if actor has it
    if (actor->class->property_read != NULL) {
        actor_compute_property(actor, index);
    }
    return odo->dataOrig;
}