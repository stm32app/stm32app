#include <actor/lib/time.h>
#include <actor/thread.h>

#define MAX_THREAD_SLEEP_TIME_LONG (((uint32_t)-1) / 2)
#define MAX_THREAD_SLEEP_TIME 60000

static actor_signal_t actor_thread_message_dispatch(actor_thread_t* thread, actor_message_t* event);
static actor_signal_t actor_thread_message_actor_dispatch(actor_thread_t* thread,
                                                          actor_message_t* event,
                                                          actor_t* actor,
                                                          actor_worker_t* worker);
static size_t actor_thread_message_requeue(actor_thread_t* thread,
                                           actor_message_t* event,
                                           actor_message_status_t previous_status);
static inline bool actor_thread_message_queue_shift(actor_thread_t* thread,
                                                    actor_message_t* event,
                                                    size_t deferred_count);

/* A generic RTOS task function that handles all typical propertiesurations of threads. It supports
  following features or combinations of them:
  - Waking up actors on individual timer alarms to simulate delays and periodical work
  - Aborting or speeding up the timer alarms (with 1ms precision of FreeRTOS)
  - Yielding execution to other actors and getting it at the end of event queue
  - Broadcasting events either to specific actor, multiple listeners or first taker
  - Re-queuing events for later processing if a receiving actor was busy
  - Maintaining queue of events or lightweight event event mailbox slot.

  A node has at least five of these threads with different priorities. Devices declare "workers"
  which act as methods for a corresponding thread. This way actors can prioritize parts of their
  logic to play well in shared environment. For example actors may afford doing longer processing of
  data in background without blocking networking or handling inputs for the whole node. Another
  benefit of allowing actor to run logic with different priorities is responsiveness and
  flexibility. An high-priority input event may tell actor to abort longer task and do something
  else instead.

  A downside of multiple actors sharing the same thread, is that there is no automatic time-slicing
  of work. Whole thread (including processing of new events) is blocked until a actor finishes its
  work, unless a thread with higher priority tells the actor to stop. Devices still need to be
  mindful of blocking the cpu within a scope of its task priority. There can be a few solutions to
  this:
  - Devices can split their workload into chunks. Execution control would need to be yielded after
  each chunk. It can be done either by using alarm to schedule next worker in future (with 1ms
  resolution) or right after the thread goes through its queue of events (effectively simulating
  RTOS yield)
  - Devices can also create their own custom threads with own queues, leveraging all the dispatching
  and queue-management mechanisms available to node-wide threads. In this case RTOS will use
  time-slicing to periodically break up execution and allow other threads to work.
 */
void actor_thread_execute(actor_thread_t* thread) {
  thread->current_time = xTaskGetTickCount();
  thread->last_time = thread->current_time;
  size_t deferred_count = 0;  // Counter of re-queued events
  actor_message_t event = {.producer = actor_thread_get_root_actor(thread),
                           .type = ACTOR_MESSAGE_THREAD_START};

  while (true) {
    actor_message_status_t previous_message_status = event.status;

    // route event to its listeners
    actor_thread_message_dispatch(thread, &event);

    // keep event for later processing if needed
    deferred_count += actor_thread_message_requeue(thread, &event, previous_message_status);

    // move to the next event
    if (!actor_thread_message_queue_shift(thread, &event, deferred_count)) {
      // wait for external notifications or scheduled wakeup
      actor_thread_message_await(thread, &event);
    }
  }
}

/*
  Devices may indicate which types of events they want to be notified about. But there're also
  internal synthetic events that they receieve unconditionally.
*/
static inline bool actor_thread_should_notify_actor(actor_thread_t* thread,
                                                    actor_message_t* event,
                                                    actor_t* actor,
                                                    actor_worker_t* worker) {
  switch (event->type) {
    // Ticks get notified when thread starts and stops, so they can construct/destruct or schedule a
    // periodical alarm
    case ACTOR_MESSAGE_THREAD_STOP:
    case ACTOR_MESSAGE_THREAD_START:
      return true;

    // Ticks that set up software alarm will recieve schedule event in time
    case ACTOR_MESSAGE_THREAD_ALARM:
      return worker->next_time > 0 && time_gte(thread->current_time, worker->next_time);

    default:
      if (event->consumer == actor) {
        // Events targeted to a specific reciepeint will always be received
        return true;
      } else {
        // Devices have to subscribe to other types of events manually
        return actor_message_is_subscribed(actor, event);
      }
  }
}

