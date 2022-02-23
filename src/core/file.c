#include "core/file.h"

app_job_signal_t app_file_wait(app_file_t *file) {
  if (file->job) {
    return app_job_wait(file->job);
  } else {
    return APP_JOB_TASK_WAIT;
  }
}

app_job_signal_t app_file_open(app_file_t *file, const char *path, int flags) {
    file->flags = flags;
    file->path = path;
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_OPEN, file->storage, NULL, 0, file);
    return app_file_wait(file);
}

app_job_signal_t app_file_sync(app_file_t *file) {
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_SYNC, file->storage, NULL, 0, file);
    return app_file_wait(file);
}

app_job_signal_t app_file_seek(app_file_t *file, uint32_t position, uint8_t whence) {
    file->position = position;
    return app_file_wait(file);
}

app_file_t *app_file_stat(app_file_t *file) {
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_STATS, file->storage, NULL, 0, file);
    return file;
}

app_job_signal_t app_file_seek_to(app_file_t *file, uint32_t position) {
    return app_file_seek(file, position, 0);
}

app_job_signal_t app_file_read(app_file_t *file, uint8_t *data, uint8_t size) {
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_READ, file->storage, data, size, file);
    return app_file_wait(file);
}

app_job_signal_t app_file_write(app_file_t *file, const uint8_t *data, uint8_t size) {
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_WRITE, file->storage, (uint8_t *) data, size, file);
    return app_file_wait(file);
}

app_job_signal_t app_file_truncate(app_file_t *file) {
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_ERASE, file->storage, NULL, 0, file);
    return app_file_wait(file);
}

app_job_signal_t app_file_delete(app_file_t *file) {
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_DELETE, file->storage, NULL, 0, file);
    return app_file_wait(file);
}

app_job_signal_t app_file_close(app_file_t *file) {
    actor_publish_event_with_argument_for(file->owner, APP_EVENT_CLOSE, file->storage, NULL, 0, file);
    return app_file_wait(file);
}