#ifndef INC_CORE_ASYNC
#define INC_CORE_ASYNC

#include <actor/message.h>

typedef struct actor_async {
  // position of currently executed step
  uint32_t step;
  // maximum position that was reached by execution
  uint32_t max;
  // pointer to label to return to from deferred block
  uint32_t* target;
  // returned status
  actor_signal_t status;
  // Misc counter used for job-specific tasks
  uint32_t counter;
  // Misc retry counter for handling errors
  int8_t retries;
} actor_async_t;

#define actor_async_error (actor_async_signal < 0 ? actor_async_signal : 0)

// Establish async block using duff device
#define actor_async_begin(state, ...)         \
  __VA_ARGS__;                                \
  actor_async_t* async = &state;              \
  actor_signal_t actor_async_signal = signal; \
  switch (async->step) {                      \
    default:

#define actor_async_exit(signal) \
  async->step = -1;              \
  async->status = signal;        \
  actor_async_signal = signal;   \
  goto actor_async_cleanup;

#define actor_async_end(signal, cleanup, ...) \
  actor_async_exit(signal);                   \
  case -1:                                    \
  actor_async_cleanup : {                     \
    cleanup;                                  \
    __VA_ARGS__                               \
  }                                           \
    return signal;                            \
    }                                         \
    actor_async_signal = async->status;       \
    *async = (actor_async_t){};               \
    return async->status;

#define actor_async_assert(condition, signal) \
  if (!(condition))                           \
    actor_async_exit(signal);

#define actor_async_fail() actor_async_exit(ACTOR_SIGNAL_FAILURE)

#define actor_async_step(...)                                       \
  async->step = (__COUNTER__ + 1);                                  \
  async->max = async->step > async->max ? async->step : async->max; \
  __VA_ARGS__                                                       \
  /* fall through */                                                \
  case __COUNTER__:

// Yield if expression signals to wait, re-throw failures
#define actor_async_yield(expression)                                                     \
  actor_async_step(expression);                                                           \
  if (actor_async_signal < 0 || actor_async_signal == ACTOR_SIGNAL_WAIT) {                \
    debug_printf("%s: yield: %s\n", __func__, get_actor_signal_name(actor_async_signal)); \
    return actor_async_signal;                                                            \
  }

// Yield if expression signals to wait, re-throw failures
#define actor_async_await(expression)                                                           \
  actor_async_step();                                                                           \
  actor_async_signal = expression;                                                              \
  if (actor_async_signal < 0 || actor_async_signal == ACTOR_SIGNAL_WAIT) {                      \
    debug_printf("%s: await: %s @ [%s]\n", __func__, get_actor_signal_name(actor_async_signal), \
                 #expression);                                                                  \
    return actor_async_signal;                                                                  \
  }

// Yield if expression signals to wait, ignore failures
#define actor_async_try(expression)                                                         \
  actor_async_step();                                                                       \
  actor_async_signal = expression;                                                          \
  if (actor_async_signal == ACTOR_SIGNAL_WAIT) {                                          \
    debug_printf("%s: try [%s]: %s\n", __func__, get_actor_signal_name(actor_async_signal), \
                 #expression);                                                              \
    return actor_async_signal;                                                              \
  }

// Loop while condition is truthy
#define actor_async_while(condition) \
  actor_async_step();                \
  if (condition)                     \
    return ACTOR_SIGNAL_WAIT;

#define actor_async_defer(label, ...)        \
  actor_async_step();                        \
  deferred_##label : {                       \
    if (async->max > async->step) {          \
      printf("Deferred block %s\n", #label); \
      __VA_ARGS__;                           \
      goto * async->target;                   \
    }                                        \
  }

// Invoke deferred cleanup block
#define actor_async_undefer(label)  \
  async->target = &&undefer_##label; \
  goto deferred_##label;            \
  undefer_##label : actor_async_step();

// Invoke deferred cleanup block
#define actor_async_cleanup(label)       \
  async->target = &&cleanup_##label; \
  goto deferred_##label;            \
  cleanup_##label:

#define actor_async_until(condition) actor_async_while(!(condition))

// Yield execution
#define actor_async_sleep() actor_async_await(ACTOR_SIGNAL_WAIT);
#endif