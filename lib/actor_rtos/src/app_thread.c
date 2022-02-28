#include "app_rtos_macros.h"
#include <app_thread.h>

#define MAX_THREAD_SLEEP_TIME_LONG (((uint32_t)-1) / 2)
#define MAX_THREAD_SLEEP_TIME 60000

static app_signal_t app_thread_event_dispatch(app_thread_t *thread, app_event_t *event);
static app_signal_t app_thread_event_actor_dispatch(app_thread_t *thread, app_event_t *event, actor_t *actor, actor_worker_t *worker);
static size_t app_thread_event_requeue(app_thread_t *thread, app_event_t *event, app_event_status_t previous_status);
static inline bool_t app_thread_event_queue_shift(app_thread_t *thread, app_event_t *event, size_t deferred_count);

/* A generic RTOS task function that handles all typical propertiesurations of threads. It supports following features or combinations of
  them:
  - Waking up actors on individual timer alarms to simulate delays and periodical work
  - Aborting or speeding up the timer alarms (with 1ms precision of FreeRTOS)
  - Yielding execution to other actors and getting it at the end of event queue
  - Broadcasting events either to specific actor, multiple listeners or first taker
  - Re-queuing events for later processing if a receiving actor was busy
  - Maintaining queue of events or lightweight event event mailbox slot.

  An app has at least five of these threads with different priorities. Devices declare "workers" which act as methods for a corresponding
  thread. This way actors can prioritize parts of their logic to play well in shared environment. For example actors may afford doing
  longer processing of data in background without blocking networking or handling inputs for the whole app. Another benefit of
  allowing actor to run logic with different priorities is responsiveness and flexibility. An high-priority input event may tell actor to
  abort longer task and do something else instead.

  A downside of multiple actors sharing the same thread, is that there is no automatic time-slicing of work. Whole thread (including
  processing of new events) is blocked until a actor finishes its work, unless a thread with higher priority tells the actor to stop.
  Devices still need to be mindful of blocking the cpu within a scope of its task priority. There can be a few solutions to this:
  - Devices can split their workload into chunks. Execution control would need to be yielded after each chunk. It can be done
  either by using alarm to schedule next worker in future (with 1ms resolution) or right after the thread goes through its queue of events
  (effectively simulating RTOS yield)
  - Devices can also create their own custom threads with own queues, leveraging all the dispatching and queue-management mechanisms
  available to app-wide threads. In this case RTOS will use time-slicing to periodically break up execution and allow other threads to work.
 */
void app_thread_execute(app_thread_t *thread) {
    thread->current_time = xTaskGetTickCount();
    thread->last_time = thread->current_time;
    thread->next_time = thread->current_time + MAX_THREAD_SLEEP_TIME;
    size_t deferred_count = 0; // Counter of re-queued events
    app_event_t event = {.producer = app_thread_get_root_actor(thread), .type = APP_EVENT_THREAD_ALARM};

    while (true) {
        app_event_status_t previous_event_status = event.status;

        // route event to its listeners
        app_thread_event_dispatch(thread, &event);

        // keep event for later processing if needed
        deferred_count += app_thread_event_requeue(thread, &event, previous_event_status);

        // move to the next event
        if (!app_thread_event_queue_shift(thread, &event, deferred_count)) {
            // wait for external notifications or scheduled wakeup
            app_thread_event_await(thread, &event);
        }
    }
}

/*
  Devices may indicate which types of events they want to be notified about. But there're also internal synthetic events that they
  receieve unconditionally.
*/
static inline bool_t app_thread_should_notify_actor(app_thread_t *thread, app_event_t *event, actor_t *actor, actor_worker_t *worker) {
    switch (event->type) {
    // Ticks get notified when thread starts and stops, so they can construct/destruct or schedule a periodical alarm
    case APP_EVENT_THREAD_STOP:
        return true;

    // Ticks that set up software alarm will recieve schedule event in time
    case APP_EVENT_THREAD_ALARM:
        return worker->next_time <= thread->current_time;

    default:
        if (event->consumer == actor) {
            // Events targeted to a specific reciepeint will always be received
            return true;
        } else {
            // Devices have to subscribe to other types of events manually
            return actor_event_is_subscribed(actor, event);
        }
    }
}

