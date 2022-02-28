#include "app.h"
#include "core/buffer.h"
#include "system/canopen.h"

// Count or initialize all actors in OD of given type
size_t app_actor_type_enumerate(app_t *app, OD_t *od, actor_class_t *class, actor_t *destination, size_t offset) {
    size_t count = 0;

    for (size_t seq = 0; seq < 128; seq++) {
        OD_entry_t *properties = OD_find(od, class->type + seq);
        if (properties == NULL && seq >= 19)
            break;
        if (properties == NULL)
            continue;

        uint8_t *phase = OD_getPtr(properties, class->phase_subindex, 0, NULL);

        // compute struct offset for phase property
        class->phase_offset = (void *)phase - OD_getPtr(properties, 0x00, 0, NULL);

        if (*phase != ACTOR_ENABLED || class->validate(OD_getPtr(properties, 0x00, 0, NULL)) != 0) {
            if (app != NULL && app->canopen != NULL) {
                app_error_report(app, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_ADDITIONAL_MODUL, OD_getIndex(properties));
            }
            continue;
        }

        count++;
        if (destination == NULL) {
            continue;
        }

        actor_t *actor = &destination[offset + count - 1];
        actor->seq = seq;
        actor->entry = properties;
        actor->class = class;

        actor->entry_extension.write = class->property_write == NULL ? OD_writeOriginal : class->property_write;
        actor->entry_extension.read = class->property_read == NULL ? OD_readOriginal : class->property_read;

        OD_extension_init(properties, &actor->entry_extension);
    }
    return count;
}

actor_t *app_actor_find(app_t *app, uint16_t index) {
    for (size_t i = 0; i < app->actor_count; i++) {
        actor_t *actor = &app->actor[i];
        if (actor_index(actor) == index) {
            return &app->actor[i];
        }
    }
    return NULL;
}

actor_t *app_actor_find_by_type(app_t *app, uint16_t type) {
    for (size_t i = 0; i < app->actor_count; i++) {
        actor_t *actor = &app->actor[i];
        if (actor->class->type == type) {
            return &app->actor[i];
        }
    }
    return NULL;
}

actor_t *app_actor_find_by_number(app_t *app, uint8_t number) {
    return &app->actor[number];
}

uint8_t app_actor_find_number(app_t *app, actor_t *actor) {
    for (size_t i = 0; i < app->actor_count; i++) {
        if (&app->actor[i] == actor) {
            return i;
        }
    }
    return 255;
}

void app_set_phase(app_t *app, actor_phase_t phase) {
    debug_printf("Devices - phase %s\n", get_actor_phase_name(phase));
    for (size_t i = 0; i < app->actor_count; i++) {
        // if (app->actor[i].phase != ACTOR_DISABLED) {
        actor_set_phase(&app->actor[i], phase);
        //}
    }
}

int app_allocate(app_t **app, OD_t *od, size_t (*enumerator)(app_t *app, OD_t *od, actor_t *actors)) {
    // count actors first to allocate specific size of an array
    size_t actor_count = enumerator(NULL, od, NULL);
    actor_t *actors = app_calloc_fast(actor_count, sizeof(actor_t));

    if (actors == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    }
    // run actor constructors
    enumerator(*app, od, actors);

    for (size_t i = 0; i < actor_count; i++) {
        // allocate memory for actor struct
        int ret = actor_allocate(&actors[i]);
        if (ret != 0) {
            return ret;
        }

        // first actor must be app actor
        // store actor count
        if (i == 0) {
            *app = actors->object;
            (*app)->actor_count = actor_count;
        }

        // link actor to the app
        (&actors[i])->app = *app;
    }

    return 0;
}

int app_deallocate(app_t **app) {
    for (size_t i = 0; i < (*app)->actor_count; i++) {
        int ret = actor_free(&(*app)->actor[i]);
        if (ret != 0)
            return ret;
    }
    (*app)->actor_count = 0;
    app_free((*app)->actor);
    return 0;
}

/* Strong symbols redefining defaults */

size_t app_thread_iterate_workers(app_thread_t *thread, actor_worker_t *destination) {
    app_t *app = thread->actor->app;
    size_t count = 0;
    for (size_t i = 0; i < app->actor_count; i++) {
        actor_t *actor = &app->actor[i];
        if (!actor->class->on_worker_assignment) {
            continue;
        }
        actor_worker_callback_t handler = actor->class->on_worker_assignment(actor->object, thread);
        if (!handler) {
            continue;
        }
        if (destination) {
            destination[count] = (actor_worker_t){.callback = handler, .thread = thread, .actor = actor};
        }
        count++;
    }

    return count;
}

actor_t *app_thread_get_root_actor(app_thread_t *thread) {
    return thread->actor->app->actor;
}

/* Some threads may want to redirect their stale messages to another thread */
app_thread_t *app_thread_get_catchup_thread(app_thread_t *thread) {
    if (thread == thread->actor->app->input) {
        return thread->actor->app->catchup;
    } else {
        return thread;
    }
}

app_buffer_t *app_buffer_alloc(actor_t *actor) {
    return app_pool_allocate_buffer(actor->app->buffers);
}

void app_buffer_free(app_buffer_t *buffer) {
    return app_pool_free(buffer, sizeof(app_buffer_t));
}

app_job_t *app_job_alloc(actor_t *actor) {
    return app_pool_allocate_job(actor->app->jobs);
}

void app_job_free(app_job_t *buffer) {
    return app_pool_free(job, sizeof(app_buffer_t));
}

void app_job_complete_callback(app_job_t *job) {
    if (job->actor->class->on_job_complete != NULL) {
        job->actor->class->on_job_complete(job->actor->object, job);
    }
}

void app_actor_idle_callback(actor_t *actor) {
    actor_worker_catchup(actor, NULL);
}

bool_t app_thread_is_interrupted(app_thread_t *thread) {
    return (SCB_ICSR & SCB_ICSR_VECTACTIVE);
}

void app_event_report_callback(actor_t *actor, app_event_t *event) {
    if (event->producer && event->producer != actor && event->producer->class->on_event_report) {
        event->producer->class->on_event_report(event->producer->object, event);
    }
}

app_signal_t app_buffer_allocation_callback(app_buffer_t *buffer, size_t size) {
    if (buffer->owner->class->on_buffer_allocation) {                                                                                      
        return buffer->owner->class->on_buffer_allocation(buffer->owner->object, buffer, size);                                            
    } else {
        return APP_SIGNAL_NOT_IMPLEMENTED;
    }
}
app_thread_t* actor_get_input_thread(actor_t *actor) {
    return actor->app->input;
}

char *app_actor_stringify(actor_t *actor) {
  return get_actor_type_name(actor->class->type);
};

void *actor_unbox(actor_t *actor) {
  return actor;
}