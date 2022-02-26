#ifndef INC_CORE_THREAD
#define INC_CORE_THREAD

#ifdef __cplusplus
extern "C" {
#endif

#include "env.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "job.h"

#include "core/actor.h"
#include "core/types.h"

#define TIME_DIFF(now, last) (last > now ? ((uint32_t)-1) - last + now : now - last)

enum app_thread_flags {
    APP_THREAD_SHARED = 1 << 0,
    APP_THREAD_BLOCKABLE = 1 << 1
};

struct app_thread {
    actor_t *actor;
    uint32_t last_time;
    uint32_t current_time;
    uint32_t next_time;

    TaskHandle_t task;
    QueueHandle_t queue;

    actor_worker_t *workers;
    uint16_t worker_count;

    uint8_t flags;
};

struct actor_worker {
    uint32_t last_time;    /* Previous time the tick fired */
    uint32_t next_time;    /* Next time tick will fire */
    app_thread_t *thread;
    actor_t *actor;
    app_thread_t *catchup; /* Did tick miss any messages? */
    actor_worker_callback_t callback;
};

app_thread_t *app_thread_allocate(app_thread_t **thread_slot, actor_t *actor, void (*callback)(void *ptr), const char *const name,
                        uint16_t stack_depth, size_t queue_size, size_t priority, uint8_t flags);

void app_thread_free(app_thread_t *thread);

/* Returns actor worker handling given thread */
actor_worker_t *app_thread_worker_for(app_thread_t *thread, actor_t *actor);


size_t app_thread_iterate_workers(app_thread_t *thread, actor_worker_t *destination);

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
void app_thread_worker_schedule(app_thread_t *thread, actor_worker_t *tick, uint32_t time);
void app_thread_actor_schedule(app_thread_t *thread, actor_t *actor, uint32_t time);

#define app_thread_catchup(thread) app_thread_notify_generic(app_thread_get_catchup_thread(thread), APP_SIGNAL_CATCHUP, false);
#define app_thread_reschedule(thread) app_thread_notify_generic(thread, APP_SIGNAL_RESCHEDULE, false);

#define app_thread_notify(thread, signal) app_thread_notify_generic(thread, (uint32_t)argument, true);

#define app_thread_publish_to_front(thread, event) app_thread_publish_generic(thread, event, 1)
#define app_thread_publish(thread, event) app_thread_publish_generic(thread, event, 0)
#define app_publish(app, event) app_thread_publish_generic(app->input, event, 0);

#define app_thread_actor_schedule(thread, actor, time) app_thread_worker_schedule(thread, app_thread_worker_for(thread, actor), time);

char *app_thread_get_name(app_thread_t *thread);

app_signal_t app_thread_event_await(app_thread_t *thread, app_event_t *event);

bool_t app_thread_is_interrupted(app_thread_t *thread);
bool_t app_thread_is_blockable(app_thread_t *thread);

#ifdef __cplusplus
}
#endif

#endif