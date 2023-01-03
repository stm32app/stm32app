#include <actor/buffer.h>
#include <actor/job.h>
#include <actor/message.h>
#include <actor/thread.h>

static void actor_message_set_status(actor_t* actor,
                                     actor_message_t* event,
                                     actor_message_status_t status) {
  // when actor accepted event that had `actor_buffer_t` struct as a data,...
  if (event->size == ACTOR_BUFFER_DYNAMIC_SIZE) {
    switch (status) {
      // the semaphore is icnremented to prevent deallocation
      case ACTOR_MESSAGE_RECEIVED:
      case ACTOR_MESSAGE_HANDLED:
      case ACTOR_MESSAGE_DEFERRED:
        // handled & deferred statuses make event not broadcast to other actors
        if (status != ACTOR_MESSAGE_RECEIVED) {
          event->consumer = actor;
        }
        // if previous status was deferred, the users counter was already incremented
        if (event->status != ACTOR_MESSAGE_DEFERRED) {
          actor_buffer_use((actor_buffer_t*)event->data, actor);
        }
        break;
      // when processing is done the user counter gets decremented
      case ACTOR_MESSAGE_FINALIZED:
        actor_buffer_release((actor_buffer_t*)event->data, actor);
        break;
      default:
        break;
    }
  }
  event->status = status;
}

actor_signal_t actor_message_accept_and_process_generic(actor_t* actor,
                                                        actor_message_t* event,
                                                        actor_message_t* destination,
                                                        actor_message_status_t ready_status,
                                                        actor_message_status_t busy_status,
                                                        actor_on_message_t handler) {
  // Check if destination can store the incoming event, otherwise actor will be considered busy
  if (destination == NULL || destination->type == ACTOR_MESSAGE_IDLE) {
    actor_message_set_status(actor, event, ready_status);
    memcpy(destination, event, sizeof(actor_message_t));
    if (handler != NULL) {
      return handler(actor_unbox(actor), destination);
    } else {
      return ACTOR_SIGNAL_OK;
    }
  } else {
    actor_message_set_status(actor, event, busy_status);
    return ACTOR_SIGNAL_BUSY;
  }
}

actor_signal_t actor_message_accept_and_start_job_generic(actor_t* actor,
                                                          actor_message_t* event,
                                                          actor_job_t** job_slot,
                                                          actor_thread_t* thread,
                                                          actor_job_handler_t handler,
                                                          actor_message_status_t ready_status,
                                                          actor_message_status_t busy_status) {
  if (*job_slot == NULL)
    *job_slot = actor_job_alloc(actor);
  actor_job_t* job = *job_slot;
  if (!job) {
    return busy_status;
  }
  actor_signal_t signal = actor_message_accept_and_process_generic(
      actor, event, &job->inciting_message, ready_status, busy_status, NULL);
  if (signal == ACTOR_SIGNAL_OK) {
    job->actor = actor;
    job->thread = thread;
    job->handler = handler;
    job->pointer = job_slot;
    debug_printf("| ├ Task start\t\t%s for %s\t\n", actor_stringify(actor),
                 actor_thread_get_name(thread));
    if (actor_thread_is_current(thread)) {
      actor_job_execute(job, ACTOR_SIGNAL_OK, event->producer);
    } else {
      actor_job_wakeup(job);
    }
  }
  return signal;
}

actor_signal_t actor_message_accept_and_pass_to_job_generic(actor_t* actor,
                                                            actor_message_t* event,
                                                            actor_job_t* job,
                                                            actor_thread_t* thread,
                                                            actor_job_handler_t handler,
                                                            actor_message_status_t ready_status,
                                                            actor_message_status_t busy_status) {
  actor_assert(job);
  // if currently running task is different , defer the event
  if (job->handler != handler) {
    actor_message_set_status(actor, event, busy_status);
    return ACTOR_SIGNAL_BUSY;
  }
  actor_signal_t signal = actor_message_accept_and_process_generic(
      actor, event, &job->incoming_message, ready_status, busy_status, NULL);
  if (signal == ACTOR_SIGNAL_OK) {
    actor_job_execute(job, ACTOR_SIGNAL_OK, event->producer);
  }
  return signal;
}

actor_signal_t actor_job_publish_message_generic(actor_job_t* job,
                                                 actor_message_type_t type,
                                                 actor_t* target,
                                                 uint8_t* data,
                                                 uint32_t size,
                                                 void* argument) {
  actor_publish(job->actor, &((actor_message_t){
                                .type = type,
                                .data = data,
                                .size = size,
                                .argument = argument,
                                .consumer = target,
                                .producer = job->actor,
                            }));
  return actor_job_wait(job);
}

actor_signal_t actor_publish_message_generic(actor_t* actor,
                                             actor_message_type_t type,
                                             actor_t* target,
                                             uint8_t* data,
                                             uint32_t size,
                                             void* argument) {
  return actor_publish(actor, &((actor_message_t){.type = type,
                                             .data = data,
                                             .size = size,
                                             .argument = argument,
                                             .consumer = target,
                                             .producer = actor}));
}

actor_signal_t actor_message_finalize(actor_t* actor,
                                      actor_message_t* event,
                                      actor_signal_t signal) {
  if (event != NULL && event->type != ACTOR_MESSAGE_IDLE) {
    if (event->type != ACTOR_MESSAGE_THREAD_ALARM) {
      debug_printf("│ │ ├ Finalize\t\t#%s of %s\n", actor_message_stringify(event),
                   actor_stringify(event->producer));

      actor_callback_message_handled(actor, event, signal);

      actor_message_set_status(actor, event, ACTOR_MESSAGE_FINALIZED);
    }

    memset(event, 0, sizeof(actor_message_t));
  }
  return ACTOR_SIGNAL_OK;
}

__attribute__((weak)) void actor_callback_message_handled(actor_t* actor,
                                                          actor_message_t* event,
                                                          actor_signal_t signal) {}