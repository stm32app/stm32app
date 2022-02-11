#include "task.h"
#include <libopencm3/cm3/cortex.h>


#define IS_IN_ISR (SCB_ICSR & SCB_ICSR_VECTACTIVE)

app_signal_t app_job_execute(app_job_t *task) {
  CM_ATOMIC_CONTEXT();
  configASSERT(task);
  configASSERT(task->handler);
  bool_t halt = false;
  log_printf("| ├ Task \t\t#%s at %u:%u for %s\n", get_app_event_type_name(task->inciting_event.type), task->step_index, task->task_index, get_actor_type_name(task->actor->class->type));
  while (!halt) {
    app_job_signal_t task_signal = app_job_advance(task, task->handler(task));
    switch (task_signal) {
      case APP_JOB_SUCCESS:
      case APP_JOB_FAILURE:
        if (task->step_index == task_signal) {
          log_printf("│ │ ├ %s\t\t%u:%u\n", task_signal == APP_JOB_SUCCESS ? "Success" : "Failure", task->step_index, task->task_index);
          if (task->actor->class->on_job != NULL) {
            task->actor->class->on_job(task->actor->object, task);
          }
        } else {
          app_job_finalize(task);
          actor_worker_catchup(task->actor, NULL);
          halt = true;
        }
        break;
      case APP_JOB_TASK_QUIT_ISR:
        if (IS_IN_ISR) {
          app_thread_actor_schedule(task->thread, task->actor, task->thread->current_time);
          log_printf("│ │ └ Yield from ISR\t%u:%u\n", task->step_index, task->task_index);
          return 0;
        }
        break;
      case APP_JOB_TASK_SUCCESS:
        if (task->task_index != task_signal) {
          log_printf("│ │ ├ Step complete\t%u:%u\n", task->step_index, task->task_index);
        }
        break;
      case APP_JOB_TASK_RETRY:
      case APP_JOB_TASK_CONTINUE:
        break;   
      case APP_JOB_TASK_WAIT:
        actor_event_finalize(task->actor, &task->awaited_event);  // free up room for a new event
        halt = true;
        break;
      default:
        halt = true;
    }
  }
  log_printf("│ │ └ Yield\t\t%u:%u\n", task->step_index, task->task_index);
  return 0;
}

app_signal_t app_job_finalize(app_job_t *task) {
  actor_event_finalize(task->actor, &task->inciting_event);
  actor_event_finalize(task->actor, &task->awaited_event);
  task->handler = NULL;
  return 0;
}

app_job_signal_t app_job_advance(app_job_t *task, app_job_signal_t signal) {
  switch (task->step_index) {
    case APP_JOB_FAILURE:
      task->step_index = 0;
      return APP_JOB_FAILURE;

    case APP_JOB_SUCCESS:
      task->step_index = 0;
      return APP_JOB_SUCCESS;

    default:
      break;
  }

  switch (task->task_index) {
    case APP_JOB_TASK_FAILURE:
      task->step_index = APP_JOB_FAILURE;
      return APP_JOB_FAILURE;

    case APP_JOB_TASK_SUCCESS:
      task->step_index++;
      task->task_index = 0;
      return APP_JOB_TASK_SUCCESS;

    default:
      break;
  }

  switch (signal) {
    case APP_JOB_SUCCESS:
      task->step_index = APP_JOB_SUCCESS;
      break;
      
    case APP_JOB_FAILURE:
      task->step_index = APP_JOB_FAILURE;
      break;

    case APP_JOB_RETRY:
      task->task_index = 0;
      task->step_index = 0;
      break;

    case APP_JOB_TASK_RETRY:
      task->task_index = 0;
      break;

    case APP_JOB_TASK_WAIT:
    case APP_JOB_TASK_CONTINUE:
      task->task_index++;
      break;
      
    case APP_JOB_TASK_SUCCESS:
      task->task_index = APP_JOB_TASK_SUCCESS;
      break;

    case APP_JOB_TASK_FAILURE:
      task->task_index = APP_JOB_TASK_FAILURE;
      break;
      
    case APP_JOB_TASK_LOOP:
    case APP_JOB_TASK_QUIT_ISR:
       break;
  }
  return signal;
}

app_signal_t app_job_execute_if_running_in_thread(app_job_t *task, app_thread_t *thread) {
  if (task != NULL && task->handler != NULL && task->thread == thread) {
    return app_job_execute(task);
  }
  return 0;
}
