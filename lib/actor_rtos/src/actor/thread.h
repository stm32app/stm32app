#ifndef INC_CORE_THREAD
#define INC_CORE_THREAD

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/env.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include <actor/job.h>
#include <multi_heap.h>

#include <actor/types.h>

#define TIME_DIFF(now, last) (last > now ? ((uint32_t)-1) - last + now : now - last)

enum actor_thread_flags {
    ACTOR_THREAD_SHARED = 1 << 0,
    ACTOR_THREAD_BLOCKABLE = 1 << 1
};

struct actor_thread {
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
    actor_thread_t *thread;
    actor_t *actor;
    actor_thread_t *catchup; /* Did tick miss any messages? */
    actor_worker_callback_t callback;
};

actor_thread_t *actor_thread_create(actor_thread_t **thread_slot, actor_t *actor, void (*callback)(void *ptr), const char *const name,
                        uint16_t stack_depth, size_t queue_size, size_t priority, uint8_t flags);

void actor_thread_free(actor_thread_t *thread);

/* Returns actor worker handling given thread */
actor_worker_t *actor_thread_worker_for(actor_thread_t *thread, actor_t *actor);

/*
Callback methods that is invoked as FreeRTOS tasks.
Typical task flow: Allow actors to schedule next tick, or be woken up on semaphore
*/
void actor_thread_execute(actor_thread_t *thread);

/* publish a new event to the queue and notify the thread to wake up*/
bool actor_thread_publish_generic(actor_thread_t *thread, actor_event_t *event, bool to_front);

/* Wake up thread to run asap without waiting for next tick*/
bool actor_thread_notify_generic(actor_thread_t *thread, uint32_t value, bool overwrite);

actor_thread_t *actor_thread_get_catchup_thread(actor_thread_t *thread);

void actor_thread_schedule(actor_thread_t *thread, uint32_t time);
void actor_thread_worker_schedule(actor_thread_t *thread, actor_worker_t *tick, uint32_t time);
void actor_thread_actor_schedule(actor_thread_t *thread, actor_t *actor, uint32_t time);

#define actor_thread_catchup(thread) actor_thread_notify_generic(actor_thread_get_catchup_thread(thread), ACTOR_SIGNAL_CATCHUP, false);
#define actor_thread_reschedule(thread) actor_thread_notify_generic(thread, ACTOR_SIGNAL_RESCHEDULE, false);

#define actor_thread_notify(thread, signal) actor_thread_notify_generic(thread, (uint32_t)argument, true);

#define actor_thread_publish_to_front(thread, event) actor_thread_publish_generic(thread, event, 1)
#define actor_thread_publish(thread, event) actor_thread_publish_generic(thread, event, 0)
#define zactor_publish(node, event) actor_thread_publish_generic(node->input, event, 0);
#define actor_publish(actor, event) actor_thread_publish_generic(actor_get_input_thread(actor), event, 0);

#define actor_thread_actor_schedule(thread, actor, time) actor_thread_worker_schedule(thread, actor_thread_worker_for(thread, actor), time);

char *actor_thread_get_name(actor_thread_t *thread);

actor_signal_t actor_thread_event_await(actor_thread_t *thread, actor_event_t *event);

/* Weak symbols */
bool actor_thread_is_interrupted(actor_thread_t *thread);
bool actor_thread_is_blockable(actor_thread_t *thread);

uint32_t actor_thread_iterate_workers(actor_thread_t *thread, actor_worker_t *destination);
actor_t *actor_thread_get_root_actor(actor_thread_t *thread);
actor_thread_t *actor_thread_alloc(actor_t *actor);
void actor_thread_destroy(actor_thread_t *thread);

actor_thread_t* actor_get_input_thread(actor_t *actor);
bool actor_event_is_subscribed(actor_t *actor, actor_event_t *event);
#ifdef __cplusplus
}
#endif

#endif