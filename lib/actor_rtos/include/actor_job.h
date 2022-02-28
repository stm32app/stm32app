#ifndef INC_CORE_TASK
#define INC_CORE_TASK

#include <actor_types.h>
#include <actor_event.h>

#define ACTOR_JOB_HALT_INDEX 64
#define ACTOR_JOB_SUCCESS_INDEX -1

struct actor_job {
    actor_t *actor;
    actor_thread_t *thread;         // thread that job currently was assigned to

    actor_event_t inciting_event;   // event that started the job
    actor_event_t outgoing_event;   // event that actor produced during the job 
    actor_event_t incoming_event;    // event that was receieved from other actors during the job
    actor_signal_t incoming_signal; // signal that was passed to the job

    actor_buffer_t *source_buffer;  // buffer that is used as a source for write operations
    actor_buffer_t *target_buffer;  // buffer that is used as a target for read operations
    actor_buffer_t *result_buffer;  // buffer that is returned 

    uint32_t counter;

    void *result;

    size_t job_phase;
    size_t task_phase;

    actor_on_job_complete_t handler;
};

enum actor_job_signal {
    ACTOR_JOB_HALT,

    ACTOR_JOB_TASK_CONTINUE,
    ACTOR_JOB_TASK_RETRY,
    ACTOR_JOB_TASK_WAIT,
    ACTOR_JOB_TASK_QUIT_ISR,
    ACTOR_JOB_TASK_LOOP,

    ACTOR_JOB_RETRY,
    ACTOR_JOB_TASK_SUCCESS = 252,
    ACTOR_JOB_TASK_FAILURE = 253,
    ACTOR_JOB_SUCCESS = 254,
    ACTOR_JOB_FAILURE = 255,
};

actor_signal_t actor_job_execute(actor_job_t **job_slot);
actor_signal_t actor_job_finalize(actor_job_t **job_slot);
actor_signal_t actor_job_execute_if_running_in_thread(actor_job_t **job_slot, actor_thread_t *thread);
actor_job_signal_t actor_job_advance(actor_job_t *job_slot, actor_job_signal_t signal);

actor_signal_t actor_job_wait(actor_job_t *job);

/* Weak symbols */
actor_job_t *actor_job_alloc(actor_t *actor);
void actor_job_free(actor_job_t *buffer);

void actor_node_idle_callback(actor_t *actor);
void actor_job_complete_callback(actor_job_t *job);
#endif