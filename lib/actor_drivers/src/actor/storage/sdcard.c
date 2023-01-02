#include "sdcard.h"
#include <actor/buffer.h>
#include <actor/file.h>
#include <actor/transport/sdio.h>

static actor_signal_t sdcard_property_after_change(storage_sdcard_t* sdcard,
                                                   uint8_t index,
                                                   void* value,
                                                   void* old) {
  switch (index) {
    case STORAGE_SDCARD_BLOCK_SIZE:
      sdcard->fs_config->block_size = *((uint32_t*)value);
      break;
    case STORAGE_SDCARD_BLOCK_COUNT:
      sdcard->fs_config->block_count = *((uint32_t*)value);
      break;
  }
  return 0;
}

static actor_signal_t sdcard_validate(storage_sdcard_properties_t* properties) {
  return 0;
}

static int sdcard_lfs_read(const struct lfs_config* c,
                           lfs_block_t block,
                           lfs_off_t off,
                           void* buffer,
                           lfs_size_t size) {
  storage_sdcard_t* sdcard = c->context;
  actor_job_publish_message_with_argument_for(
      sdcard->job, ACTOR_MESSAGE_READ_TO_BUFFER, sdcard->sdio, buffer,
      size / sdcard->properties->block_size, (void*)(block + off / sdcard->properties->block_size));
  return 0;
}

static int sdcard_lfs_prog(const struct lfs_config* c,
                           lfs_block_t block,
                           lfs_off_t off,
                           const void* buffer,
                           lfs_size_t size) {
  storage_sdcard_t* sdcard = c->context;
  actor_job_publish_message_with_argument_for(
      sdcard->job, ACTOR_MESSAGE_WRITE, sdcard->sdio, (void*)buffer,
      size / sdcard->properties->block_size, (void*)(block + off / sdcard->properties->block_size));
  return 0;
}

static int sdcard_lfs_erase(const struct lfs_config* c, lfs_block_t block) {
  // actor_publish_message_with_argument_for(sdcard->actor, ACTOR_MESSAGE_ERASE,
  // sdcard->sdio->actor, NULL, 1,
  //                                      (void *)(block));
  // coru_yield();
  return 0;
}

static int sdcard_lfs_sync(const struct lfs_config* c) {
  return 0;
}

static actor_signal_t sdcard_construct(storage_sdcard_t* sdcard) {
  actor_message_subscribe(sdcard->actor, ACTOR_MESSAGE_DIAGNOSE);
  sdcard->fs = (lfs_t*)actor_malloc(sizeof(lfs_t));
  sdcard->fs_file = (lfs_file_t*)actor_malloc(sizeof(lfs_file_t));
  sdcard->fs_config = (struct lfs_config*)actor_malloc(sizeof(struct lfs_config));

  actor_thread_create(&sdcard->thread, sdcard->actor, (actor_procedure_t)actor_thread_execute,
                      "SDFS", 1024, 0, 4, ACTOR_THREAD_BLOCKABLE);

  sdcard->fs_config->read_size = sdcard->properties->fs_read_size;
  sdcard->fs_config->prog_size = sdcard->properties->fs_program_size;
  sdcard->fs_config->cache_size = sdcard->properties->fs_cache_size;
  sdcard->fs_config->lookahead_size = sdcard->properties->fs_lookahead_size;
  sdcard->fs_config->block_cycles = sdcard->properties->fs_block_cycles;
  sdcard->fs_config->read = sdcard_lfs_read;
  sdcard->fs_config->prog = sdcard_lfs_prog;
  sdcard->fs_config->erase = sdcard_lfs_erase;
  sdcard->fs_config->sync = sdcard_lfs_sync;
  sdcard->fs_config->context = sdcard;

  sdcard->lookahead_buffer =
      actor_buffer_aligned(sdcard->actor, sdcard->properties->fs_lookahead_size, 16);
  sdcard->read_buffer = actor_buffer_aligned(sdcard->actor, sdcard->properties->fs_read_size, 16);
  sdcard->prog_buffer =
      actor_buffer_aligned(sdcard->actor, sdcard->properties->fs_program_size, 16);

  sdcard->fs_config->read_buffer = sdcard->read_buffer->data;
  sdcard->fs_config->prog_buffer = sdcard->prog_buffer->data;
  sdcard->fs_config->lookahead_buffer = sdcard->lookahead_buffer->data;
  return 0;
}

