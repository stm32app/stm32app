#include "actor/actor.h"
#include "301/CO_ODinterface.h"

typedef struct {
    void *dataOrig;       /**< Pointer to data */
    uint8_t subIndex;     /**< Sub index of element. */
    OD_attr_t attribute;  /**< Attribute bitfield, see @ref OD_attributes_t */
    OD_size_t dataLength; /**< Data length in bytes */
} OD_obj_record_t;


uint8_t actor_property_buffer[8];
actor_signal_t actor_property_set(actor_t *actor, uint8_t index, void *value, size_t size) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
    actor_signal_t signal = ACTOR_SIGNAL_OK;

    // bail out quickly if value hasnt changed
    if (memcmp(odo->dataOrig, value, size) == 0) {
        return ACTOR_SIGNAL_UNAFFECTED;
    }

    // run callbacks that may prevent value from being set
    if (actor->interface->property_before_change != NULL) {
        signal = actor->interface->property_before_change(actor->object, index, value, odo->dataOrig);
        if (signal != ACTOR_SIGNAL_OK) {
            return signal;
        }
    }

    // store old value in a buffer shared by all nodes
    if (actor->interface->property_after_change != NULL || actor->interface->phase_subindex == index) {
        memcpy(actor_property_buffer, odo->dataOrig, odo->dataLength < 8 ? odo->dataLength : 8);
    }

    // write a new value
    memcpy(odo->dataOrig, value, size);

    // run special case phase setter
    if (actor->interface->phase_subindex == index) {
        signal = actor_callback_phase_changed(actor, *((actor_phase_t *) value), *((actor_phase_t *) actor_property_buffer));
        if (signal != ACTOR_SIGNAL_OK) {
            return signal;
        }
    }

    // run callbacks that react to change
    if (actor->interface->property_after_change != NULL) {
        signal = actor->interface->property_after_change(actor->object, index, value, &actor_property_buffer);
        if (signal != ACTOR_SIGNAL_OK) {
            return signal;
        }
    }

    return signal;
}

/* Set numeric value of given size */ 
actor_signal_t actor_property_numeric_set(actor_t *actor, uint8_t index, uint32_t value, size_t size) {
    return actor_property_set(actor, index, &value, size);
}

/* Set stringy value of given size without streaming */
actor_signal_t actor_property_string_set(actor_t *actor, uint8_t index, char *data, size_t size) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
    actor_signal_t signal = actor_property_set(actor, index, data, size);
    // terminate string value with NULL
    if (signal >= 0 && size < odo->dataLength) {
        (&odo->dataOrig)[size + 1] = 0;
    }
    return signal;
}

actor_signal_t actor_property_stream_set(actor_t *actor, uint8_t index, void *value, size_t size, uint32_t *count_written) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
    OD_stream_t stream = {
        .dataOrig = odo->dataOrig,
        .dataLength = odo->dataLength,
        .attribute = odo->attribute,
        .object = actor->object,
        .subIndex = index,
        .dataOffset = *count_written,
    };
    return actor_signal_from_odr(actor_property_writer(&stream, value, size, count_written));
}

/*
    Version of `actor_property_set` that is fully compatible with OD writers, so it is used used
    to receieve external writes from the CANopen to actor nodes. It retains behavioral changes
    of actor_property_set, but allows values to be streamed in chunks. It converts Actor
    signals to ODR return values for compatability with OD interface, so Actor functions 
    that use it (like `actor_property_stream_set`) directly do a backward conversion
*/
ODR_t actor_property_writer(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *count_written) {
    actor_t *actor = stream->object;
    actor_signal_t signal = ACTOR_SIGNAL_OK;

    // try processing value as a stream if actor supports streaming
    if (actor->interface->stream_write != NULL) {
        signal = actor->interface->stream_write(stream, buf, count, count_written);
        if (signal != ACTOR_SIGNAL_UNAFFECTED) {
            return actor_signal_to_odr(signal);
        }
    }

    // setting the same value does not invoke listeners
    if (stream->dataLength == count && memcmp(stream->dataOrig, buf, stream->dataLength) == 0) {
        return ODR_OK;
    }

    // run callbacks that can prevent value from being set
    if (actor->interface->property_before_change != NULL && stream->dataLength == count) {
        signal = actor->interface->property_before_change(actor->object, stream->subIndex, (void *) buf, stream->dataOrig);
        if (signal != ACTOR_SIGNAL_OK) {
            return actor_signal_to_odr(signal);
        }
    }
    
    // store old value in a buffer
    if (actor->interface->property_after_change != NULL) {
        memcpy(actor_property_buffer, stream->dataOrig, stream->dataLength < 8 ? stream->dataLength : 8);
    }

    // write new value, and only proceed if it's fully set
    ODR_t odr = OD_writeOriginal(stream, buf, count, count_written);
    if (odr != ODR_OK) {
        return odr;
    }

    // run callbacks that react to change
    if (actor->interface->property_after_change != NULL) {
        signal = actor->interface->property_after_change(actor->object, stream->subIndex, (void *) buf, &actor_property_buffer);
        if (signal != ACTOR_SIGNAL_OK) {
            return actor_signal_to_odr(signal);
        }
    }

    return actor_signal_to_odr(signal);
}

ODR_t actor_property_reader(OD_stream_t *stream, void *buf, OD_size_t count, OD_size_t *count_read) {
    actor_t *actor = stream->object;
    actor_signal_t signal = ACTOR_SIGNAL_OK;
    // try processing value as a stream if actor supports streaming
    if (actor->interface->stream_read != NULL) {
        signal = actor->interface->stream_write(stream, buf, count, count_read);
        if (signal != ACTOR_SIGNAL_UNAFFECTED) {
            return actor_signal_to_odr(signal);
        }
    }
    // run getter
    if (actor->interface->property_get != NULL && count == stream->dataLength) {
        signal = actor->interface->property_get(actor->object, stream->subIndex, buf);
        if (signal != ACTOR_SIGNAL_OK) {
            return actor_signal_to_odr(signal);
        }
    }
    return OD_readOriginal(stream, buf, count, count_read);
}

actor_signal_t actor_property_stream_get(actor_t *actor, uint8_t index, void *data, size_t size, uint32_t *count_read) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
    OD_stream_t stream = {
        .dataOrig = odo->dataOrig,
        .dataLength = odo->dataLength,
        .attribute = odo->attribute,
        .object = actor->object,
        .subIndex = index,
        .dataOffset = *count_read,
    };
    return actor_signal_from_odr(actor_property_reader(&stream, data, size, count_read));
}

void *actor_property_pointer_get(actor_t *actor, uint8_t index) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];

    // allow getters to run if actor has it
    if (actor->interface->property_get != NULL) {
        actor_signal_t signal = actor->interface->property_get(actor->object, index, odo->dataOrig);
        if (signal < 0) {
            return NULL;
        }
    }
    return odo->dataOrig;
}