#include <actor/job.h>
#include <actor/thread.h>

actor_signal_t actor_job_execute(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_enter_critical();
  actor_assert(job);
  actor_assert(job->handler);
  debug_printf("| ├ Task \t\t#%s at %li:%li for %s\n",
               actor_message_stringify(&job->inciting_message), (long) job->job_phase, (long) job->task_phase,
               actor_stringify(job->actor));

  actor_signal_t returned_signal = ACTOR_SIGNAL_OK;

  for (bool halt = false; !halt;) {
    // run a single step of a job/task
    actor_signal_t job_signal = job->handler(job, signal, caller);

    // advance internal counters
    halt = actor_job_advance(job, job_signal);

    // when job is complete, caller receieves the signal of the last task
    if (job_signal != ACTOR_SIGNAL_JOB_COMPLETE) {
      returned_signal = job_signal;
    }

    // job stops on uncaught error or when completion signal is receieved
    if (returned_signal < 0 || job_signal == ACTOR_SIGNAL_JOB_COMPLETE) {
      debug_printf("│ │ ├ %s\t\t%li:%li\n", "Halt", (long) job->job_phase, (long) job->task_phase);
      actor_callback_job_complete(job->actor);
      actor_job_finalize(job, returned_signal);
      actor_worker_catchup(job->actor, NULL);
    } else if (returned_signal == ACTOR_SIGNAL_TASK_COMPLETE) {
      debug_printf("│ │ ├ Task complete\t%li:%li\n", (long) job->job_phase, (long) job->task_phase);
    } else if (returned_signal == ACTOR_SIGNAL_WAIT) {
      // wait event has to free up space for new event to arrive
      actor_message_finalize(job->actor, &job->incoming_message, ACTOR_SIGNAL_OK);
    }

    signal = ACTOR_SIGNAL_OK;
  }
  debug_printf("│ │ └ Yield\t\t%li:%li\n", (long ) job->job_phase, (long) job->task_phase);
  actor_exit_critical();
  return returned_signal;
}

actor_signal_t actor_job_finalize(actor_job_t* job, actor_signal_t signal) {
  actor_message_finalize(job->actor, &job->inciting_message, signal);
  actor_message_finalize(job->actor, &job->incoming_message, signal);
  actor_t* actor = job->actor;
  *job->pointer = NULL;
  actor_job_free(job);
  return 0;
}

actor_signal_t actor_job_abort(actor_job_t *job, actor_t *caller) {
  return actor_job_execute(job, ACTOR_SIGNAL_ABORT, caller);
}

bool actor_job_advance(actor_job_t* job, actor_signal_t signal) {
  switch (signal) {
      // case ACTOR_SIGNAL_RETRY:
      //     job->task_phase = 0;
      //     break;

    case ACTOR_SIGNAL_TASK_COMPLETE:
      job->task_phase = 0;
      job->job_phase++;
      return true;

    case ACTOR_SIGNAL_WAIT:
      job->task_phase++;
      return true;

    case ACTOR_SIGNAL_JOB_COMPLETE:
    case ACTOR_SIGNAL_LOOP:
      break;

    default:
      if (signal < 0) {
        debug_printf("│ │ ├ Got failure\t%li:%li\n", (long) job->job_phase, (long) job->task_phase);
        return true;
      } else {
        job->task_phase++;
      }
      break;
  }
  return false;
}

actor_signal_t actor_job_execute_if_running_in_thread(actor_job_t* job, actor_thread_t* thread) {
  if (job != NULL && job->handler != NULL && job->thread == thread) {
    return actor_job_execute(job, ACTOR_SIGNAL_OK, job->actor);
  }
  return 0;
}

actor_signal_t actor_job_task_retry(actor_job_t* job) {
  job->task_phase = 0;
  return ACTOR_SIGNAL_LOOP;
}

actor_signal_t actor_job_wait(actor_job_t* job) {
  actor_assert(job->thread);
  if (actor_thread_is_blockable(job->thread)) {
    actor_enable_interrupts();
    // block thread on a signal
    actor_signal_t signal = actor_thread_message_await(job->thread, &job->incoming_message);
    actor_disable_interrupts();
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
