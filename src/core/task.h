#ifndef INC_CORE_TASK
#define INC_CORE_TASK

#include "app.h"
#include "core/event.h"

#define APP_TASK_HALT_INDEX 64
#define APP_TASK_SUCCESS_INDEX -1

struct app_task {
  char *name;

  actor_t *actor;
  app_thread_t *thread;

  app_event_t inciting_event;
  app_event_t awaited_event;

  uint32_t counter;

  // 
  uint8_t *data;
  size_t size;
  
  size_t phase_index;
  size_t step_index;

  actor_on_task_t handler;
};

enum app_task_signal {
  APP_TASK_CONTINUE,
  APP_TASK_COMPLETE,
  APP_TASK_RETRY,
  APP_TASK_HALT,
  APP_TASK_FAILURE,
  APP_TASK_STEP_RETRY,
  APP_TASK_STEP_WAIT,
  APP_TASK_STEP_COMPLETE,
  APP_TASK_STEP_CONTINUE,
  APP_TASK_STEP_HALT,
  APP_TASK_STEP_LOOP,
};

app_signal_t app_task_execute(app_task_t *task);
app_task_signal_t app_task_advance(app_task_t *task);
app_signal_t app_task_finalize(app_task_t *task);

#endif