static actor_signal_t sdcard_read(storage_sdcard_t* sdcard) {
  return 0;
}

static actor_signal_t sdcard_start(storage_sdcard_t* sdcard) {
  actor_set_phase(sdcard->actor, ACTOR_PHASE_PREPARATION);
  return 0;
}

static actor_signal_t sdcard_stop(storage_sdcard_t* sdcard) {
  return 0;
}

static actor_signal_t sdcard_link(storage_sdcard_t* sdcard) {
  actor_link(sdcard->actor, (void**)&sdcard->sdio, sdcard->properties->sdio_index, NULL);
  return 0;
}
static actor_signal_t sdcard_phase_changed(storage_sdcard_t* sdcard, actor_phase_t phase) {
  switch (phase) {
    case ACTOR_PHASE_CONSTRUCTION:
      return sdcard_construct(sdcard);
    case ACTOR_PHASE_LINKAGE:
      return sdcard_link(sdcard);
    case ACTOR_PHASE_START:
      return sdcard_start(sdcard);
    case ACTOR_PHASE_STOP:
      return sdcard_stop(sdcard);
    default:
      return 0;
  }
}

static actor_signal_t sdcard_message_handled(storage_sdcard_t* sdcard,
                                             actor_message_t* event,
                                             actor_signal_t signal) {
  actor_thread_actor_wakeup(sdcard->job->thread, sdcard->actor);
  return 0;
}

static actor_signal_t sdcard_signal_received(storage_sdcard_t* sdcard,
                                             actor_signal_t signal,
                                             actor_t* caller,
                                             void* source) {
  switch (signal) {
    default:
      break;
  }
  return 0;
}

static actor_signal_t sdcard_job_diagnose(actor_job_t* job) {
  return ACTOR_SIGNAL_JOB_COMPLETE;
}

static actor_signal_t sdcard_job_mount(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  storage_sdcard_t* sdcard = job->actor->object;

  actor_job_publish_message_for(job, ACTOR_MESSAGE_MOUNT, sdcard->sdio);

  if (sdcard->properties->block_size == 0) {
    debug_printf("SDFS - Failed to mount: Block size is 0");
  }
  int err = lfs_mount(sdcard->fs, sdcard->fs_config);

  // reformat if we can't mount the filesystem
  // this should only happen on the first boot
  if (err == LFS_ERR_CORRUPT) {
    lfs_format(sdcard->fs, sdcard->fs_config);
    err = lfs_mount(sdcard->fs, sdcard->fs_config);
    if (err) {
      return ACTOR_SIGNAL_CORRUPTED;
    }
  }

  // read current cou nt
  uint32_t boot_count = 0;
  lfs_file_open(sdcard->fs, sdcard->fs_file, "boot_count.txt", LFS_O_RDWR | LFS_O_CREAT);
  lfs_file_read(sdcard->fs, sdcard->fs_file, &boot_count, sizeof(boot_count));

  // update boot count
  boot_count += 1;
  lfs_file_rewind(sdcard->fs, sdcard->fs_file);
  lfs_file_write(sdcard->fs, sdcard->fs_file, &boot_count, sizeof(boot_count));
  // remember the storage is not updated until the file is closed successfully
  lfs_file_close(sdcard->fs, sdcard->fs_file);
  return ACTOR_SIGNAL_JOB_COMPLETE;
}

static actor_signal_t sdcard_job_open(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  storage_sdcard_t* sdcard = job->actor->object;
  actor_file_t* file = (actor_file_t*)job->inciting_message.argument;
  configASSERT(file);
  configASSERT(file->path);

  file->handle = actor_malloc(sizeof(lfs_file_t));
  lfs_file_open(sdcard->fs, file->handle, (const char*)file->path, file->flags);
  return ACTOR_SIGNAL_JOB_COMPLETE;
}

static actor_signal_t sdcard_job_close(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  storage_sdcard_t* sdcard = job->actor->object;
  actor_file_t* file = (actor_file_t*)job->inciting_message.argument;
  configASSERT(file);
  configASSERT(file->handle);

  lfs_file_close(sdcard->fs, file->handle);
  actor_free(file->handle);
  return ACTOR_SIGNAL_JOB_COMPLETE;
}

