#include <actor/event.h>
#include <actor/buffer.h>
#include <actor/job.h>
#include <actor/thread.h>


static void actor_event_set_status(actor_t *actor, actor_event_t *event, actor_event_status_t status) {
    // when actor accepted event that had `actor_buffer_t` struct as a data,...
    if (event->size == ACTOR_BUFFER_DYNAMIC_SIZE) {
        switch (status) {
        // the semaphore is icnremented to prevent deallocation
        case ACTOR_EVENT_RECEIVED:
        case ACTOR_EVENT_HANDLED:
        case ACTOR_EVENT_DEFERRED:
            // handled & deferred statuses make event not broadcast to other actors
            if (status != ACTOR_EVENT_RECEIVED) {
              event->consumer = actor;
            }
            // if previous status was deferred, the users counter was already incremented
            if (event->status != ACTOR_EVENT_DEFERRED) {
                actor_buffer_use((actor_buffer_t *)event->data, actor);
            }
            break;
        // when processing is done the user counter gets decremented
        case ACTOR_EVENT_FINALIZED:
            actor_buffer_release((actor_buffer_t *)event->data, actor);
            break;
        default:
            break;
        }
    }
    event->status = status;
}

actor_signal_t actor_event_accept_and_process_generic(actor_t *actor, actor_event_t *event, actor_event_t *destination,
                                                    actor_event_status_t ready_status, actor_event_status_t busy_status,
                                                    actor_on_event_report_t handler) {
    // Check if destination can store the incoming event, otherwise actor will be considered busy
    if (destination == NULL || destination->type == ACTOR_EVENT_IDLE) {
        actor_event_set_status(actor, event, ready_status);
        memcpy(destination, event, sizeof(actor_event_t));
        if (handler != NULL) {
            return handler(actor_unbox(actor), destination);
        } else {
          return ACTOR_SIGNAL_OK;
        }
    } else {
        actor_event_set_status(actor, event, busy_status);
        return ACTOR_SIGNAL_BUSY;
    }
}

actor_signal_t actor_event_accept_and_start_job_generic(actor_t *actor, actor_event_t *event, actor_job_t **job_slot, actor_thread_t *thread,
                                                      actor_on_job_complete_t handler, actor_event_status_t ready_status,
                                                      actor_event_status_t busy_status) {
    if (*job_slot == NULL)
      *job_slot = actor_job_alloc(actor);
    actor_job_t *job = *job_slot;
    if (!job) {
      return busy_status;
    }
    actor_signal_t signal = actor_event_accept_and_process_generic(actor, event, &job->inciting_event, ready_status, busy_status, NULL);
    if (signal == ACTOR_SIGNAL_OK) {
        job->actor = actor;
        job->thread = thread;
        job->handler = handler;
        debug_printf("| ├ Task start\t\t%s for %s\t\n", actor_node_stringify(actor), actor_thread_get_name(thread));
        actor_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
}

actor_signal_t actor_event_accept_and_pass_to_job_generic(actor_t *actor, actor_event_t *event, actor_job_t **job_slot, actor_thread_t *thread,
                                                        actor_on_job_complete_t handler, actor_event_status_t ready_status,
                                                        actor_event_status_t busy_status) {
    actor_job_t *job = *job_slot;
    actor_assert(job);
    // if task currently running is different, defer the event
    if (job->handler != handler) {
        actor_event_set_status(actor, event, busy_status);
        return ACTOR_SIGNAL_BUSY;
    }
    actor_signal_t signal = actor_event_accept_and_process_generic(actor, event, &job->incoming_event, ready_status, busy_status, NULL);
    if (signal == ACTOR_SIGNAL_OK) {
        actor_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
}

actor_signal_t actor_signal_pass_to_job(actor_t *actor, actor_signal_t signal, actor_job_t **job_slot, actor_thread_t *thread) {
    actor_job_t *job = *job_slot;
    if (job != NULL) {
        job->incoming_signal = signal;
        actor_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return 0;
}

actor_signal_t actor_signal_job(actor_t *actor, actor_signal_t signal, actor_job_t **job_slot) {
    actor_job_t *job = *job_slot;
    if (job != NULL) {
        job->incoming_signal = signal;
        return actor_job_execute(job_slot);
    }
    return 0;
}


actor_signal_t actor_job_publish_event_generic(actor_job_t *job, actor_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument) {
    job->outgoing_event =
        (actor_event_t){.type = type, .data = data, .size = size, .argument = argument, .consumer = target, .producer = job->actor};

    actor_publish(job->actor, &job->outgoing_event);
    return actor_job_wait(job);
}


actor_event_t actor_prepared_event;
actor_signal_t actor_publish_event_generic(actor_t *actor, actor_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument) {
    actor_prepared_event =
        (actor_event_t){.type = type, .data = data, .size = size, .argument = argument, .consumer = target, .producer = actor};
    return actor_publish(actor, &actor_prepared_event);
}

actor_signal_t actor_event_finalize(actor_t *actor, actor_event_t *event) {
    if (event != NULL && event->type != ACTOR_EVENT_IDLE) {
        if (event->type != ACTOR_EVENT_THREAD_ALARM) {
            debug_printf("│ │ ├ Finalize\t\t#%s of %s\n", get_actor_event_type_name(event->type),
                         actor_node_stringify(event->producer));

            actor_event_report_callback(actor, event);

            actor_event_set_status(actor, event, ACTOR_EVENT_FINALIZED);
        }

        memset(event, 0, sizeof(actor_event_t));
    }
    return ACTOR_SIGNAL_OK;
}

__attribute__((weak)) void actor_event_report_callback(actor_t *actor, actor_event_t *event) {

}