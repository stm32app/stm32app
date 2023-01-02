#ifndef INC_CORE_TASK
#define INC_CORE_TASK

#include <actor/message.h>
#include <actor/types.h>

typedef enum actor_job_flag
{
  ACTOR_JOB_ASYNC = 1 << 0,
} actor_job_flag_t;

struct actor_job {
  actor_t* actor;          // actor owning the job
  actor_job_t** pointer;   // pointer that binds job to its actor
  actor_thread_t* thread;  // thread that job currently was assigned to

  actor_message_t inciting_message;  // event that started the job
  actor_message_t incoming_message;  // event that was receieved from other actors during the job

  actor_buffer_t* source_buffer;  // buffer that is used as a source for write operations
  actor_buffer_t* target_buffer;  // buffer that is used as a target for read operations
  actor_buffer_t* output_buffer;  // buffer that is returned

  uint8_t flags;
  int32_t job_phase;
  int32_t task_phase;
  int8_t retries;
  uint32_t counter;

  actor_job_handler_t handler;

  void* state;  // pointer to user-provided state for re-entrancy
};

actor_signal_t actor_job_execute(actor_job_t* job, actor_signal_t signal, actor_t* caller);
actor_signal_t actor_job_finalize(actor_job_t* job, actor_signal_t signal);
actor_signal_t actor_job_execute_if_running_in_thread(actor_job_t* job, actor_thread_t* thread);
actor_signal_t actor_job_abort(actor_job_t* job, actor_t* caller);

bool actor_job_advance(actor_job_t* job, actor_signal_t signal);

/* Block thread if its blockable and await for incoming signal.*/
actor_signal_t actor_job_wait(actor_job_t* job);

actor_signal_t actor_job_switch_thread(actor_job_t* job, actor_thread_t* thread);

/* Weak symbols */
actor_job_t* actor_job_alloc(actor_t* actor);
void actor_job_free(actor_job_t* buffer);

void actor_callback_job_complete(actor_t* actor);
void actor_callback_job_started(actor_t* actor);

actor_signal_t actor_job_delay(actor_job_t* job, uint32_t delay_ms);
actor_signal_t actor_job_delay_us(actor_job_t* job, uint32_t delay_us);
#define actor_job_wakeup(job) actor_job_delay(job, 0)

actor_signal_t actor_job_task_retry(actor_job_t* job);

#define actor_async_error (actor_async_signal < 0 ? actor_async_signal : 0)

// Establish async block using duff device
#define actor_async_begin(counter)                               \
  int32_t* actor_async_state = &(counter);                       \
  actor_message_t* actor_async_message = &job->incoming_message; \
  actor_signal_t actor_async_signal = signal;                    \
  switch (counter) {                                             \
    default:

#define actor_async_exit(signal) \
  *actor_async_state = -1;       \
  actor_async_signal = signal;   \
  goto actor_async_cleanup;

#define actor_async_end(signal, ...) \
  actor_async_exit(signal);          \
  case -1:                           \
    return signal;                   \
    }                                \
  actor_async_cleanup:               \
    __VA_ARGS__                      \
    return actor_async_signal;

#define actor_async_job_begin() actor_async_begin(job->job_phase);
#define actor_async_task_begin() actor_async_begin(job->task_phase);
#define actor_async_job_end(...) actor_async_end(ACTOR_SIGNAL_JOB_COMPLETE, __VA_ARGS__);
#define actor_async_task_end(...) actor_async_end(ACTOR_SIGNAL_TASK_COMPLETE, __VA_ARGS__);
#define actor_async_assert(condition, signal) \
  if (!(condition))                           \
    actor_async_exit(signal);

#define actor_async_fail() actor_async_exit(ACTOR_SIGNAL_FAILURE)

// Yield if expression signals to wait, re-throw failures
#define actor_async_await(expression)                                      \
  *actor_async_state = __LINE__;                                           \
  actor_async_signal = expression;                                         \
  if (actor_async_signal < 0 || actor_async_signal == ACTOR_SIGNAL_WAIT) { \
    return actor_async_signal;                                             \
  }                                                                        \
  case __LINE__:                                                           \
    if (actor_async_signal < 0) {                                          \
      return actor_async_signal;                                           \
    }

// Yield if expression signals to wait, ignore failures
#define actor_async_try(expression) \
  *actor_async_state = __LINE__;    \
  return expression;                \
  case __LINE__:

// Loop until condition becomes truthy
#define actor_async_until(condition) \
  *actor_async_state = __LINE__;     \
  case __LINE__:                     \
    if (!(condition))                  \
      return ACTOR_SIGNAL_WAIT;

// Yield execution
#define actor_async_yield() actor_async_await(ACTOR_SIGNAL_WAIT);
#endif