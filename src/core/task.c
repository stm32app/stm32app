#include "task.h"
#include <libopencm3/cm3/cortex.h>

#define IS_IN_ISR (SCB_ICSR & SCB_ICSR_VECTACTIVE)

app_signal_t app_job_execute(app_job_t *job) {
    cm_disable_interrupts();
    debug_assert(job);
    debug_assert(job->handler);
    bool_t halt = false;
    debug_printf("| ├ Task \t\t#%s at %u:%u for %s\n", get_app_event_type_name(job->inciting_event.type), job->job_phase, job->task_phase,
                 get_actor_type_name(job->actor->class->type));
    while (!halt) {
        app_job_signal_t job_signal = app_job_advance(job, job->handler(job));
        switch (job_signal) {
        case APP_JOB_HALT:
            debug_printf("│ │ ├ %s\t\t%u:%u\n", "Halt", job->job_phase, job->task_phase);
            if (job->actor->class->on_job != NULL) {
                job->actor->class->on_job(job->actor->object, job);
            }
            app_job_finalize(job);
            actor_worker_catchup(job->actor, NULL);
            halt = true;
            break;
        case APP_JOB_SUCCESS:
        case APP_JOB_FAILURE:
            if (job->job_phase == job_signal) {
                debug_printf("│ │ ├ %s\t\t%u:%u\n", job_signal == APP_JOB_SUCCESS ? "Success" : "Failure", job->job_phase, job->task_phase);
                if (job->actor->class->on_job != NULL) {
                    job->actor->class->on_job(job->actor->object, job);
                }
            } else {
                app_job_finalize(job);
                actor_worker_catchup(job->actor, NULL);
                halt = true;
            }
            break;
        case APP_JOB_TASK_QUIT_ISR:
            if (IS_IN_ISR) {
                app_thread_actor_schedule(job->thread, job->actor, job->thread->current_time);
                debug_printf("│ │ └ Yield from ISR\t%u:%u\n", job->job_phase, job->task_phase);
                cm_enable_interrupts();
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
            actor_event_finalize(job->actor, &job->awaited_event); // free up room for a new event
            halt = true;
            break;
        default:
            halt = true;
        }
    }
    debug_printf("│ │ └ Yield\t\t%u:%u\n", job->job_phase, job->task_phase);
    cm_enable_interrupts();
    return 0;
}

app_signal_t app_job_finalize(app_job_t *job) {
    actor_event_finalize(job->actor, &job->inciting_event);
    actor_event_finalize(job->actor, &job->awaited_event);
    job->handler = NULL;
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

app_signal_t app_job_execute_if_running_in_thread(app_job_t *job, app_thread_t *thread) {
    if (job != NULL && job->handler != NULL && job->thread == thread) {
        return app_job_execute(job);
    }
    return 0;
}

app_signal_t app_job_execute_in_coroutine_if_running_in_thread(app_job_t *job, app_thread_t *thread, coru_t *coroutine) {
    if (job != NULL && job->handler != NULL && job->thread == thread) {
        cm_disable_interrupts();
        int a = coru_resume(coroutine);
        cm_enable_interrupts();
        return a;
    }
    return 0;
}
