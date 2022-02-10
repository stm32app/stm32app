#ifndef INC_CORE_THREAD
#define INC_CORE_THREAD

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "core/actor.h"
#include "core/types.h"

#define TIME_DIFF(now, last) (last > now ? ((uint32_t)-1) - last + now : now - last)

struct app_thread {
    actor_t *actor;
    uint32_t last_time;
    uint32_t current_time;
    uint32_t next_time;
    // size_t index; /* Corresponding handler in actor_ticks struct */
    TaskHandle_t task;
    QueueHandle_t queue;
    void *argument;
};

struct app_threads {
    app_thread_t *input;           /* Thread with queue of events that need immediate response*/
    app_thread_t *catchup;         /* Allow actors that were busy to catch up with input events  */
    app_thread_t *high_priority;   /* Logic that is scheduled by actors themselves */
    app_thread_t *medium_priority; /* A queue of events that concerns medium_priorityting  outside */
    app_thread_t *low_priority;    /* Logic that runs periodically that is not very important */
    app_thread_t *bg_priority;     /* A background thread of sorts for work that can be done in free time */
};

struct actor_tick {
    uint32_t last_time;    /* Previous time the tick fired */
    uint32_t next_time;    /* Next time tick will fire */
    actor_t *next_actor; /* Next actor having the same tick */
    actor_t *prev_actor; /* Previous actor handling the same tick*/
    app_thread_t *catchup; /* Did tick miss any messages? */
    actor_on_tick_t callback;
};

struct actor_ticks {
    actor_tick_t *input;           /* Logic that processes input and immediate work */
    actor_tick_t *high_priority;   /* Asynchronous work that needs to be done later */
    actor_tick_t *medium_priority; /* Asynchronous work that needs to be done later */
    actor_tick_t *low_priority;    /* Logic that runs periodically even when there is no input*/
    actor_tick_t *bg_priority;     /* Lowest priority work that is done when there isn't anything more important */
};

int actor_tick_allocate(actor_tick_t **destination, actor_on_tick_t callback);
void actor_tick_free(actor_tick_t **tick);

int actor_ticks_allocate(actor_t *actor);
int actor_ticks_free(actor_t *actor);

int app_thread_allocate(app_thread_t **thread, void *app_or_object, void (*callback)(void *ptr), const char *const name,
                        uint16_t stack_depth, size_t queue_size, size_t priority, void *argument);
void actor_tick_initialize(actor_tick_t *tick);

int app_thread_free(app_thread_t **thread);

/* Find out numeric index of a tick that given standard thread handles  */
size_t app_thread_get_tick_index(app_thread_t *thread);

int app_threads_allocate(app_t *app);

int app_threads_free(app_t *app);

/* Find app actors that subscribe to given thread. Joins matching actor tick handlers in a linked list, a returns first actor as a list
 * head*/
actor_t *app_thread_filter_actors(app_thread_t *thread);
actor_tick_t *actor_tick_for_thread(actor_t *actor, app_thread_t *thread);

/*
Callback methods that is invoked as FreeRTOS tasks.
Typical task flow: Allow actors to schedule next tick, or be woken up on semaphore
*/
void app_thread_execute(app_thread_t *thread);

/* publish a new event to the queue and notify the thread to wake up*/
bool_t app_thread_publish_generic(app_thread_t *thread, app_event_t *event, bool_t to_front);

/* Wake up thread to run asap without waiting for next tick*/
bool_t app_thread_notify_generic(app_thread_t *thread, uint32_t value, bool_t overwrite);

app_thread_t *app_thread_get_catchup_thread(app_thread_t *thread);

void app_thread_schedule(app_thread_t *thread, uint32_t time);
void app_thread_tick_schedule(app_thread_t *thread, actor_tick_t *tick, uint32_t time);
void app_thread_actor_schedule(app_thread_t *thread, actor_t *actor, uint32_t time);

#define app_thread_catchup(thread) app_thread_notify_generic(app_thread_get_catchup_thread(thread), APP_SIGNAL_CATCHUP, false);
#define app_thread_reschedule(thread) app_thread_notify_generic(thread, APP_SIGNAL_RESCHEDULE, false);

#define app_thread_notify(thread, signal) app_thread_notify_generic(thread, (uint32_t)argument, true);

#define app_thread_publish_to_front(thread, event) app_thread_publish_generic(thread, event, 1)
#define app_thread_publish(thread, event) app_thread_publish_generic(thread, event, 0)
#define app_publish(app, event) app_thread_publish_generic(app->threads->input, event, 0);

#define app_thread_actor_schedule(thread, actor, time) app_thread_tick_schedule(thread, actor_tick_for_thread(actor, thread), time);

char *app_thread_get_name(app_thread_t *thread);

#ifdef __cplusplus
}
#endif

#endif