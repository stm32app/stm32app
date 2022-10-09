#include <actor/actor.h>
#include <actor/buffer.h>

int actor_send(actor_t *actor, actor_t *origin, void *value, void *argument) {
    actor_assert(actor && origin);
    if (actor->interface->value_received == NULL) {
        return -1;
    }
    return actor->interface->value_received(actor->object, origin, value, argument);
}

int actor_signal(actor_t *actor, actor_signal_t signal, actor_t *caller, void *argument) {
    actor_assert(actor && caller);
    if (actor->interface->signal_received == NULL) {
        return -1;
    }
    return actor->interface->signal_received(actor->object, signal, caller, argument);
}

int actor_link(actor_t *actor, void **destination, uint16_t index, void *argument) {
    if (index == 0) {
        return 0;
    }
    actor_assert(actor);
    actor_t *target = actor_node_find(actor->node, index);
    if (target != NULL) {
        *destination = target;
        actor_signal(target, ACTOR_SIGNAL_LINK, actor, argument);
        return 0;
    } else {
        debug_printf("    ! Device 0x%x (%s) could not find actor 0x%x\n", actor_index(actor), actor_stringify(actor), index);
        *destination = NULL;
        return -1;
    }
}

int actor_object_allocate(actor_t *actor) {
    actor->object = actor_calloc_region(ACTOR_REGION_FAST, 1, actor->interface->size);
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

actor_signal_t actor_callback_phase_changed(actor_t *actor, actor_phase_t phase, actor_phase_t previous) {
    if (actor->interface->phase_changed != NULL) {
        actor_signal_t signal = actor->interface->phase_changed(actor->object, phase);
        switch (signal) {
        case ACTOR_SIGNAL_CANCEL:
        case ACTOR_SIGNAL_WAIT:
            actor_set_phase(actor, previous);
            break;
        default:
            if (signal < 0) {
                actor_set_phase(actor, ACTOR_PHASE_DISABLED);
            }
            break;
        }
        return signal;
    }
#if DEBUG
    debug_printf("  - Device phase: 0x%x %s %s <= %s\n", actor_index(actor), actor_stringify(actor), actor_phase_stringify(actor_get_phase(actor)),
                 actor_phase_stringify(previous));
#endif
    return ACTOR_SIGNAL_OK;
}

bool actor_message_is_subscribed(actor_t *actor, actor_message_t *event) {
    return actor->event_subscriptions & event->type;
}

void actor_message_subscribe(actor_t *actor, actor_message_type_t type) {
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