static actor_signal_t sdcard_job_erase(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  storage_sdcard_t* sdcard = job->actor->object;
  actor_file_t* file = (actor_file_t*)job->inciting_message.argument;
  configASSERT(file);
  configASSERT(file->path);

  lfs_remove(sdcard->fs, file->path);
  actor_free(file->handle);
  return ACTOR_SIGNAL_JOB_COMPLETE;
}

static actor_signal_t sdcard_job_read(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  storage_sdcard_t* sdcard = job->actor->object;
  actor_file_t* file = (actor_file_t*)job->inciting_message.argument;
  configASSERT(file);
  configASSERT(file->handle);

  uint32_t size = job->inciting_message.size;
  if (size == 0) {
    size = ((uint32_t)lfs_file_size(sdcard->fs, file->handle));
  }
  actor_buffer_t* buffer = actor_buffer_target_aligned(job->actor, job->inciting_message.data,
                                                       job->inciting_message.size, 16);
  if (lfs_file_read(sdcard->fs, file->handle, buffer->data, size) < 0) {
    return ACTOR_SIGNAL_FAILURE;
  } else {
    return ACTOR_SIGNAL_JOB_COMPLETE;
  }
}

static actor_signal_t sdcard_job_write(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  storage_sdcard_t* sdcard = job->actor->object;
  actor_file_t* file = (actor_file_t*)job->inciting_message.argument;
  configASSERT(file);
  configASSERT(file->handle);
  configASSERT(job->inciting_message.size);
  configASSERT(job->inciting_message.data);

  lfs_file_seek(sdcard->fs, file->handle, file->position, LFS_SEEK_SET);
  actor_buffer_t* buffer =
      actor_buffer_source(job->actor, job->inciting_message.data, job->inciting_message.size);
  if (lfs_file_write(sdcard->fs, file->handle, buffer->data, buffer->size) < 0) {
    return ACTOR_SIGNAL_FAILURE;
  }
  return 0;
}

static actor_signal_t sdcard_worker_thread(storage_sdcard_t* sdcard,
                                           actor_message_t* event,
                                           actor_worker_t* tick,
                                           actor_thread_t* thread) {
  actor_job_execute_if_running_in_thread(sdcard->job, thread);
  return 0;
}

static actor_signal_t sdcard_worker_on_input(storage_sdcard_t* sdcard,
                                             actor_message_t* event,
                                             actor_worker_t* tick,
                                             actor_thread_t* thread) {
  switch (event->type) {
    case ACTOR_MESSAGE_THREAD_START:
      debug_log_inhibited = false;
      return actor_message_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->thread,
                                                sdcard_job_mount);
    case ACTOR_MESSAGE_READ_TO_BUFFER:
    case ACTOR_MESSAGE_READ:
      return actor_message_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->thread,
                                                sdcard_job_read);
    case ACTOR_MESSAGE_WRITE:
      return actor_message_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->thread,
                                                sdcard_job_write);
    case ACTOR_MESSAGE_OPEN:
      return actor_message_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->thread,
                                                sdcard_job_open);
    case ACTOR_MESSAGE_CLOSE:
      return actor_message_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->thread,
                                                sdcard_job_close);
    case ACTOR_MESSAGE_ERASE:
      return actor_message_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->thread,
                                                sdcard_job_erase);
    case ACTOR_MESSAGE_STATS:
      return actor_message_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->thread,
                                                sdcard_job_open);
    default:
      break;
  }
  return 0;
}

static actor_worker_callback_t sdcard_worker_assignment(storage_sdcard_t* sdcard,
                                                        actor_thread_t* thread) {
  if (thread == sdcard->actor->node->input) {
    return (actor_worker_callback_t)sdcard_worker_on_input;
  } else if (thread == sdcard->thread) {
    return (actor_worker_callback_t)sdcard_worker_thread;
  }
  return NULL;
}

actor_interface_t storage_sdcard_class = {
    .type = STORAGE_SDCARD,
    .size = sizeof(storage_sdcard_t),
    .phase_subindex = STORAGE_SDCARD_PHASE,
    .validate = (actor_method_t)sdcard_validate,
    .phase_changed = (actor_phase_changed_t)sdcard_phase_changed,
    .signal_received = (actor_signal_received_t)sdcard_signal_received,
    .message_handled = (actor_message_handled_t)sdcard_message_handled,
    .worker_assignment = (actor_worker_assignment_t)sdcard_worker_assignment,
    .property_after_change = (actor_property_changed_t)sdcard_property_after_change,
};
