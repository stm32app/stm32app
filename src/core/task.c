#include "task.h"
#include <libopencm3/cm3/cortex.h>


#define IS_IN_ISR (SCB_ICSR & SCB_ICSR_VECTACTIVE)

app_signal_t app_task_execute(app_task_t *task) {
  CM_ATOMIC_CONTEXT();
  configASSERT(task);
  configASSERT(task->handler);
  bool_t halt = false;
  log_printf("| ├ Task \t\t#%s at %u:%u for %s\n", get_app_event_type_name(task->inciting_event.type), task->phase_index, task->step_index, get_actor_type_name(task->actor->class->type));
  while (!halt) {
    app_task_signal_t task_signal = app_task_advance(task, task->handler(task));
    switch (task_signal) {
      case APP_TASK_SUCCESS:
      case APP_TASK_FAILURE:
        if (task->phase_index == task_signal) {
          log_printf("│ │ ├ %s\t\t%u:%u\n", task_signal == APP_TASK_SUCCESS ? "Success" : "Failure", task->phase_index, task->step_index);
          if (task->actor->class->on_task != NULL) {
            task->actor->class->on_task(task->actor->object, task);
          }
        } else {
          app_task_finalize(task);
          actor_tick_catchup(task->actor, NULL);
          halt = true;
        }
        break;
      case APP_TASK_STEP_QUIT_ISR:
        if (IS_IN_ISR) {
          app_thread_actor_schedule(task->thread, task->actor, task->thread->current_time);
          log_printf("│ │ └ Yield from ISR\t%u:%u\n", task->phase_index, task->step_index);
          return 0;
        }
        break;
      case APP_TASK_STEP_SUCCESS:
        if (task->step_index != task_signal) {
          log_printf("│ │ ├ Step complete\t%u:%u\n", task->phase_index, task->step_index);
        }
        break;
      case APP_TASK_STEP_RETRY:
      case APP_TASK_STEP_CONTINUE:
        break;   
      case APP_TASK_STEP_WAIT:
        actor_event_finalize(task->actor, &task->awaited_event);  // free up room for a new event
        halt = true;
        break;
      default:
        halt = true;
    }
  }
  log_printf("│ │ └ Yield\t\t%u:%u\n", task->phase_index, task->step_index);
  return 0;
}

app_signal_t app_task_finalize(app_task_t *task) {
  actor_event_finalize(task->actor, &task->inciting_event);
  actor_event_finalize(task->actor, &task->awaited_event);
  task->handler = NULL;
  return 0;
}

app_task_signal_t app_task_advance(app_task_t *task, app_task_signal_t signal) {
  switch (task->phase_index) {
    case APP_TASK_FAILURE:
      task->phase_index = 0;
      return APP_TASK_FAILURE;

    case APP_TASK_SUCCESS:
      task->phase_index = 0;
      return APP_TASK_SUCCESS;

    default:
      break;
  }

  switch (task->step_index) {
    case APP_TASK_STEP_FAILURE:
      task->phase_index = APP_TASK_FAILURE;
      return APP_TASK_FAILURE;

    case APP_TASK_STEP_SUCCESS:
      task->phase_index++;
      task->step_index = 0;
      return APP_TASK_STEP_SUCCESS;

    default:
      break;
  }

  switch (signal) {
    case APP_TASK_SUCCESS:
      task->phase_index = APP_TASK_SUCCESS;
      break;
      
    case APP_TASK_FAILURE:
      task->phase_index = APP_TASK_FAILURE;
      break;

    case APP_TASK_RETRY:
      task->step_index = 0;
      task->phase_index = 0;
      break;

    case APP_TASK_STEP_RETRY:
      task->step_index = 0;
      break;

    case APP_TASK_STEP_WAIT:
    case APP_TASK_STEP_CONTINUE:
      task->step_index++;
      break;
      
    case APP_TASK_STEP_SUCCESS:
      task->step_index = APP_TASK_STEP_SUCCESS;
      break;

    case APP_TASK_STEP_FAILURE:
      task->step_index = APP_TASK_STEP_FAILURE;
      break;
      
    case APP_TASK_STEP_LOOP:
    case APP_TASK_STEP_QUIT_ISR:
       break;
  }
  return signal;
}

app_signal_t app_task_execute_if_running_in_thread(app_task_t *task, app_thread_t *thread) {
  if (task != NULL && task->handler != NULL && task->thread == thread) {
    return app_task_execute(task);
  }
  return 0;
}
