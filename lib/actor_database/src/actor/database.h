#ifndef INC_ACTOR_DATABASE
#define INC_ACTOR_DATABASE

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/actor.h>
#include "sqlite_config.h"
#include "sqlite3.h"



struct actor_database {
    actor_t *actor;
    actor_database_properties_t *properties;

    actor_thread_t *thread;
    actor_worker_t worker;
    actor_job_t *job;

    actor_generic_device_t *storage;
    actor_buffer_t *journal_buffer;
    
    sqlite3 *connection;
    sqlite3_vfs vfs;
    sqlite3_io_methods vfs_io;
};


extern actor_class_t actor_database_class;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif