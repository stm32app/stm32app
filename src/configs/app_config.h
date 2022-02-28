#ifndef INC_APP_CONFIG
#define INC_APP_CONFIG
#include "lib/debug.h"

#define app_actor_stringify(actor) actor->class->type


#define app_disable_interrupts() cm_disable_interrupts();
#define app_enable_interrupts() cm_enable_interrupts();

#define APP_ASSERT(condition) debug_assert(condition)

#define APP_JOB_CALLBACK(job)                                                                                                              \
    if (job->actor->class->on_job_complete != NULL) {                                                                                      \
        job->actor->class->on_job_complete(job->actor->object, job);                                                                       \
    }

#define APP_BUFFER_CUSTOMIZED_ALLOCATION(buffer, size)

#define app_error_report(app, errorBit, errorCode, index) CO_errorReport(app->canopen->instance->em, errorBit, errorCode, index)
#define app_error_reset(app, errorBit, errorCode, index) CO_errorReset(app->canopen->instance->em, errorBit, errorCode, index)

#define actor_error_report(actor, errorBit, errorCode) CO_errorReport(actor->app->canopen->instance->em, errorBit, errorCode, actor->index)
#define actor_error_reset(actor, errorBit, errorCode) CO_errorReset(actor->app->canopen->instance->em, errorBit, errorCode, actor->index)


#define APP_BUFFER_INITIAL_SIZE 32
#define APP_BUFFER_MAX_SIZE 1024 * 16
#define APP_USE_DATABASE 0

#define DEBUG_LOG_LEVEL 5
#endif