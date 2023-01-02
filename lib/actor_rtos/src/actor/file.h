#ifndef INC_CORE_FILE
#define INC_CORE_FILE

#include <actor/types.h>

typedef struct actor_file {
  actor_t* owner;     // actor that works with the file
  actor_t* storage;   // actor that handles FS operations
  actor_job_t* job;   // (optional) job that will receieve the results
  const char* path;   // point to filename
  int flags;          // flags with which file needs to be open
  uint32_t position;  // desired position in the file
  void* handle;       // custom data (e.g. can hold file handle)
  uint32_t size;      // total size of a file
} actor_file_t;

// lfs-compatible file modes
typedef enum actor_file_mode
{
  ACTOR_FILE_READ = 1,        // read
  ACTOR_FILE_WRITE = 2,       // write
  ACTOR_FILE_READ_WRITE = 3,  // read and wite

  ACTOR_FILE_CREATE = 0x0100,     // Create a file if it does not exist
  ACTOR_FILE_EXCLUSIVE = 0x0200,  // Fail if a file already exists
  ACTOR_FILE_TRUNCATE = 0x0400,   // Truncate the existing file to zero size
  ACTOR_FILE_APPEND = 0x0800,     // Move to end of file on every write
  ACTOR_FILE_ERROR = 0x080000     // Had error
} actor_file_mode_t;

actor_signal_t actor_file_publish_message_with_argument_for(actor_file_t* file,
                                                            actor_message_type_t type,
                                                            actor_t* target,
                                                            uint8_t* buffer,
                                                            uint32_t size);

actor_signal_t actor_file_open(actor_file_t* file, const char* path, int flags);

actor_signal_t actor_file_sync(actor_file_t* file);

actor_signal_t actor_file_seek(actor_file_t* file, uint32_t position, uint8_t whence);
actor_signal_t actor_file_stat(actor_file_t* file);

actor_signal_t actor_file_seek_to(actor_file_t* file, uint32_t position);

actor_signal_t actor_file_read(actor_file_t* file, uint8_t* data, uint8_t size);

actor_signal_t actor_file_write(actor_file_t* file, const uint8_t* data, uint8_t size);

actor_signal_t actor_file_truncate(actor_file_t* file);

actor_signal_t actor_file_delete(actor_file_t* file);

actor_signal_t actor_file_close(actor_file_t* file);

actor_signal_t actor_file_wait(actor_file_t* file);

#define actor_file_unlink actor_file_delete

#endif