/* Notify interested actors of a new event. Events may either be targeted to a specific actor, or
 * broadcasted to all/*/
static actor_signal_t actor_thread_message_dispatch(actor_thread_t* thread,
                                                    actor_message_t* event) {
  actor_signal_t signal = 0;

  for (size_t i = 0; i < thread->worker_count; i++) {
    // If event has no indicated reciepent, all actors that are subscribed to thread and event will
    // need to be notified
    actor_worker_t* worker =
        event->consumer ? actor_thread_worker_for(thread, event->consumer) : &thread->workers[i];

    signal = actor_thread_message_actor_dispatch(thread, event, worker->actor, worker);

    // let worker know that it has events to catch up later
    if (signal == ACTOR_SIGNAL_BUSY) {
      worker->catchup = thread;
    }

    // stop broadcasting if actor wants this event for itself
    if (event->consumer != NULL) {
      break;
    }
  }

  return signal;
}

/* In response to event, actor can request to:
 * - stop broadcasting event, so other actors will not receive it
 * - keep event in the queue, so this actor can process it later
 * - keep event in the queue for later processing, but allow other actors to handle it before that
 * - wake up on software timer at specific time in future */

static actor_signal_t actor_thread_message_actor_dispatch(actor_thread_t* thread,
                                                          actor_message_t* event,
                                                          actor_t* actor,
                                                          actor_worker_t* worker) {
  actor_signal_t signal = 0;

  if (actor_thread_should_notify_actor(thread, event, actor, worker)) {
    debug_printf("├ %-22s#%s from %s\n", actor_stringify(actor), actor_message_stringify(event),
                 actor_stringify(event->producer));

    if (event->type == ACTOR_MESSAGE_THREAD_ALARM) {
      worker->next_time = 0;
    }
    // Tick callback may change event status, set software timer or both
    signal = worker->callback(actor_unbox(actor), event, worker, thread);
    worker->last_time = thread->current_time;

    // Mark event as processed
    if (event->status == ACTOR_MESSAGE_WAITING) {
      event->status = ACTOR_MESSAGE_RECEIVED;
    }
  }

  // Device may request thread to wake up at specific time without waiting for external events by
  // settings next_time of its workers
  // - To wake up periodically actor should re-schedule its worker after each run
  // - To yield control until other events are processed actor should set schedule to current time
  // of a thread
  if (time_gt(thread->next_time, worker->next_time)) {
    thread->next_time = worker->next_time;
  }

  return signal;
}

/* Find a queue to insert deferred message to so it doesn't get lost. A thread may put it back to
 * its own queue at the end, put it into another thread queue, or even into a queue not owned by any
 * thread. This can be used to create lightweight threads that only have a notification mailbox
 * slot, but then can still retain events elsewhere.  */
static inline QueueHandle_t actor_thread_get_catchup_queue(actor_thread_t* thread) {
  return actor_thread_get_catchup_thread(thread)->queue;
}

