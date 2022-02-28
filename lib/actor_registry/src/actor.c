#include "actor.h"
#include "301/CO_ODinterface.h"
#include "core/buffer.h"
#include "system/canopen.h"

int actor_send(actor_t *actor, actor_t *origin, void *value, void *argument) {
    configASSERT(actor && origin);
    if (actor->class->on_value == NULL) {
        return 1;
    }
    return actor->class->on_value(actor->object, origin, value, argument);
}

int actor_signal(actor_t *actor, actor_t *origin, app_signal_t signal, void *argument) {
    configASSERT(actor && origin);
    if (actor->class->on_signal == NULL) {
        return 1;
    }
    return actor->class->on_signal(actor->object, origin, signal, argument);
}

int actor_link(actor_t *actor, void **destination, uint16_t index, void *argument) {
    if (index == 0) {
        return 0;
    }
    configASSERT(actor);
    actor_t *target = app_actor_find(actor->app, index);
    if (target != NULL) {
        *destination = target->object;
        if (target->class->on_link != NULL) {
            target->class->on_link(target->object, actor, argument);
        }

        return 0;
    } else {
        *destination = NULL;
        debug_printf("    ! Device 0x%x (%s) could not find actor 0x%x\n", actor_index(actor), get_actor_type_name(actor->class->type),
                     index);
        actor_set_phase(actor, ACTOR_DISABLED);
        actor_error_report(actor, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL);
        return 1;
    }
}

int actor_allocate(actor_t *actor) {
    actor->object = app_malloc(actor->class->size);
    if (actor->object == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    }
    // cast to app object which is following actor conventions
    app_t *obj = actor->object;
    // by convention each object struct has pointer to actor as its first member
    obj->actor = actor;
    // second member is `properties` poiting to memory struct in OD
    obj->properties = OD_getPtr(actor->entry, 0x00, 0, NULL);

    actor->entry_extension.object = actor->object;

    return 0;
}

int actor_free(actor_t *actor) {
    app_free(actor->object);
    return 0;
}

int actor_timeout_check(uint32_t *clock, uint32_t time_since_last_tick, uint32_t *next_tick) {
    if (*clock > time_since_last_tick) {
        *clock -= time_since_last_tick;
        if (*next_tick > *clock) {
            *next_tick = *clock;
        }
        return 1;
    } else {
        *clock = 0;
        return 0;
    }
}


void actor_on_phase_change(actor_t *actor, actor_phase_t phase) {
#if DEBUG
    debug_printf("  - Device phase: 0x%x %s %s <= %s\n", actor_index(actor), get_actor_type_name(actor->class->type),
                 get_actor_phase_name(phase), get_actor_phase_name(actor->previous_phase));
    actor->previous_phase = phase;
#endif

    switch (phase) {
    case ACTOR_CONSTRUCTING:
        if (actor->class->construct != NULL) {
            if (actor->class->construct(actor->object)) {
                actor_set_phase(actor, ACTOR_DISABLED);
                return;
            }
        }
        break;
    case ACTOR_DESTRUCTING:
        if (actor->class->destruct != NULL) {
            actor->class->destruct(actor->object);
        }
        break;
    case ACTOR_LINKING:
        if (actor->class->link != NULL) {
            actor->class->link(actor->object);
        }
        break;
    case ACTOR_STARTING:
        actor->class->start(actor->object);
        if (actor_get_phase(actor) == ACTOR_STARTING) {
            actor_set_phase(actor, ACTOR_RUNNING);
            return;
        }
        break;
    case ACTOR_STOPPING:
        actor->class->stop(actor->object);
        if (actor_get_phase(actor) == ACTOR_STOPPING) {
            actor_set_phase(actor, ACTOR_STOPPED);
            return;
        }
        break;
    case ACTOR_PAUSING:
        if (actor->class->pause == NULL) {
            actor_set_phase(actor, ACTOR_STOPPING);
        } else {
            actor->class->pause(actor->object);
            if (actor_get_phase(actor) == ACTOR_PAUSING) {
                actor_set_phase(actor, ACTOR_PAUSED);
                return;
            }
        }
        break;
    case ACTOR_RESUMING:
        if (actor->class->resume == NULL) {
            actor_set_phase(actor, ACTOR_STARTING);
            return;
        } else {
            actor->class->resume(actor->object);
            if (actor_get_phase(actor) == ACTOR_RESUMING) {
                actor_set_phase(actor, ACTOR_RUNNING);
                return;
            }
        }
        break;
    default:
        break;
    }

    if (actor->class->on_phase != NULL) {
        actor->class->on_phase(actor->object, phase);
    }
}

bool_t actor_event_is_subscribed(actor_t *actor, app_event_t *event) {
    return actor->event_subscriptions & event->type;
}

void actor_event_subscribe(actor_t *actor, app_event_type_t type) {
    actor->event_subscriptions |= type;
}

app_signal_t actor_worker_catchup(actor_t *actor, actor_worker_t *worker) {
    if (worker == NULL)
        worker = app_thread_worker_for(actor->app->input, actor);
    if (worker != NULL) {
        app_thread_t *thread = worker->catchup;
        if (thread) {
            worker->catchup = NULL;
            return app_thread_catchup(thread);
        }
    }
    return APP_SIGNAL_OK;
}
inline void actor_gpio_set(uint8_t port, uint8_t pin) {
    return gpio_set(GPIOX(port), 1 << pin);
}
inline void actor_gpio_clear(uint8_t port, uint8_t pin) {
    return gpio_clear(GPIOX(port), 1 << pin);
}
inline uint32_t actor_gpio_get(uint8_t port, uint8_t pin) {
    return gpio_get(GPIOX(port), 1 << pin);
}

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
        (&odo->dataOrig)[size + 1] = '\0';
    }
    return actor_set_property(actor, index, data, size);
}

ODR_t actor_compute_property_stream(actor_t *actor, uint8_t index, uint8_t *data, OD_size_t size, OD_stream_t *stream,
                                    OD_size_t *count_read) {
    configASSERT(stream);
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