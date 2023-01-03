#include <actor/job.h>
#include <actor/thread.h>

actor_signal_t actor_job_execute(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_assert(job);
  actor_assert(job->handler);
  // run a single step of a job/task
  return job->handler(job, signal, caller);
}


actor_signal_t actor_job_finalize(actor_job_t* job, actor_signal_t signal) {
  actor_callback_job_complete(job->actor);
  actor_message_finalize(job->actor, &job->inciting_message, signal);
  actor_message_finalize(job->actor, &job->incoming_message, signal);
  actor_t* actor = job->actor;
  *job->pointer = NULL;
  actor_job_free(job);
  actor_worker_catchup(actor, NULL);
  return signal;
}

actor_signal_t actor_job_abort(actor_job_t *job, actor_t *caller) {
  return actor_job_execute(job, ACTOR_SIGNAL_ABORT, caller);
}

actor_signal_t actor_job_execute_if_running_in_thread(actor_job_t* job, actor_thread_t* thread) {
  if (job != NULL && job->handler != NULL && job->thread == thread) {
    return actor_job_execute(job, ACTOR_SIGNAL_OK, job->actor);
  }
  return 0;
}

actor_signal_t actor_job_wait(actor_job_t* job) {
  actor_assert(job->thread);
  if (actor_thread_is_blockable(job->thread)) {
    //actor_enable_interrupts();
    // block thread on a signal
    actor_signal_t signal = actor_thread_message_await(job->thread, &job->incoming_message);
    //actor_disable_interrupts();
    actor_worker_t* worker = actor_thread_worker_for(job->thread, job->actor);
    worker->next_time = 0;
    return signal;
  } else {
    return ACTOR_SIGNAL_WAIT;
  }
}

actor_signal_t actor_job_switch_thread(actor_job_t* job, actor_thread_t* thread) {
  job->thread = thread;
  if (actor_thread_is_interrupted(
          thread) /* || actor_thread_get_current(thread->actor) != thread*/) {
    actor_job_wakeup(job);
    return ACTOR_SIGNAL_WAIT;
  } else {
    return ACTOR_SIGNAL_OK;
  }
}

__attribute__((weak)) actor_signal_t actor_job_delay(actor_job_t* job, uint32_t delay_ms) {
  actor_thread_actor_delay(job->thread, job->actor, delay_ms);
  return ACTOR_SIGNAL_WAIT;
}

__attribute__((weak)) actor_signal_t actor_job_delay_us(actor_job_t* job, uint32_t delay_us) {
  return actor_job_delay_us(job, delay_us / 1000);
}

__attribute__((weak)) actor_job_t* actor_job_alloc(actor_t* actor) {
  return actor_malloc_region(ACTOR_REGION_FAST, sizeof(actor_job_t));
}

__attribute__((weak)) void actor_job_free(actor_job_t* buffer) {
  actor_free(buffer);
}

__attribute__((weak)) void actor_callback_job_complete(actor_t* actor) {}

__attribute__((weak)) void actor_callback_job_started(actor_t* actor) {}