/*
  A broadcasted event may be claimed for exclusive processing by actor that is too busy to process
  it right away. In that case event then gets pushed to the back of the queue, so the thread can
  keep on processing other events. When all is left in queue is deferred events, the thread will
  sleep and wait for either new events or notification from actor that is now ready to process a
  deferred event.
*/
static size_t actor_thread_message_requeue(actor_thread_t* thread,
                                           actor_message_t* event,
                                           actor_message_status_t previous_status) {
  QueueHandle_t queue;

  switch (event->status) {
    case ACTOR_MESSAGE_WAITING:
      if (event->type != ACTOR_MESSAGE_THREAD_ALARM) {
        debug_printf("No actors are listening to event: #%s\n", actor_message_stringify(event));
        actor_message_finalize(event->producer, event, ACTOR_SIGNAL_OK);
      }
      break;

    // Some busy actor wants to handle event later, so the event has to be re-queued
    case ACTOR_MESSAGE_DEFERRED:
    case ACTOR_MESSAGE_ADDRESSED:
      queue = actor_thread_get_catchup_queue(thread);
      if (queue != NULL) {
        // If event is reinserted back into the same queue it came from, the thread has
        // to keep the records about that to avoid popping the event right back
        if (queue == thread->queue) {
          event->status = ACTOR_MESSAGE_DEFERRED;
        } else {
          // Otherwise event status gets reset, so receiving thread has a chance to do its own
          // bookkeeping The thread will not wake up automatically, since that requires sending
          // notification
          event->status = ACTOR_MESSAGE_WAITING;
        }

        // If the target queue is full, event gets lost.
        if (xQueueSend(queue, event, 0)) {
          if (event->status == ACTOR_MESSAGE_DEFERRED &&
              previous_status != ACTOR_MESSAGE_DEFERRED) {
            return 1;
          }
        } else {
          debug_printf("The queue doesnt have any room for deferred event #%s\n",
                       actor_message_stringify(event));
        }
      } else {
        // Threads without a queue will leave event stored in the only available notification slot
        // New events will overwrite the deferred event that wasnt handled in time
      }
      break;

    // When a actor catches up with deferred event, it has to mark it as processed or else the event
    // will keep on being requeued
    case ACTOR_MESSAGE_HANDLED:
      if (previous_status == ACTOR_MESSAGE_DEFERRED && thread->queue) {
        return -1;
      }
      break;
    default:
      break;
  }

  return 0;
}

/*
  Thread ingests new event from queue and dispatches it for actors to handle.

  Unlike others, input thread off-loads its deferred events to a queue of a `catchup` thread to keep
  its own queue clean. This happens transparently to actors, as they dont know of that special
  thread existance.
  */
static inline bool actor_thread_message_queue_shift(actor_thread_t* thread,
                                                    actor_message_t* event,
                                                    size_t deferred_count) {
  // Threads without queues receieve their events via notification slot
  if (thread->queue == NULL) {
    return false;
  }

  // There is no need to shift queue if all events in the queue are deferred
  if (deferred_count > 0 && deferred_count == uxQueueMessagesWaiting(thread->queue)) {
    return false;
  }

  // Try getting an event out of the queue without blocking. Thread blocks on notification signal
  // instead
  if (!xQueueReceive(thread->queue, event, 0)) {
    // clear out previous event
    // *event = (actor_message_t) {};
    return false;
  }
  return true;
}

