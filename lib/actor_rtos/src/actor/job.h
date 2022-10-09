#ifndef INC_CORE_TASK
#define INC_CORE_TASK

#include <actor/event.h>
#include <actor/types.h>

struct actor_job {
    actor_t *actor;         // actor owning the job
    actor_job_t **pointer;  // pointer that binds job to its actor
    actor_thread_t *thread; // thread that job currently was assigned to

    actor_message_t inciting_message; // event that started the job
    actor_message_t incoming_message;    // event that was receieved from other actors during the job
    
    // actor_message_t outgoing_message;   // event that actor produced during the job
    // actor_signal_t incoming_signal; // signal that was passed to the job

    actor_buffer_t *source_buffer; // buffer that is used as a source for write operations
    actor_buffer_t *target_buffer; // buffer that is used as a target for read operations
    actor_buffer_t *output_buffer; // buffer that is returned

    int8_t job_phase;
    int8_t task_phase;
    int8_t retries;
    uint32_t counter;

    actor_job_handler_t handler;

    void *state; // pointer to user-provided state for re-entrancy
};

actor_signal_t actor_job_execute(actor_job_t *job, actor_signal_t signal, actor_t *caller);
actor_signal_t actor_job_finalize(actor_job_t *job, actor_signal_t signal);
actor_signal_t actor_job_execute_if_running_in_thread(actor_job_t *job, actor_thread_t *thread);
bool actor_job_advance(actor_job_t *job, actor_signal_t signal);

/* Block thread if its blockable and await for incoming signal.*/
actor_signal_t actor_job_wait(actor_job_t *job);

actor_signal_t actor_job_switch_thread(actor_job_t *job, actor_thread_t *thread);

/* Weak symbols */
actor_job_t *actor_job_alloc(actor_t *actor);
void actor_job_free(actor_job_t *buffer);

void actor_callback_job_complete(actor_t *actor);
void actor_callback_job_started(actor_t *actor);

actor_signal_t actor_job_delay(actor_job_t *job, uint32_t delay_ms);
actor_signal_t actor_job_delay_us(actor_job_t *job, uint32_t delay_us);
#define actor_job_wakeup(job) actor_job_delay(job, 0)

actor_signal_t actor_job_task_retry(actor_job_t *job);

#endif