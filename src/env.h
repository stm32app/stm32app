#ifndef INC_ENV
#define INC_ENV

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"
#include "lib/debug.h"
#include "../lib/multi_heap/multi_heap.h"

#define app_error_report(app, errorBit, errorCode, index) CO_errorReport(app->canopen->instance->em, errorBit, errorCode, index)
#define app_error_reset(app, errorBit, errorCode, index) CO_errorReset(app->canopen->instance->em, errorBit, errorCode, index)

#define actor_error_report(actor, errorBit, errorCode) CO_errorReport(actor->app->canopen->instance->em, errorBit, errorCode, actor->index)
#define actor_error_reset(actor, errorBit, errorCode) CO_errorReset(actor->app->canopen->instance->em, errorBit, errorCode, actor->index)


#endif