/*
  A thread goes to sleep to allow tasks with lower priority to run, when it does not have any events
  that can be processed right now.
  - Publishing new events also sends the notification for the thread to wake up. If an event was
  published to the back of the queue that had deferred events in it, the thread will need to rotate
  the whole queue to get to to the new events. This is due to FreeRTOS only allowing to take first
  event in the queue.
  - A actor that was previously busy can send a `ACTOR_SIGNAL_CATCHUP` notification, indicating that
  it is ready to catch up with events that it deferred previously. In that case thread will attempt
  to re-dispatch all the events in the queue.
*/
actor_signal_t actor_thread_message_await(actor_thread_t* thread, actor_message_t* event) {
  actor_signal_t signal = 0;

  for (bool stop = false; stop == false;) {
    // task gets blocked expecting notification, with a timeout to simulate software alarm.
    int32_t timeout = thread->next_time - thread->current_time;

    // if notifications keep being published, the timeout may not happen in time causing alarm to
    // lag behind. in that case timer has to be adjusted to be fired after all events and
    // notifications are processed.
    if (thread->next_time == 0) {
      timeout = portMAX_DELAY;
    } else if (timeout < 0) {
      timeout = 0;
    }

    // threads are expected to receive external notifications in order to wake up
    if (timeout > 0) {
      debug_printf("├ Sleep for %ims\n", (int)timeout);
    }

    uintptr_t notification = ulTaskNotifyTake(true, pdMS_TO_TICKS((uint32_t)timeout));

    switch (notification) {
      case ACTOR_SIGNAL_OK:
        // if no notification was recieved (value of signal being zero) within scheduled time,
        // it means that thread is woken up by schedule so synthetic wake up event is broadcasted
        *event = (actor_message_t){.producer = actor_thread_get_root_actor(thread),
                                   .type = ACTOR_MESSAGE_THREAD_ALARM};
        debug_printf(
            "├ Wakeup\t\tafter %lums (timeout of %lims)\n",
            (unsigned long)((xTaskGetTickCount() - thread->current_time) * portTICK_PERIOD_MS),
            (long)timeout);
        stop = true;
        break;

      case ACTOR_SIGNAL_RESCHEDULE:
        // something wants the thread to wake up earlier than its current schedule.
        // next_time must be already updated with new time, so the thread can just go into blocked
        // state again
        debug_printf("├ Reschedule\n");
        break;

      case ACTOR_SIGNAL_CATCHUP:
        // something tells thread that it is now ready to process messages that it asked to keep
        // deferred
        debug_printf("├ Catchup\n");
        __attribute__((fallthrough));
      default:
        // memory address in the notification slot means there's an incoming event in the queue
        if (notification > 50) {
          if (thread->queue != NULL) {
            // threads with queue that receieve notification expect a new message in queue
            if (!xQueueReceive(thread->queue, event, 0)) {
              debug_printf("├  Was woken up by notification but its queue is empty\n");
              continue;
            }
          } else {
            // if thread doesnt have queue, event is passed as address in notification slot instead.
            // publisher will have to ensure that event memory survives until thread is ready for
            // it.
            memcpy(event, (void*)notification, sizeof(actor_message_t));
          }
          // notifying with signal is used by custom threads
        } else {
          signal = notification;
        }
        stop = true;
    }
  }

  // current time will only update after waking up
  thread->last_time = thread->current_time;
  thread->current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

  if (thread->next_time != 0 && time_gt(thread->current_time, thread->next_time))
    thread->next_time = 0;

  return signal;
}

