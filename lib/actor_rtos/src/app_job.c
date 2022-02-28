#include <app_job.h>
#include <app_rtos_macros.h>
#include <app_thread.h>

app_signal_t app_job_execute(app_job_t **job_slot) {
    app_job_t *job = *job_slot;
    app_disable_interrupts();
    APP_ASSERT(job);
    APP_ASSERT(job->handler);
    bool_t halt = false;
    debug_printf("| ├ Task \t\t#%s at %u:%u for %s\n", get_app_event_type_name(job->inciting_event.type), job->job_phase, job->task_phase,
                 app_actor_stringify(job->actor));
    while (!halt) {
        app_job_signal_t job_signal = app_job_advance(job, job->handler(job));
        switch (job_signal) {
        case APP_JOB_HALT:
        case APP_JOB_SUCCESS:
        case APP_JOB_FAILURE:
            if (job->job_phase == job_signal || job_signal == APP_JOB_HALT) {
                debug_printf("│ │ ├ %s\t\t%u:%u\n", "Halt", job->job_phase, job->task_phase);
                app_job_complete_callback(job);
            }
            if (job->job_phase != job_signal || job_signal == APP_JOB_HALT) {
                actor_t *actor = job->actor;
                app_job_finalize(job_slot);
                app_actor_idle_callback(actor);
                halt = true;
            }
            break;
        case APP_JOB_TASK_QUIT_ISR:
            if (app_thread_is_interrupted(job->thread)) {
                app_thread_actor_schedule(job->thread, job->actor, job->thread->current_time);
                debug_printf("│ │ └ Yield from ISR\t%u:%u\n", job->job_phase, job->task_phase);
                app_enable_interrupts();
                return 0;
            }
            break;
        case APP_JOB_TASK_SUCCESS:
            if (job->task_phase != job_signal) {
                debug_printf("│ │ ├ Step complete\t%u:%u\n", job->job_phase, job->task_phase);
            }
            break;
        case APP_JOB_TASK_RETRY:
        case APP_JOB_TASK_CONTINUE:
        case APP_JOB_TASK_FAILURE:
            break;
        case APP_JOB_TASK_WAIT:
            actor_event_finalize(job->actor, &job->incoming_event); // free up room for a new event
            halt = true;
            break;
        default:
            halt = true;
        }
        job->incoming_signal = APP_SIGNAL_PENDING;
    }
    debug_printf("│ │ └ Yield\t\t%u:%u\n", job->job_phase, job->task_phase);
    app_enable_interrupts();
    return 0;
}

app_signal_t app_job_finalize(app_job_t **job_slot) {
    app_job_t *job = *job_slot;
    actor_event_finalize(job->actor, &job->inciting_event);
    actor_event_finalize(job->actor, &job->incoming_event);
    app_job_free(job);
    *job_slot = NULL;
    return 0;
}

app_job_signal_t app_job_advance(app_job_t *job, app_job_signal_t signal) {
    switch (job->job_phase) {
    case APP_JOB_FAILURE:
        job->job_phase = 0;
        return APP_JOB_FAILURE;

    case APP_JOB_SUCCESS:
        job->job_phase = 0;
        return APP_JOB_SUCCESS;

    default:
        break;
    }

    switch (job->task_phase) {
    case APP_JOB_TASK_FAILURE:
        job->job_phase = APP_JOB_FAILURE;
        return APP_JOB_FAILURE;

    case APP_JOB_TASK_SUCCESS:
        job->job_phase++;
        job->task_phase = 0;
        return APP_JOB_TASK_SUCCESS;

    default:
        break;
    }

    switch (signal) {
    case APP_JOB_SUCCESS:
        job->job_phase = APP_JOB_SUCCESS;
        break;

    case APP_JOB_FAILURE:
        job->job_phase = APP_JOB_FAILURE;
        break;

    case APP_JOB_RETRY:
        job->task_phase = 0;
        job->job_phase = 0;
        break;

    case APP_JOB_TASK_RETRY:
        job->task_phase = 0;
        break;

    case APP_JOB_TASK_WAIT:
    case APP_JOB_TASK_CONTINUE:
        job->task_phase++;
        break;

    case APP_JOB_TASK_SUCCESS:
        job->task_phase = APP_JOB_TASK_SUCCESS;
        break;

    case APP_JOB_TASK_FAILURE:
        debug_printf("│ │ ├ Got failure\t%u:%u\n", job->job_phase, job->task_phase);
        job->task_phase = APP_JOB_TASK_FAILURE;
        break;

    case APP_JOB_HALT:
    case APP_JOB_TASK_LOOP:
    case APP_JOB_TASK_QUIT_ISR:
        break;
    }
    return signal;
}

app_signal_t app_job_execute_if_running_in_thread(app_job_t **job_slot, app_thread_t *thread) {
    app_job_t *job = *job_slot;
    if (job != NULL && job->handler != NULL && job->thread == thread) {
        return app_job_execute(job_slot);
    }
    return 0;
}

app_signal_t app_job_wait(app_job_t *job) {
    APP_ASSERT(job->thread);
    if (app_thread_is_blockable(job->thread)) {
        app_enable_interrupts();
        job->incoming_signal = app_thread_event_await(job->thread, &job->inciting_event);
        app_disable_interrupts();
        actor_worker_t *worker = app_thread_worker_for(job->thread, job->actor);
        worker->next_time = -1;
        return job->incoming_signal;
    } else {
        return APP_SIGNAL_OK;
    }
}

__attribute__((weak)) app_job_t *app_job_alloc(actor_t *actor) {
    return app_malloc(sizeof(app_job_t));
}
__attribute__((weak)) void app_job_free(app_job_t *buffer) {
    app_free(buffer);
}

__attribute__((weak)) void app_actor_idle_callback(actor_t *actor) {
}
__attribute__((weak)) void app_job_complete_callback(app_job_t *job) {
}