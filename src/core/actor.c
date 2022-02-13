#include "actor.h"
#include "301/CO_ODinterface.h"
#include "core/buffer.h"
#include "system/canopen.h"

int actor_send(actor_t *actor, actor_t *origin, void *value, void *argument) {
    if (actor->class->on_value == NULL) {
        return 1;
    }
    return actor->class->on_value(actor->object, origin, value, argument);
}

int actor_signal(actor_t *actor, actor_t *origin, app_signal_t signal, void *argument) {
    if (actor->class->on_signal == NULL) {
        return 1;
    }
    return actor->class->on_signal(actor->object, origin, signal, argument);
}

int actor_link(actor_t *actor, void **destination, uint16_t index, void *argument) {
    if (index == 0) {
        return 0;
    }
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
    actor->object = malloc(actor->class->size);
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

    return actor_workers_allocate(actor);
}

int actor_free(actor_t *actor) {
    free(actor->object);
    return actor_workers_free(actor);
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

static void actor_event_set_status(actor_t *actor, app_event_t *event, app_event_status_t status) {
    // when actor accepted event that had `app_buffer_t` struct as a data,...
    if (event->size == APP_BUFFER_DYNAMIC_SIZE) {
        switch (status) {
        // the semaphore is icnremented to prevent deallocation
        case APP_EVENT_RECEIVED:
        case APP_EVENT_HANDLED:
        case APP_EVENT_DEFERRED:
            // if previous status was deferred, the users counter was already incremented
            if (event->status != APP_EVENT_DEFERRED) {
                app_buffer_use((app_buffer_t *)event->data, actor);
            }
            break;
        // when processing is done the user counter gets decremented
        case APP_EVENT_FINALIZED:
            app_buffer_release((app_buffer_t *)event->data, actor);
            break;
        default:
            break;
        }
    }
    event->status = status;
}

app_signal_t actor_event_accept_and_process_generic(actor_t *actor, app_event_t *event, app_event_t *destination,
                                                    app_event_status_t ready_status, app_event_status_t busy_status,
                                                    actor_on_report_t handler) {
    // Check if destination can store the incoming event, otherwise actor will be considered busy
    if (destination == NULL || destination->type == APP_EVENT_IDLE) {
        if (ready_status >= APP_EVENT_HANDLED) {
            event->consumer = actor;
        }
        actor_event_set_status(actor, event, ready_status);
        memcpy(destination, event, sizeof(app_event_t));
        if (handler != NULL) {
            return handler(actor->object, destination);
        }
        return APP_SIGNAL_OK;
    } else {
        actor_event_set_status(actor, event, busy_status);
        return APP_SIGNAL_BUSY;
    }
}

app_signal_t actor_event_accept_and_start_job_generic(actor_t *actor, app_event_t *event, app_job_t *job, app_thread_t *thread,
                                                      actor_on_job_t handler, app_event_status_t ready_status,
                                                      app_event_status_t busy_status) {
    app_signal_t signal = actor_event_accept_and_process_generic(actor, event, &job->inciting_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        job->actor = actor;
        job->handler = handler;
        memcpy(&job->inciting_event, event, sizeof(app_event_t));
        memset(&job->awaited_event, 0, sizeof(app_event_t));
        job->task_phase = job->job_phase = 0;
        job->thread = thread;
        job->counter = 0;
        debug_printf("| ├ Task start\t\t%s for %s\t\n", get_actor_type_name(actor->class->type), app_thread_get_name(thread));
        app_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
}

app_signal_t actor_event_accept_and_pass_to_job_generic(actor_t *actor, app_event_t *event, app_job_t *job, app_thread_t *thread,
                                                        actor_on_job_t handler, app_event_status_t ready_status,
                                                        app_event_status_t busy_status) {
    if (job->handler != handler) {
        actor_event_set_status(actor, event, busy_status);
        return APP_SIGNAL_BUSY;
    }
    app_signal_t signal = actor_event_accept_and_process_generic(actor, event, &job->awaited_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        app_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
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

app_signal_t actor_event_report(actor_t *actor, app_event_t *event) {
    (void)actor;
    if (event->producer && event->producer != actor && event->producer->class->on_report) {
        return event->producer->class->on_report(event->producer->object, event);
    } else {
        return APP_SIGNAL_OK;
    }
}

app_signal_t actor_event_finalize(actor_t *actor, app_event_t *event) {
    if (event != NULL && event->type != APP_EVENT_IDLE) {
        if (event->type != APP_EVENT_THREAD_ALARM) {
            debug_printf("│ │ ├ Finalize\t\t#%s of %s\n", get_app_event_type_name(event->type),
                         get_actor_type_name(event->producer->class->type));

            actor_event_report(actor, event);

            actor_event_set_status(actor, event, APP_EVENT_FINALIZED);
        }

        memset(event, 0, sizeof(app_event_t));
    }
    return APP_SIGNAL_OK;
}

app_signal_t actor_worker_catchup(actor_t *actor, actor_worker_t *tick) {
    if (tick == NULL)
        tick = actor->ticks->input;
    if (tick != NULL) {
        app_thread_t *thread = tick->catchup;
        if (thread) {
            tick->catchup = NULL;
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

ODR_t actor_set_property_string(actor_t *actor, uint8_t index, uint8_t *data, size_t size) {
    OD_obj_record_t *odo = &((OD_obj_record_t *)actor->entry->odObject)[index];
    if (size < odo->dataLength) {
        (&odo->dataOrig)[size + 1] = '\0';
    }
    return actor_set_property(actor, index, data, size);
}

ODR_t actor_compute_property_stream(actor_t *actor, uint8_t index, uint8_t *data, OD_size_t size, OD_stream_t *stream, OD_size_t *count_read) {
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