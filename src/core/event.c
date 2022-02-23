#include "event.h"
#include "core/buffer.h"
#include "core/actor.h"



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
                                                    actor_on_event_report_t handler) {
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
                                                      actor_on_job_complete_t handler, app_event_status_t ready_status,
                                                      app_event_status_t busy_status) {
    app_signal_t signal = actor_event_accept_and_process_generic(actor, event, &job->inciting_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        job->actor = actor;
        job->handler = handler;
        memcpy(&job->inciting_event, event, sizeof(app_event_t));
        memset(&job->incoming_event, 0, sizeof(app_event_t));
        job->task_phase = job->job_phase = 0;
        job->thread = thread;
        job->counter = 0;
        job->incoming_signal = APP_SIGNAL_PENDING;
        debug_printf("| ├ Task start\t\t%s for %s\t\n", get_actor_type_name(actor->class->type), app_thread_get_name(thread));
        app_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
}

app_signal_t actor_event_accept_and_pass_to_job_generic(actor_t *actor, app_event_t *event, app_job_t *job, app_thread_t *thread,
                                                        actor_on_job_complete_t handler, app_event_status_t ready_status,
                                                        app_event_status_t busy_status) {
    if (job->handler != handler) {
        actor_event_set_status(actor, event, busy_status);
        return APP_SIGNAL_BUSY;
    }
    app_signal_t signal = actor_event_accept_and_process_generic(actor, event, &job->incoming_event, ready_status, busy_status, NULL);
    if (signal == APP_SIGNAL_OK) {
        app_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return signal;
}

app_signal_t actor_event_accept_for_job_generic(actor_t *actor, app_event_t *event, app_job_t *job, app_thread_t *thread,
                                                actor_on_job_complete_t handler, app_event_status_t ready_status, app_event_status_t busy_status) {
    if (job->handler == NULL) {
        return actor_event_accept_and_start_job_generic(actor, event, job, thread, handler, ready_status, busy_status);
    } else {
        return actor_event_accept_and_pass_to_job_generic(actor, event, job, thread, handler, ready_status, busy_status);
    }
}

app_signal_t actor_signal_pass_to_job(actor_t *actor, app_signal_t signal, app_job_t *job, app_thread_t *thread) {
    if (job->handler != NULL) {
        job->incoming_signal = signal;
        app_thread_actor_schedule(thread, actor, thread->current_time);
    }
    return 0;
}

app_signal_t actor_signal_job(actor_t *actor, app_signal_t signal, app_job_t *job) {
    if (job->handler != NULL) {
        job->incoming_signal = signal;
        return app_job_execute(job);
    }
    return 0;
}


app_signal_t app_job_publish_event_generic(app_job_t *job, app_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument) {
    app_prepared_event =
        (app_event_t){.type = type, .data = data, .size = size, .argument = argument, .consumer = target, .producer = actor};
    return app_publish(actor->app, &app_prepared_event);
}


app_event_t app_prepared_event;
app_signal_t actor_publish_event_generic(actor_t *actor, app_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument) {
    app_prepared_event =
        (app_event_t){.type = type, .data = data, .size = size, .argument = argument, .consumer = target, .producer = actor};
    return app_publish(actor->app, &app_prepared_event);
}

app_signal_t actor_event_report(actor_t *actor, app_event_t *event) {
    (void)actor;
    if (event->producer && event->producer != actor && event->producer->class->on_event_report) {
        return event->producer->class->on_event_report(event->producer->object, event);
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