bool actor_thread_notify_generic(actor_thread_t* thread, uint32_t value, bool overwrite) {
  debug_printf("│ │ ├ Notify\t\t%s with #%s\n", actor_thread_get_name(thread),
               value < 50 ? actor_signal_stringify(value) : (char*)(&value));
  if (actor_thread_is_interrupted(thread)) {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    bool result = xTaskNotifyFromISR(thread->task, value,
                                     overwrite ? eSetValueWithOverwrite : eSetValueWithoutOverwrite,
                                     &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return result;
  } else {
    return xTaskNotify(thread->task, value,
                       overwrite ? eSetValueWithOverwrite : eSetValueWithoutOverwrite);
  }
}

bool actor_thread_publish_generic(actor_thread_t* thread, actor_message_t* event, bool to_front) {
  debug_printf("│ │ ├ Publish\t\t#%s to %s for %s on %s\n", actor_message_stringify(event),
               actor_stringify(event->producer),
               event->consumer ? actor_stringify(event->consumer) : "broadcast",
               actor_thread_get_name(thread));
  bool result = false;

  if (actor_thread_is_interrupted(thread)) {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (thread->queue == NULL ||
        xQueueGenericSendFromISR(thread->queue, event, &xHigherPriorityTaskWoken, to_front)) {
      result = xTaskNotifyFromISR(thread->task, (uintptr_t)event, eSetValueWithOverwrite,
                                  &xHigherPriorityTaskWoken);
    };
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  } else {
    if (thread->queue == NULL || xQueueGenericSend(thread->queue, event, 0, to_front)) {
      result = xTaskNotify(thread->task, (uintptr_t)event, eSetValueWithOverwrite);
    }
  }

  return result;
}

uint32_t actor_thread_delay(actor_thread_t* thread, uint32_t delay_ms) {
  uint32_t time = thread->current_time + delay_ms;
  if (time_gte(thread->next_time, time)) {
    thread->next_time = time_not_zero(time);
    // notification should be sent even if task has active status
    // i.e. a more important task may have taken over the less important task
    // while it was on its way to become blocked, creating a race condition
    // eTaskState state = eTaskGetState(thread->task);
    // if (state == eBlocked || state == eReady) {
    actor_thread_reschedule(thread);
    //}
  }
  return time;
}

uint32_t actor_worker_delay(actor_worker_t* worker, uint32_t delay_ms) {
  uint32_t time = worker->thread->current_time + delay_ms;
  if (time_gt(worker->next_time, time)) {
    worker->next_time = time_not_zero(worker->thread->current_time + delay_ms);
  }
  actor_thread_delay(worker->thread, delay_ms);
  return time;
}

actor_thread_t* actor_thread_create(actor_thread_t** thread_slot,
                                    actor_t* actor,
                                    void (*callback)(void* ptr),
                                    const char* const name,
                                    uint16_t stack_depth,
                                    size_t queue_size,
                                    size_t priority,
                                    uint8_t flags) {
  actor_thread_t* thread = actor_thread_alloc(actor);
  if (thread != NULL) {
    *thread_slot = thread;
    *thread = (actor_thread_t){.actor = actor, .flags = flags};

    // initialize workers for all listening actors
    thread->worker_count = actor_thread_iterate_workers(thread, NULL);
    if (thread->worker_count > 0) {
      thread->workers =
          actor_calloc_region(ACTOR_REGION_FAST, thread->worker_count, sizeof(actor_worker_t));

      if (thread->workers) {
        actor_thread_iterate_workers(thread, thread->workers);

        // initialize task & queue
        xTaskCreate(callback, name, stack_depth, (void*)thread, priority, (void*)&thread->task);

        if (thread->task != NULL) {
          if (queue_size > 0) {
            thread->queue = xQueueCreate(queue_size, sizeof(actor_message_t));
          }
          if (queue_size == 0 || thread->queue != NULL) {
            return thread;
          }
        }
      }
    }

    // clean up whatever has been allocated
    actor_thread_destroy(thread);
    *thread_slot = NULL;
  }
  return NULL;
}

void actor_thread_destroy(actor_thread_t* thread) {
  if (thread != NULL) {
    if (thread->queue != NULL) {
      vQueueDelete(thread->queue);
    }
    if (thread->task) {
      vTaskDelete(thread->task);
    }
    actor_thread_free(thread);
  }
}

actor_worker_t* actor_thread_worker_for(actor_thread_t* thread, actor_t* actor) {
  for (uint16_t i = 0; i < thread->worker_count; i++) {
    if ((&thread->workers[i])->actor == actor) {
      return &thread->workers[i];
    }
  }
  return NULL;
}

char* actor_thread_get_name(actor_thread_t* thread) {
  return pcTaskGetTaskName(thread->task);
}

void actor_enter_critical(void) {
  actor_disable_interrupts();
}
void actor_exit_critical(void) {
  actor_enable_interrupts();
}
void actor_exit_critical_cleanup(void *value) {
  actor_exit_critical();
}

bool actor_thread_is_blockable(actor_thread_t* thread) {
  return thread->flags & ACTOR_THREAD_BLOCKABLE;
}

bool actor_thread_is_current(actor_thread_t* thread) {
  return false;
}

__attribute__((weak)) uint32_t actor_thread_iterate_workers(actor_thread_t* thread,
                                                            actor_worker_t* destination) {
  return 0;
}

__attribute__((weak)) actor_t* actor_thread_get_root_actor(actor_thread_t* thread) {
  return thread->actor;
}

__attribute__((weak)) actor_thread_t* actor_thread_alloc(actor_t* actor) {
  return actor_malloc_region(ACTOR_REGION_FAST, sizeof(actor_thread_t));
}

__attribute__((weak)) void actor_thread_free(actor_thread_t* thread) {
  actor_free(thread);
}

__attribute__((weak)) actor_thread_t* actor_thread_get_catchup_thread(actor_thread_t* thread) {
  return thread;
}

__attribute__((weak)) bool actor_thread_is_interrupted(actor_thread_t* thread) {
  return false;
}

__attribute__((weak)) actor_thread_t* actor_get_input_thread(actor_t* actor) {
  return NULL;  // todo: throw
}

__attribute__((weak)) bool actor_message_is_subscribed(actor_t* actor, actor_message_t* event) {
  return true;
}

__attribute__((weak)) void* pvPortMalloc(size_t size) {
  return actor_malloc_region(ACTOR_REGION_FAST, size);
}
__attribute__((weak)) void vPortFree(void* ptr) {
  return actor_free(ptr);
}
