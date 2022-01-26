#include "task.h"


app_signal_t app_task_execute(app_task_t *task) {
  configASSERT(task);
  bool_t halt = false;
  while (!halt) {
    app_task_signal_t task_signal = app_task_advance(task);
    switch (task_signal) {
      case APP_TASK_STEP_RETRY:
      case APP_TASK_STEP_COMPLETE:
         break;
      case APP_TASK_COMPLETE:
      case APP_TASK_HALT:
        app_task_finalize(task);
        actor_tick_catchup(task->actor, task->tick);
        return task_signal;
      case APP_TASK_STEP_WAIT:
        actor_event_finalize(task->actor, &task->awaited_event);  // free up room for a new event
        return task_signal;
      default:
        return task_signal;
    }
  }
  return APP_TASK_COMPLETE;
}

app_task_signal_t app_task_advance(app_task_t *task) {
  app_task_signal_t signal = task->handler(task);
  switch (signal) {
    case APP_TASK_CONTINUE:
    case APP_TASK_COMPLETE:
      task->phase_index++;
      break;
      
    case APP_TASK_RETRY:
      task->step_index = 0;
      task->phase_index = 0;
      break;
      
    case APP_TASK_HALT:
      task->phase_index = APP_TASK_HALT_INDEX;
      break;

    case APP_TASK_STEP_RETRY:
      task->step_index = 0;
      break;

    case APP_TASK_STEP_WAIT:
      task->step_index++;
      break;

    case APP_TASK_STEP_COMPLETE:
      task->phase_index++;
      task->step_index = 0;
      break;
      
    case APP_TASK_STEP_HALT:
      task->step_index = APP_TASK_HALT_INDEX;
      break;
  }
  return signal;
}

app_signal_t app_task_finalize(app_task_t *task) {
  if (task->actor->class->on_task != NULL) {
    task->actor->class->on_task(task->actor->object, task);
  }
  actor_event_finalize(task->actor, &task->inciting_event);
  actor_event_finalize(task->actor, &task->awaited_event);
  return 0;
}
