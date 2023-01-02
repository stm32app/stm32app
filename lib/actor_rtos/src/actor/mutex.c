#include "mutex.h"
#include <actor/job.h>
#include <actor/lib/time.h>
#include <actor/thread.h>

// Find if the job is blocked on given mutex
static uint8_t actor_mutex_blocked_jobs_find(actor_mutex_t* mutex, actor_job_t* job) {
  for (uint8_t i = 0; i < mutex->blocks; i++) {
    if (mutex->blocked_jobs[i] == job) {
      return i;
    }
  }
  return -1;
}

// Let mutex know that a job is waiting for its release
static void actor_mutex_jobs_block(actor_mutex_t* mutex, actor_job_t* job) {
  actor_assert(mutex->blocked_jobs == NULL ||
               actor_mutex_blocked_jobs_find(mutex, job) == (uint8_t)-1);
  // LIMIT: 255 max semaphore value
  // LIMIT: 254 max blocked jobs
  actor_assert(mutex->blocks < (uint8_t)-2);
  if (mutex->blocked_jobs == NULL)
    mutex->blocked_jobs =
        actor_realloc_region(ACTOR_REGION_FAST, mutex->blocked_jobs, mutex->blocks + 1);
  mutex->blocked_jobs[mutex->blocks] = job;
  mutex->blocks++;
}

// find given job and unblock it, returns true if job was found
static bool actor_mutex_jobs_unblock(actor_mutex_t* mutex, actor_job_t* job) {
  uint8_t i = actor_mutex_blocked_jobs_find(mutex, job);
  if (i == (uint8_t)-1) {
    return false;
  }
  mutex->blocked_jobs[i] = NULL;
  mutex->blocks--;
  return true;
}

// find first blocked job and remove it from queue
static actor_job_t* actor_mutex_jobs_shift(actor_mutex_t* mutex) {
  for (uint8_t i = 0; i < mutex->blocks; i++) {
    actor_job_t* job = mutex->blocked_jobs[i];
    if (job != NULL) {
      mutex->blocked_jobs[i] = NULL;
      mutex->blocks--;
      return job;
    }
  }

  return NULL;
}

void actor_mutex_expiration_delay(actor_job_t* job,
                                  uint32_t* mutex_expiration_slot,
                                  uint32_t timeout_ms) {
  *mutex_expiration_slot = actor_thread_delay(job->thread, timeout_ms);
}

void actor_mutex_attempt_expiration(actor_mutex_t** mutex_slot,
                                    actor_job_t* job,
                                    uint32_t* mutex_expiration_slot) {
  if (*mutex_expiration_slot != 0) {
    if (time_gte(job->thread->current_time, *mutex_expiration_slot)) {
      actor_mutex_release(mutex_slot, job);
      *mutex_expiration_slot = 0;
    }
  }
}

actor_signal_t actor_mutex_acquire(actor_mutex_t** mutex_slot, actor_job_t* job) {
  actor_assert(job);
  actor_enter_critical();
  if (*mutex_slot == NULL) {
    *mutex_slot = actor_mutex_alloc(job->actor);
  }
  actor_mutex_t* mutex = *mutex_slot;

  if (mutex->holders <= mutex->extra_holders_limit) {
    mutex->holders++;
    actor_exit_critical();
    return ACTOR_SIGNAL_OK;
  }

  actor_mutex_jobs_block(mutex, job);
  actor_exit_critical();
  return actor_job_wait(job);
}

actor_signal_t actor_mutex_release(actor_mutex_t** mutex_slot, actor_job_t* job) {
  actor_assert(mutex_slot);
  actor_mutex_t* mutex = *mutex_slot;
  actor_assert(mutex);
  actor_assert(mutex->holders);

  // give mutex to a blocked job
  if (mutex->blocks) {
    actor_job_t* blocked_job = actor_mutex_jobs_shift(mutex);
    actor_job_execute(blocked_job, ACTOR_SIGNAL_CATCHUP, job->actor);
    return ACTOR_SIGNAL_CATCHUP;
    // deinitialize mutex
  } else {
    mutex->holders--;
    if (mutex->holders == 0) {
      actor_free(mutex->blocked_jobs);
      actor_free(mutex);
    }
    return ACTOR_SIGNAL_OK;
  }
}

actor_signal_t actor_mutex_cancel(actor_mutex_t* mutex, actor_job_t* job) {
  return actor_mutex_jobs_unblock(mutex, job) ? ACTOR_SIGNAL_OK : ACTOR_SIGNAL_UNAFFECTED;
}

__attribute__((weak)) actor_mutex_t* actor_mutex_alloc(actor_t* actor) {
  return actor_malloc_region(ACTOR_REGION_FAST, sizeof(actor_mutex_t));
}

__attribute__((weak)) void actor_mutex_free(actor_mutex_t* mutex) {
  actor_free(mutex);
}
