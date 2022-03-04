#include <actor/job.h>
#include <actor/thread.h>
actor_signal_t actor_job_execute(actor_job_t **job_slot) {
    actor_job_t *job = *job_slot;
    actor_disable_interrupts();
    actor_assert(job);
    actor_assert(job->handler);
    bool halt = false;
    debug_printf("| ├ Task \t\t#%s at %u:%u for %s\n", actor_event_stringify(&job->inciting_event), job->job_phase, job->task_phase,
                 actor_stringify(job->actor));
    while (!halt) {
        actor_job_signal_t job_signal = actor_job_advance(job, job->handler(job));
        switch (job_signal) {
        case ACTOR_JOB_HALT:
        case ACTOR_JOB_SUCCESS:
        case ACTOR_JOB_FAILURE:
            if (job->job_phase == job_signal || job_signal == ACTOR_JOB_HALT) {
                debug_printf("│ │ ├ %s\t\t%u:%u\n", "Halt", job->job_phase, job->task_phase);
                actor_job_complete_callback(job);
            }
            if (job->job_phase != job_signal || job_signal == ACTOR_JOB_HALT) {
                actor_t *actor = job->actor;
                actor_job_finalize(job_slot);
                actor_node_idle_callback(actor);
                halt = true;
            }
            break;
        case ACTOR_JOB_TASK_QUIT_ISR:
            if (actor_thread_is_interrupted(job->thread)) {
                actor_thread_actor_schedule(job->thread, job->actor, job->thread->current_time);
                debug_printf("│ │ └ Yield from ISR\t%u:%u\n", job->job_phase, job->task_phase);
                actor_enable_interrupts();
                return 0;
            }
            break;
        case ACTOR_JOB_TASK_SUCCESS:
            if (job->task_phase != job_signal) {
                debug_printf("│ │ ├ Step complete\t%u:%u\n", job->job_phase, job->task_phase);
            }
            break;
        case ACTOR_JOB_TASK_RETRY:
        case ACTOR_JOB_TASK_CONTINUE:
        case ACTOR_JOB_TASK_FAILURE:
            break;
        case ACTOR_JOB_TASK_WAIT:
            actor_event_finalize(job->actor, &job->incoming_event); // free up room for a new event
            halt = true;
            break;
        default:
            halt = true;
        }
        job->incoming_signal = ACTOR_SIGNAL_PENDING;
    }
    debug_printf("│ │ └ Yield\t\t%u:%u\n", job->job_phase, job->task_phase);
    actor_enable_interrupts();
    return 0;
}

actor_signal_t actor_job_finalize(actor_job_t **job_slot) {
    actor_job_t *job = *job_slot;
    actor_event_finalize(job->actor, &job->inciting_event);
    actor_event_finalize(job->actor, &job->incoming_event);
    actor_job_free(job);
    *job_slot = NULL;
    return 0;
}

actor_job_signal_t actor_job_advance(actor_job_t *job, actor_job_signal_t signal) {
    switch (job->job_phase) {
    case ACTOR_JOB_FAILURE:
        job->job_phase = 0;
        return ACTOR_JOB_FAILURE;

    case ACTOR_JOB_SUCCESS:
        job->job_phase = 0;
        return ACTOR_JOB_SUCCESS;

    default:
        break;
    }

    switch (job->task_phase) {
    case ACTOR_JOB_TASK_FAILURE:
        job->job_phase = ACTOR_JOB_FAILURE;
        return ACTOR_JOB_FAILURE;

    case ACTOR_JOB_TASK_SUCCESS:
        job->job_phase++;
        job->task_phase = 0;
        return ACTOR_JOB_TASK_SUCCESS;

    default:
        break;
    }

    switch (signal) {
    case ACTOR_JOB_SUCCESS:
        job->job_phase = ACTOR_JOB_SUCCESS;
        break;

    case ACTOR_JOB_FAILURE:
        job->job_phase = ACTOR_JOB_FAILURE;
        break;

    case ACTOR_JOB_RETRY:
        job->task_phase = 0;
        job->job_phase = 0;
        break;

    case ACTOR_JOB_TASK_RETRY:
        job->task_phase = 0;
        break;

    case ACTOR_JOB_TASK_WAIT:
    case ACTOR_JOB_TASK_CONTINUE:
        job->task_phase++;
        break;

    case ACTOR_JOB_TASK_SUCCESS:
        job->task_phase = ACTOR_JOB_TASK_SUCCESS;
        break;

    case ACTOR_JOB_TASK_FAILURE:
        debug_printf("│ │ ├ Got failure\t%u:%u\n", job->job_phase, job->task_phase);
        job->task_phase = ACTOR_JOB_TASK_FAILURE;
        break;

    case ACTOR_JOB_HALT:
    case ACTOR_JOB_TASK_LOOP:
    case ACTOR_JOB_TASK_QUIT_ISR:
        break;
    }
    return signal;
}

actor_signal_t actor_job_execute_if_running_in_thread(actor_job_t **job_slot, actor_thread_t *thread) {
    actor_job_t *job = *job_slot;
    if (job != NULL && job->handler != NULL && job->thread == thread) {
        return actor_job_execute(job_slot);
    }
    return 0;
}

actor_signal_t actor_job_wait(actor_job_t *job) {
    actor_assert(job->thread);
    if (actor_thread_is_blockable(job->thread)) {
        actor_enable_interrupts();
        job->incoming_signal = actor_thread_event_await(job->thread, &job->inciting_event);
        actor_disable_interrupts();
        actor_worker_t *worker = actor_thread_worker_for(job->thread, job->actor);
        worker->next_time = -1;
        return job->incoming_signal;
    } else {
        return ACTOR_SIGNAL_OK;
    }
}

__attribute__((weak)) actor_job_t *actor_job_alloc(actor_t *actor) {
    return actor_malloc(sizeof(actor_job_t));
}
__attribute__((weak)) void actor_job_free(actor_job_t *buffer) {
    actor_free(buffer);
}

__attribute__((weak)) void actor_node_idle_callback(actor_t *actor) {
}
__attribute__((weak)) void actor_job_complete_callback(actor_job_t *job) {
}