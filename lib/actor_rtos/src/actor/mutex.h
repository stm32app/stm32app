#ifndef INC_ACTOR_MUTEX
#define INC_ACTOR_MUTEX

#include <actor/env.h>
#include <actor/job.h>

struct actor_mutex {
    actor_t *actor;
    actor_job_t **blocked_jobs;
    uint8_t blocks;
    uint8_t holders;
    uint8_t extra_holders_limit;
};


actor_signal_t actor_mutex_acquire(actor_mutex_t **mutex_slot, actor_job_t *job);
actor_signal_t actor_mutex_release(actor_mutex_t **mutex_slot, actor_job_t *job);
actor_signal_t actor_mutex_cancel(actor_mutex_t *mutex, actor_job_t *job);

void actor_mutex_expiration_delay(actor_job_t *job, uint32_t *mutex_expiration_slot, uint32_t timeout_ms);
void actor_mutex_attempt_expiration(actor_mutex_t **mutex_slot, actor_job_t *job, uint32_t *mutex_expiration_slot);


actor_signal_t actor_mutex_take(actor_job_t *job, actor_mutex_t **mutex_slot, uint32_t timeout_ms);
actor_signal_t actor_mutex_give(actor_job_t *job, actor_mutex_t **mutex_slot, uint32_t timeout_ms);


actor_mutex_t *actor_mutex_alloc(actor_t *actor);
void actor_mutex_free(actor_mutex_t *mutex);
#endif