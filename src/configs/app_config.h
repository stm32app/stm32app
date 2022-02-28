#ifndef INC_ACTOR_CONFIG
#define INC_ACTOR_CONFIG
#include "lib/debug.h"

#define actor_node_stringify(actor) actor->class->type


#define ACTOR_JOB_CALLBACK(job)                                                                                                              \
    if (job->actor->class->on_job_complete != NULL) {                                                                                      \
        job->actor->class->on_job_complete(job->actor->object, job);                                                                       \
    }

#define ACTOR_BUFFER_CUSTOMIZED_ALLOCATION(buffer, size)


#define ACTOR_BUFFER_INITIAL_SIZE 32
#define ACTOR_BUFFER_MAX_SIZE 1024 * 16
#define ACTOR_USE_DATABASE 0

#define DEBUG_LOG_LEVEL 5
#endif