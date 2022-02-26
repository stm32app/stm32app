#ifndef INC_CORE_FILE
#define INC_CORE_FILE

#include "core/actor.h"
#include "core/types.h"

typedef struct app_file {
    actor_t *owner;     // actor that works with the file
    actor_t *storage;   // actor that handles FS operations
    app_job_t *job;     // (optional) job that will receieve the results
    const char *path;      // point to filename
    int flags;          // flags with which file needs to be open
    uint32_t position; // desired position in the file
    void *handle;       // custom data (e.g. can hold file handle)
    uint32_t size;      // total size of a file
} app_file_t;

// lfs-compatible file modes
typedef enum app_file_mode {
    APP_FILE_READ = 1,       // read
    APP_FILE_WRITE = 2,      // write
    APP_FILE_READ_WRITE = 3, // read and wite

    APP_FILE_CREATE = 0x0100,    // Create a file if it does not exist
    APP_FILE_EXCLUSIVE = 0x0200, // Fail if a file already exists
    APP_FILE_TRUNCATE = 0x0400,  // Truncate the existing file to zero size
    APP_FILE_APPEND = 0x0800,    // Move to end of file on every write
    APP_FILE_ERROR = 0x080000    // Had error
} app_file_mode_t;

app_signal_t app_file_publish_event_with_argument_for(app_file_t *file, app_event_type_t type, actor_t *target, uint8_t *buffer, uint32_t size);

app_signal_t app_file_open(app_file_t *file, const char *path, int flags);

app_signal_t app_file_sync(app_file_t *file);

app_signal_t app_file_seek(app_file_t *file, uint32_t position, uint8_t whence);
app_signal_t app_file_stat(app_file_t *file);

app_signal_t app_file_seek_to(app_file_t *file, uint32_t position);

app_signal_t app_file_read(app_file_t *file, uint8_t *data, uint8_t size);

app_signal_t app_file_write(app_file_t *file, const uint8_t *data, uint8_t size);

app_signal_t app_file_truncate(app_file_t *file);

app_signal_t app_file_delete(app_file_t *file);

app_signal_t app_file_close(app_file_t *file);

app_signal_t app_file_wait(app_file_t *file);

#define app_file_unlink app_file_delete

#endif