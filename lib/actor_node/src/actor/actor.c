#include <actor/actor.h>
#include <actor/buffer.h>

int actor_send(actor_t *actor, actor_t *origin, void *value, void *argument) {
    actor_assert(actor && origin);
    if (actor->class->on_value == NULL) {
        return 1;
    }
    return actor->class->on_value(actor->object, origin, value, argument);
}

int actor_signal(actor_t *actor, actor_t *origin, actor_signal_t signal, void *argument) {
    actor_assert(actor && origin);
    if (actor->class->on_signal == NULL) {
        return 1;
    }
    return actor->class->on_signal(actor->object, origin, signal, argument);
}

int actor_link(actor_t *actor, void **destination, uint16_t index, void *argument) {
    if (index == 0) {
        return 0;
    }
    actor_assert(actor);
    actor_t *target = actor_node_find(actor->node, index);
    if (target != NULL) {
        *destination = target->object;
        if (target->class->on_link != NULL) {
            target->class->on_link(target->object, actor, argument);
        }

        return 0;
    } else {
        *destination = NULL;
        debug_printf("    ! Device 0x%x (%s) could not find actor 0x%x\n", actor_index(actor), actor_stringify(actor),
                     index);
        actor_set_phase(actor, ACTOR_DISABLED);
        actor_error_report(actor, 0x2DU/*CO_EM_INCONSISTENT_OBJECT_DICT*/, 0x7000U/*CO_EMC_ADDITIONAL_MODUL*/);
        return 1;
    }
}

int actor_object_allocate(actor_t *actor) {
    actor->object = actor_calloc(1, actor->class->size);
    if (actor->object == NULL) {
        return ACTOR_SIGNAL_OUT_OF_MEMORY;
    }
    // cast to node object which is following actor conventions
    actor_node_t *obj = actor->object;
    // by convention each object struct has pointer to actor as its first member
    obj->actor = actor;
    // second member is `properties` poiting to memory struct in OD
    obj->properties = OD_getPtr(actor->entry, 0x00, 0, NULL);

    actor->entry_extension.object = actor->object;

    return 0;
}

int actor_object_free(actor_t *actor) {
    actor_free(actor->object);
    return 0;
}

void actor_on_phase_change(actor_t *actor, actor_phase_t phase) {
#if DEBUG
    debug_printf("  - Device phase: 0x%x %s %s <= %s\n", actor_index(actor), actor_stringify(actor),
                 actor_phase_stringify(phase), actor_phase_stringify(actor->previous_phase));
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

bool actor_event_is_subscribed(actor_t *actor, actor_event_t *event) {
    return actor->event_subscriptions & event->type;
}

void actor_event_subscribe(actor_t *actor, actor_event_type_t type) {
    actor->event_subscriptions |= type;
}

actor_signal_t actor_worker_catchup(actor_t *actor, actor_worker_t *worker) {
    if (worker == NULL)
        worker = actor_thread_worker_for(actor->node->input, actor);
    if (worker != NULL) {
        actor_thread_t *thread = worker->catchup;
        if (thread) {
            worker->catchup = NULL;
            return actor_thread_catchup(thread);
        }
    }
    return ACTOR_SIGNAL_OK;
}
