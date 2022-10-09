#include <actor/file.h>
#include <actor/event.h>

actor_signal_t actor_file_publish_message_with_argument_for(actor_file_t *file, actor_message_type_t type, actor_t *target, uint8_t *buffer, uint32_t size) {
  if (file->job) {
    return actor_job_publish_message_with_argument_for(file->job, ACTOR_MESSAGE_OPEN, file->storage, NULL, 0, file);
  } else {
    return actor_publish_message_with_argument_for(file->owner, ACTOR_MESSAGE_OPEN, file->storage, NULL, 0, file);
  }
}

actor_signal_t actor_file_open(actor_file_t *file, const char *path, int flags) {
    file->flags = flags;
    file->path = path;
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_OPEN, file->storage, NULL, 0);
}

actor_signal_t actor_file_sync(actor_file_t *file) {
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_SYNC, file->storage, NULL, 0);
}

actor_signal_t actor_file_seek(actor_file_t *file, uint32_t position, uint8_t whence) {
    file->position = position;
    return 0;
}

actor_signal_t actor_file_stat(actor_file_t *file) {
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_STATS, file->storage, NULL, 0);
}

actor_signal_t actor_file_seek_to(actor_file_t *file, uint32_t position) {
    return actor_file_seek(file, position, 0);
}

actor_signal_t actor_file_read(actor_file_t *file, uint8_t *data, uint8_t size) {
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_READ, file->storage, data, size);
}

actor_signal_t actor_file_write(actor_file_t *file, const uint8_t *data, uint8_t size) {
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_WRITE, file->storage, (uint8_t *) data, size);
}

actor_signal_t actor_file_truncate(actor_file_t *file) {
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_ERASE, file->storage, NULL, 0);
}

actor_signal_t actor_file_delete(actor_file_t *file) {
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_DELETE, file->storage, NULL, 0);
}

actor_signal_t actor_file_close(actor_file_t *file) {
    return actor_file_publish_message_with_argument_for(file, ACTOR_MESSAGE_CLOSE, file->storage, NULL, 0);
}