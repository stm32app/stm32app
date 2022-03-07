#include <actor/buffer.h>
#include <actor/node.h>
#include "definitions/enums.h"

// Count or initialize all actors in OD of given type
size_t actor_node_type_enumerate(actor_node_t *node, OD_t *od, actor_class_t *actor_class, actor_t *destination, size_t offset) {
    size_t count = 0;

    for (size_t seq = 0; seq < 128; seq++) {
        OD_entry_t *properties = OD_find(od, actor_class->type + seq);
        if (properties == NULL && seq >= 19)
            break;
        if (properties == NULL)
            continue;

        uint8_t *phase = OD_getPtr(properties, actor_class->phase_subindex, 0, NULL);

        // compute struct offset for phase property
        actor_class->phase_offset = (void *)phase - OD_getPtr(properties, 0x00, 0, NULL);

        if (*phase != ACTOR_ENABLED || actor_class->validate(OD_getPtr(properties, 0x00, 0, NULL)) != 0) {
            if (node != NULL) {
                actor_node_error_report(node, APP_ERROR_INCONSISTENT_OBJECT_DICT, APP_ERROR_ADDITIONAL_MODUL, OD_getIndex(properties));
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
        actor->class = actor_class;

        actor->entry_extension.write = actor_class->property_write == NULL ? OD_writeOriginal : actor_class->property_write;
        actor->entry_extension.read = actor_class->property_read == NULL ? OD_readOriginal : actor_class->property_read;

        OD_extension_init(properties, &actor->entry_extension);
    }
    return count;
}

actor_t *actor_node_find(actor_node_t *node, uint16_t index) {
    for (size_t i = 0; i < node->actor_count; i++) {
        actor_t *actor = &node->actor[i];
        if (actor_index(actor) == index) {
            return &node->actor[i];
        }
    }
    return NULL;
}

actor_t *actor_node_find_by_type(actor_node_t *node, uint16_t type) {
    for (size_t i = 0; i < node->actor_count; i++) {
        actor_t *actor = &node->actor[i];
        if (actor->class->type == type) {
            return &node->actor[i];
        }
    }
    return NULL;
}

actor_t *actor_node_find_by_number(actor_node_t *node, uint8_t number) {
    return &node->actor[number];
}

uint8_t actor_node_find_number(actor_node_t *node, actor_t *actor) {
    for (size_t i = 0; i < node->actor_count; i++) {
        if (&node->actor[i] == actor) {
            return i;
        }
    }
    return 255;
}

void actor_node_actors_set_phase(actor_node_t *node, actor_phase_t phase) {
    debug_printf("Devices - phase %s\n", actor_phase_stringify(phase));
    for (size_t i = 0; i < node->actor_count; i++) {
        // if (node->actor[i].phase != ACTOR_DISABLED) {
        actor_set_phase(&node->actor[i], phase);
        //}
    }
}

int actor_node_allocate(actor_node_t **node, OD_t *od, size_t (*enumerator)(actor_node_t *node, OD_t *od, actor_t *actors)) {
    // count actors first to allocate specific size of an array
    size_t actor_count = enumerator(NULL, od, NULL);
    actor_t *actors = actor_calloc_region(ACTOR_REGION_FAST, actor_count, sizeof(actor_t));

    if (actors == NULL) {
        return ACTOR_SIGNAL_OUT_OF_MEMORY;
    }
    // run actor constructors
    enumerator(*node, od, actors);

    for (size_t i = 0; i < actor_count; i++) {
        // allocate memory for actor struct
        int ret = actor_object_allocate(&actors[i]);
        if (ret != 0) {
            return ret;
        }

        // first actor must be node actor
        // store actor count
        if (i == 0) {
            *node = actors->object;
            (*node)->actor_count = actor_count;
        }

        // link actor to the node
        (&actors[i])->node = *node;
    }

    return 0;
}

int actor_node_deallocate(actor_node_t **node) {
    for (size_t i = 0; i < (*node)->actor_count; i++) {
        int ret = actor_object_free(&(*node)->actor[i]);
        if (ret != 0)
            return ret;
    }
    (*node)->actor_count = 0;
    actor_object_free((*node)->actor);
    return 0;
}

/* Strong symbols redefining defaults */

uint32_t actor_thread_iterate_workers(actor_thread_t *thread, actor_worker_t *destination) {
    actor_node_t *node = thread->actor->node;
    size_t count = 0;
    for (size_t i = 0; i < node->actor_count; i++) {
        actor_t *actor = &node->actor[i];
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

actor_t *actor_thread_get_root_actor(actor_thread_t *thread) {
    return thread->actor->node->actor;
}

/* Some threads may want to redirect their stale messages to another thread */
actor_thread_t *actor_thread_get_catchup_thread(actor_thread_t *thread) {
    if (thread == thread->actor->node->input) {
        return thread->actor->node->catchup;
    } else {
        return thread;
    }
}

actor_buffer_t *actor_buffer_alloc(actor_t *actor) {
    return actor_pool_allocate(actor->node->buffers, sizeof(actor_buffer_t));
}

void actor_buffer_free(actor_buffer_t *buffer) {
    return actor_pool_free(buffer, sizeof(actor_buffer_t));
}

actor_job_t *actor_job_alloc(actor_t *actor) {
    return actor_pool_allocate(actor->node->jobs, sizeof(actor_job_t));
}

void actor_job_free(actor_job_t *job) {
    return actor_pool_free(job, sizeof(actor_buffer_t));
}

void actor_job_complete_callback(actor_job_t *job) {
    if (job->actor->class->on_job_complete != NULL) {
        job->actor->class->on_job_complete(job->actor->object, job);
    }
}

void actor_node_idle_callback(actor_t *actor) {
    actor_worker_catchup(actor, NULL);
}

void actor_event_report_callback(actor_t *actor, actor_event_t *event) {
    if (event->producer && event->producer != actor && event->producer->class->on_event_report) {
        event->producer->class->on_event_report(event->producer->object, event);
    }
}

actor_signal_t actor_buffer_allocation_callback(actor_buffer_t *buffer, uint32_t size) {
    if (buffer->owner->class->on_buffer_allocation) {
        return buffer->owner->class->on_buffer_allocation(buffer->owner->object, buffer, size);
    } else {
        return ACTOR_SIGNAL_NOT_IMPLEMENTED;
    }
}
actor_thread_t *actor_get_input_thread(actor_t *actor) {
    return actor->node->input;
}

#ifdef INC_ENUMS
char *actor_stringify(actor_t *actor) {
    return get_actor_type_name(actor->class->type);
};
char *actor_event_stringify(actor_event_t *event) {
    return get_actor_event_type_name(event->type);
};
char *actor_phase_stringify(int phase) {
    return get_actor_phase_name(phase);
};
#endif

void *actor_unbox(actor_t *actor) {
    return actor->object;
}

actor_t *actor_box(void *object) {
    uintXX_t *ptr = object;
    return (actor_t *)((uintXX_t)*ptr);
}
