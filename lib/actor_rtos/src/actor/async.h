#ifndef INC_CORE_ASYNC
#define INC_CORE_ASYNC

#include <actor/message.h>

// Establish async block using duff device
typedef struct actor_async_args {
  uint32_t timeout;
  uint8_t retries;
  uint8_t catch : 1;
} actor_async_args_t;

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

  uint8_t exited : 1;
  uint8_t retries;
  uint8_t timeouts;
} actor_async_t;

#define actor_async_error (actor_async_signal < 0 ? actor_async_signal : 0)

#define actor_async_begin(state, on_call, on_begin, on_end, on_step, on_yield) \
  on_call;                                                                     \
  actor_async_args_t actor_async_args = {};                                    \
  actor_async_t* async = &state;                                               \
  actor_signal_t actor_async_signal = signal;                                  \
  uint32_t* actor_async_jump = NULL;                                           \
  switch (async->step) {                                                       \
    case -2:                                                                   \
      /* Code to supress warnings */                                           \
      if (&actor_async_args != NULL)                                           \
        actor_async_jump = &&actor_async_on_step;                              \
      goto* actor_async_jump;                                                  \
      goto actor_async_on_step;                                                \
      goto actor_async_on_yield;                                               \
      goto actor_async_on_end;                                                 \
    /* Hook blocks */                                                          \
    actor_async_on_step:                                                       \
      on_step;                                                                 \
      goto* actor_async_jump;                                                  \
    actor_async_on_yield:                                                      \
      on_yield;                                                                \
      goto* actor_async_jump;                                                  \
    actor_async_on_end:                                                        \
      on_end;                                                                  \
      goto* actor_async_jump;                                                  \
      break;                                                                   \
    default:                                                                   \
      on_begin;

#define actor_async_end(signal, ...)                             \
  actor_async_exit(signal);                                      \
  case -1:                                                       \
  actor_async_cleanup : {                                        \
    _actor_async_hook(end, _actor_label_name(end, __COUNTER__)); \
    __VA_ARGS__                                                  \
  }                                                              \
    }                                                            \
    actor_async_signal = async->status;                          \
    *async = (actor_async_t){};                                  \
    return actor_async_signal;

#define _actor_async_hook(name, label_name) \
  actor_async_jump = &&label_name;          \
  goto actor_async_on_##name;               \
  label_name:

#define _actor_label_name(name, counter) name##counter

#define _actor_async_step(COUNTER, ...)                            \
  async->step = COUNTER;                                            \
  async->max = async->step > async->max ? async->step : async->max; \
  __VA_ARGS__                                                       \
  /* fall through */                                                \
  case COUNTER:                                                     \
    _actor_async_hook(step, _actor_label_name(step, COUNTER));

#define _actor_async_yield(COUNTER, ...)                       \
  actor_async_args = (actor_async_args_t){__VA_ARGS__};        \
  _actor_async_hook(yield, _actor_label_name(yield, COUNTER)); \
  return actor_async_signal;

#define actor_async_yield_with_suffix(COUNTER, expression, timeout...)                    \
  _actor_async_step(COUNTER, expression);                                                 \
  if (actor_async_signal < 0 || actor_async_signal == ACTOR_SIGNAL_WAIT) {                \
    debug_printf("%s: yield: %s\n", __func__, get_actor_signal_name(actor_async_signal)); \
    _actor_async_yield(COUNTER, timeout);                                                 \
  }

#define actor_async_await_with_suffix(COUNTER, expression, timeout...)                          \
  _actor_async_step(COUNTER);                                                                   \
  actor_async_signal = expression;                                                              \
  if ((actor_async_signal < 0 && !actor_async_args.catch) ||                                    \
      actor_async_signal == ACTOR_SIGNAL_WAIT) {                                                \
    debug_printf("%s: await: %s @ [%s]\n", __func__, get_actor_signal_name(actor_async_signal), \
                 #expression);                                                                  \
    _actor_async_yield(COUNTER, timeout);                                                       \
  }

#define actor_async_while_with_suffix(COUNTER, expression, timeout...) \
  _actor_async_step(COUNTER);                                          \
  if (expression) {                                                    \
    _actor_async_yield(COUNTER, timeout);                              \
  }

#define actor_async_cleanup_with_suffix(COUNTER, label)  \
  async->target = &&_actor_label_name(cleanup, COUNTER); \
  goto deferred_##label;                                 \
  _actor_label_name(cleanup, COUNTER) : _actor_async_step(__COUNTER__);

// Yield if expression signals to wait, re-throw failures
#define actor_async_yield(expression, timeout...) \
  actor_async_yield_with_suffix(__COUNTER__, expression, timeout)

// Yield if expression signals to wait, re-throw failures
#define actor_async_await(expression, timeout...) \
  actor_async_await_with_suffix(__COUNTER__, expression, timeout)

// Loop while condition is truthy
#define actor_async_while(condition, timeout...) \
  actor_async_while_with_suffix(__COUNTER__, condition, timeout)

// Loop until condition is truthy
#define actor_async_until(condition, timeout...) \
  actor_async_while_with_suffix(__COUNTER__, !(condition), timeout)

// Yield execution
#define actor_async_sleep(timeout...) actor_async_await(ACTOR_SIGNAL_WAIT, timeout);

// Defer cleanup block to run on trigger
#define actor_async_defer(label, ...)        \
  _actor_async_step(__COUNTER__);            \
  deferred_##label : {                       \
    if (async->max > async->step) {          \
      printf("Deferred block %s\n", #label); \
      __VA_ARGS__;                           \
      goto * async->target;                  \
    }                                        \
  }

// Invoke deferred cleanup block
#define actor_async_cleanup(label) actor_async_cleanup_with_suffix(__COUNTER__, label)

// Throw if assert failed
#define actor_async_assert(condition, signal) \
  if (!(condition))                           \
    actor_async_exit(signal);

// Throw generic error
#define actor_async_fail() actor_async_exit(ACTOR_SIGNAL_FAILURE)

// Exit with signal
#define actor_async_exit(signal) \
  async->step = -1;              \
  async->status = signal;        \
  async->exited = 1;             \
  actor_async_signal = signal;   \
  goto actor_async_cleanup;

#endif