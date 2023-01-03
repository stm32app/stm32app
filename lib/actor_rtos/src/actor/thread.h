#ifndef INC_CORE_THREAD
#define INC_CORE_THREAD

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hooks.h"
#include <actor/env.h>
#include <actor/job.h>
#include <actor/types.h>
#include <actor/mutex.h>

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
bool actor_thread_publish_generic(actor_thread_t *thread, actor_message_t *event, bool to_front);

/* Wake up thread to run asap without waiting for next tick*/
bool actor_thread_notify_generic(actor_thread_t *thread, uint32_t value, bool overwrite);

actor_thread_t *actor_thread_get_catchup_thread(actor_thread_t *thread);

uint32_t actor_thread_delay(actor_thread_t *thread, uint32_t delay_ms);
uint32_t actor_worker_delay(actor_worker_t *worker, uint32_t delay_ms);
#define actor_thread_actor_delay(thread, actor, delay_ms) actor_worker_delay(actor_thread_worker_for(thread, actor), delay_ms);

#define actor_thread_actor_wakeup(thread, actor) actor_worker_delay(actor_thread_worker_for(thread, actor), 0);
#define actor_thread_wakeup(thread) actor_thread_delay(thread, 0)
#define actor_worker_wakeup(thread) actor_worker_delay(thread, 0)

#define actor_thread_catchup(thread) actor_thread_notify_generic(actor_thread_get_catchup_thread(thread), ACTOR_SIGNAL_CATCHUP, false);
#define actor_thread_reschedule(thread) actor_thread_notify_generic(thread, ACTOR_SIGNAL_RESCHEDULE, false);

#define actor_thread_notify(thread, signal) actor_thread_notify_generic(thread, (uint32_t)argument, true);

#define actor_thread_publish_to_front(thread, event) actor_thread_publish_generic(thread, event, 1)
#define actor_thread_publish(thread, event) actor_thread_publish_generic(thread, event, 0)
#define actor_publish(actor, event) actor_thread_publish_generic(actor_get_input_thread(actor), event, 0);

char *actor_thread_get_name(actor_thread_t *thread);

actor_signal_t actor_thread_message_await(actor_thread_t *thread, actor_message_t *event);

/* Weak symbols */
bool actor_thread_is_interrupted(actor_thread_t *thread);
bool actor_thread_is_blockable(actor_thread_t *thread);
bool actor_thread_is_current(actor_thread_t *thread);

uint32_t actor_thread_iterate_workers(actor_thread_t *thread, actor_worker_t *destination);
actor_t *actor_thread_get_root_actor(actor_thread_t *thread);
actor_thread_t *actor_thread_alloc(actor_t *actor);
void actor_thread_destroy(actor_thread_t *thread);

actor_thread_t* actor_get_input_thread(actor_t *actor);
bool actor_message_is_subscribed(actor_t *actor, actor_message_t *event);

void actor_enter_critical(void);
void actor_exit_critical(void);
void actor_exit_critical_cleanup(void *value);

#define actor_critical_context() \
  actor_enter_critical();      \
  int8_t __critical __attribute__((__cleanup__(actor_exit_critical_cleanup))) = 0

#ifdef __cplusplus
}
#endif

#endif