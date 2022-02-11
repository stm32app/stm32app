#ifndef INC_CORE_TASK
#define INC_CORE_TASK

#include "app.h"
#include "core/event.h"

#define APP_JOB_HALT_INDEX 64
#define APP_JOB_SUCCESS_INDEX -1

struct app_job {
    char *name;

    actor_t *actor;
    app_thread_t *thread;

    app_event_t inciting_event;
    app_event_t awaited_event;

    uint32_t counter;

    void *result;

    size_t step_index;
    size_t task_index;

    actor_on_job_t handler;
};

enum app_job_signal {
    APP_JOB_TASK_CONTINUE,
    APP_JOB_TASK_RETRY,
    APP_JOB_TASK_WAIT,
    APP_JOB_TASK_QUIT_ISR,
    APP_JOB_TASK_LOOP,

    APP_JOB_RETRY,
    APP_JOB_TASK_SUCCESS = 252,
    APP_JOB_TASK_FAILURE = 253,
    APP_JOB_SUCCESS = 254,
    APP_JOB_FAILURE = 255,
};

app_signal_t app_job_execute(app_job_t *task);
app_job_signal_t app_job_advance(app_job_t *task, app_job_signal_t signal);
app_signal_t app_job_finalize(app_job_t *task);
app_signal_t app_job_execute_if_running_in_thread(app_job_t *task, app_thread_t *thread);
#endif