/* Notify interested actors of a new event. Events may either be targeted to a specific actor, or broadcasted to all/*/
static app_signal_t app_thread_event_dispatch(app_thread_t *thread, app_event_t *event) {
    app_signal_t signal = 0;

    for (size_t i = 0; i < thread->worker_count; i++) {
        // If event has no indicated reciepent, all actors that are subscribed to thread and event will need to be notified
        actor_worker_t *worker = event->consumer ? app_thread_worker_for(thread, event->consumer) : &thread->workers[i];

        signal = app_thread_event_actor_dispatch(thread, event, worker->actor, worker);

        // let worker know that it has events to catch up later
        if (signal == APP_SIGNAL_BUSY) {
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

static app_signal_t app_thread_event_actor_dispatch(app_thread_t *thread, app_event_t *event, actor_t *actor, actor_worker_t *worker) {
    app_signal_t signal = 0;

    if (app_thread_should_notify_actor(thread, event, actor, worker)) {
        debug_printf("├ %-22s#%s from %s\n", app_actor_stringify(actor), get_app_event_type_name(event->type),
                     app_actor_stringify(event->producer));

        if (event->type == APP_EVENT_THREAD_ALARM || event->type == APP_EVENT_THREAD_START) {
            worker->next_time = -1;
        }
        // Tick callback may change event status, set software timer or both
        signal = worker->callback(actor_unbox(actor), event, worker, thread);
        worker->last_time = thread->current_time;

        // Mark event as processed
        if (event->status == APP_EVENT_WAITING) {
            event->status = APP_EVENT_RECEIVED;
        }
    }

    // Device may request thread to wake up at specific time without waiting for external events by settings next_time of its workers
    // - To wake up periodically actor should re-schedule its worker after each run
    // - To yield control until other events are processed actor should set schedule to current time of a thread

    // Fixme: breaks something :/
    if (worker->next_time != 0 && thread->next_time > worker->next_time) {
        thread->next_time = worker->next_time;
    }

    return signal;
}

/* Find a queue to insert deferred message to so it doesn't get lost. A thread may put it back to its own queue at the end, put it into
 * another thread queue, or even into a queue not owned by any thread. This can be used to create lightweight threads that only have a
 * notification mailbox slot, but then can still retain events elsewhere.  */
static inline QueueHandle_t app_thread_get_catchup_queue(app_thread_t *thread) {
    return app_thread_get_catchup_thread(thread)->queue;
}

/*
  A broadcasted event may be claimed for exclusive processing by actor that is too busy to process it right away. In that case event
  then gets pushed to the back of the queue, so the thread can keep on processing other events. When all is left in queue is deferred
  events, the thread will sleep and wait for either new events or notification from actor that is now ready to process a deferred event.
*/
static size_t app_thread_event_requeue(app_thread_t *thread, app_event_t *event, app_event_status_t previous_status) {
    QueueHandle_t queue;

    switch (event->status) {
    case APP_EVENT_WAITING:
        if (event->type != APP_EVENT_THREAD_ALARM) {
            debug_printf("No actors are listening to event: #%s\n", get_app_event_type_name(event->type));
            actor_event_finalize(event->producer, event);
        }
        break;

    // Some busy actor wants to handle event later, so the event has to be re-queued
    case APP_EVENT_DEFERRED:
    case APP_EVENT_ADDRESSED:
        queue = app_thread_get_catchup_queue(thread);
        if (queue != NULL) {
            // If event is reinserted back into the same queue it came from, the thread has
            // to keep the records about that to avoid popping the event right back
            if (queue == thread->queue) {
                event->status = APP_EVENT_DEFERRED;
            } else {
                // Otherwise event status gets reset, so receiving thread has a chance to do its own bookkeeping
                // The thread will not wake up automatically, since that requires sending notification
                event->status = APP_EVENT_WAITING;
            }

            // If the target queue is full, event gets lost.
            if (xQueueSend(queue, event, 0)) {
                if (event->status == APP_EVENT_DEFERRED && previous_status != APP_EVENT_DEFERRED) {
                    return 1;
                }
            } else {
                debug_printf("The queue doesnt have any room for deferred event #%s\n", get_app_event_type_name(event->type));
            }
        } else {
            // Threads without a queue will leave event stored in the only available notification slot
            // New events will overwrite the deferred event that wasnt handled in time
        }
        break;

    // When a actor catches up with deferred event, it has to mark it as processed or else the event
    // will keep on being requeued
    case APP_EVENT_HANDLED:
        if (previous_status == APP_EVENT_DEFERRED && thread->queue) {
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

  Unlike others, input thread off-loads its deferred events to a queue of a `catchup` thread to keep its own queue clean. This happens
  transparently to actors, as they dont know of that special thread existance.
  */
static inline bool_t app_thread_event_queue_shift(app_thread_t *thread, app_event_t *event, size_t deferred_count) {
    // Threads without queues receieve their events via notification slot
    if (thread->queue == NULL) {
        return false;
    }

    // There is no need to shift queue if all events in the queue are deferred
    if (deferred_count > 0 && deferred_count == uxQueueMessagesWaiting(thread->queue)) {
        return false;
    }

    // Try getting an event out of the queue without blocking. Thread blocks on notification signal instead
    if (!xQueueReceive(thread->queue, event, 0)) {
        // clear out previous event
        // *event = (app_event_t) {};
        return false;
    }
    return true;
}

/*
  A thread goes to sleep to allow tasks with lower priority to run, when it does not have any events that can be processed right now.
  - Publishing new events also sends the notification for the thread to wake up. If an event was published to the back of the queue that
  had deferred events in it, the thread will need to rotate the whole queue to get to to the new events. This is due to FreeRTOS only
  allowing to take first event in the queue.
  - A actor that was previously busy can send a `APP_SIGNAL_CATCHUP` notification, indicating that it is ready to catch up with events
    that it deferred previously. In that case thread will attempt to re-dispatch all the events in the queue.
*/
app_signal_t app_thread_event_await(app_thread_t *thread, app_event_t *event) {
    app_signal_t signal = 0;

    for (bool_t stop = false; stop == false;) {
        // task gets blocked expecting notification, with a timeout to simulate software alarm.
        int32_t timeout = thread->next_time - thread->current_time;

        // if notifications keep being published, the timeout may not happen in time causing alarm to lag behind.
        // in that case timer has to be adjusted to be fired after all events and notifications are processed.
        if (timeout < 0) {
            timeout = 0;
        }

        // threads are expected to receive external notifications in order to wake up
        if (timeout > 0) {
            debug_printf("├ Sleep for %ims\n", (int)timeout);
        }

        uintXX_t notification = ulTaskNotifyTake(true, pdMS_TO_TICKS((uint32_t)timeout));

        switch (notification) {
        case APP_SIGNAL_OK:
            // if no notification was recieved (value of signal being zero) within scheduled time,
            // it means that thread is woken up by schedule so synthetic wake up event is broadcasted
            *event = (app_event_t){.producer = app_thread_get_root_actor(thread), .type = APP_EVENT_THREAD_ALARM};
            debug_printf("├ Wakeup\t\tafter %lims (timeout of %lims)\n",
                         ((xTaskGetTickCount() - thread->current_time) * portTICK_PERIOD_MS), timeout);
            stop = true;
            break;

        case APP_SIGNAL_RESCHEDULE:
            // something wants the thread to wake up earlier than its current schedule.
            // next_time must be already updated with new time, so the thread can just go into blocked state again
            debug_printf("├ Reschedule\n");
            break;

        case APP_SIGNAL_CATCHUP:
            // something tells thread that it is now ready to process messages that it asked to keep deferred
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
                    // publisher will have to ensure that event memory survives until thread is ready for it.
                    memcpy(event, (void *)notification, sizeof(app_event_t));
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

    if (thread->next_time <= thread->current_time)
        thread->next_time = thread->current_time + MAX_THREAD_SLEEP_TIME;

    return signal;
}

bool_t app_thread_notify_generic(app_thread_t *thread, uint32_t value, bool_t overwrite) {
    debug_printf("│ │ ├ Notify\t\t%s with #%s\n", app_thread_get_name(thread), value < 50 ? get_app_signal_name(value) : (char *)(&value));
    if (app_thread_is_interrupted(thread)) {
        static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        bool_t result = xTaskNotifyFromISR(thread->task, value, overwrite ? eSetValueWithOverwrite : eSetValueWithoutOverwrite,
                                           &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        return result;
    } else {
        return xTaskNotify(thread->task, value, overwrite ? eSetValueWithOverwrite : eSetValueWithoutOverwrite);
    }
}

bool_t app_thread_publish_generic(app_thread_t *thread, app_event_t *event, bool_t to_front) {
    debug_printf("│ │ ├ Publish\t\t#%s to %s for %s on %s\n", get_app_event_type_name(event->type), app_actor_stringify(event->producer),
                 event->consumer ? app_actor_stringify(event->consumer) : "broadcast", app_thread_get_name(thread));
    bool_t result = false;

    if (app_thread_is_interrupted(thread)) {
        static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (thread->queue == NULL || xQueueGenericSendFromISR(thread->queue, event, &xHigherPriorityTaskWoken, to_front)) {
            result = xTaskNotifyFromISR(thread->task, (uintXX_t)event, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
        };
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        if (thread->queue == NULL || xQueueGenericSend(thread->queue, event, 0, to_front)) {
            result = xTaskNotify(thread->task, (uintXX_t)event, eSetValueWithOverwrite);
        }
    }

    return result;
}

void app_thread_schedule(app_thread_t *thread, uint32_t time) {
    if (thread->next_time > time) {
        thread->next_time = time;
        // notification should be sent even if task has active status
        // i.e. a more important task may have taken over the less important task
        // while it was on its way to become blocked, creating a race condition
        // eTaskState state = eTaskGetState(thread->task);
        // if (state == eBlocked || state == eReady) {
        app_thread_reschedule(thread);
        //}
    }
}
void app_thread_worker_schedule(app_thread_t *thread, actor_worker_t *worker, uint32_t time) {
    worker->next_time = time;
    app_thread_schedule(thread, time);
}

app_thread_t *app_thread_create(app_thread_t **thread_slot, actor_t *actor, void (*callback)(void *ptr), const char *const name,
                                  uint16_t stack_depth, size_t queue_size, size_t priority, uint8_t flags) {
    app_thread_t *thread = app_thread_alloc(actor);
    if (thread != NULL) {
        *thread_slot = thread;

        thread->actor = actor;
        thread->flags = flags;

        // initialize workers for all listneing actors
        thread->worker_count = app_thread_iterate_workers(thread, NULL);
        if (thread->worker_count > 0) {
            thread->workers = app_malloc(thread->worker_count * sizeof(actor_worker_t));
            memset(thread->workers, 0, thread->worker_count * sizeof(actor_worker_t));

            if (thread->workers) {
                app_thread_iterate_workers(thread, thread->workers);

                // initialize task & queue
                xTaskCreate(callback, name, stack_depth, (void *)thread, priority, (void *)&thread->task);

                if (thread->task != NULL) {
                    if (queue_size > 0) {
                        thread->queue = xQueueCreate(queue_size, sizeof(app_event_t));
                    }
                    if (queue_size == 0 || thread->queue != NULL) {
                        return thread;
                    }
                }
            }
        }

        // clean up whatever has been allocated
        app_thread_destroy(thread);
        *thread_slot = NULL;
    }
    return NULL;
}

void app_thread_destroy(app_thread_t *thread) {
    if (thread != NULL) {
        if (thread->queue != NULL) {
            vQueueDelete(thread->queue);
        }
        if (thread->task) {
            vTaskDelete(thread->task);
        }
        app_thread_free(thread);
    }
}

actor_worker_t *app_thread_worker_for(app_thread_t *thread, actor_t *actor) {
    for (uint16_t i = 0; i < thread->worker_count; i++) {
        if ((&thread->workers[i])->actor == actor) {
            return &thread->workers[i];
        }
    }
    return NULL;
}

char *app_thread_get_name(app_thread_t *thread) {
    return pcTaskGetTaskName(thread->task);
}

bool_t app_thread_is_blockable(app_thread_t *thread) {
    return thread->flags & APP_THREAD_BLOCKABLE;
}

__attribute__((weak)) uint32_t app_thread_iterate_workers(app_thread_t *thread, actor_worker_t *destination) {
    return 0;
}

__attribute__((weak)) actor_t *app_thread_get_root_actor(app_thread_t *thread) {
    return thread->actor;
}

__attribute__((weak)) app_thread_t *app_thread_alloc(actor_t *actor) {
    return app_malloc(sizeof(app_thread_t));
}

__attribute__((weak)) void app_thread_free(app_thread_t *thread) {
    app_free(thread);
}

__attribute__((weak)) app_thread_t *app_thread_get_catchup_thread(app_thread_t *thread) {
    return thread;
}

__attribute__((weak)) bool_t app_thread_is_interrupted(app_thread_t *thread) {
    return false;
}

__attribute__((weak)) app_thread_t* actor_get_input_thread(actor_t *actor) {
    return NULL; //todo: throw
}

__attribute__((weak)) bool_t actor_event_is_subscribed(actor_t *actor, app_event_t *event) {
    return true;
}