#include "sdcard.h"
#include "transport/sdio.h"
#include "core/buffer.h"

static ODR_t sdcard_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    storage_sdcard_t *sdcard = stream->object;
    (void)sdcard;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);

    switch (stream->subIndex) {
    case STORAGE_SDCARD_BLOCK_SIZE:
        sdcard->fs_config.block_size = *((uint32_t *)stream->dataOrig);
        break;
    case STORAGE_SDCARD_BLOCK_COUNT:
        sdcard->fs_config.block_count = *((uint32_t *)stream->dataOrig);
        break;
    }
    return result;
}

static app_signal_t sdcard_validate(storage_sdcard_properties_t *properties) {
    return 0;
}

static int sdcard_lfs_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    return 0;

}

static int sdcard_lfs_prog(const struct lfs_config *c, lfs_block_t block,
            lfs_off_t off, const void *buffer, lfs_size_t size) {
    return 0;
}

static int sdcard_lfs_erase(const struct lfs_config *c, lfs_block_t block) {
    return 0;
}

static int sdcard_lfs_sync(const struct lfs_config *c) {
    return 0;
}


static app_signal_t sdcard_construct(storage_sdcard_t *sdcard) {
    sdcard->fs_config.read_size = sdcard->properties->fs_read_size;
    sdcard->fs_config.prog_size = sdcard->properties->fs_program_size;
    sdcard->fs_config.cache_size = sdcard->properties->fs_cache_size;
    sdcard->fs_config.lookahead_size = sdcard->properties->fs_lookahead_size;
    sdcard->fs_config.block_cycles = sdcard->properties->fs_block_cycles;
    sdcard->fs_config.read = sdcard_lfs_read;
    sdcard->fs_config.prog = sdcard_lfs_prog;
    sdcard->fs_config.erase = sdcard_lfs_erase;
    sdcard->fs_config.sync = sdcard_lfs_sync;

    sdcard->lookahead_buffer = app_buffer_aligned(sdcard->actor, sdcard->properties->fs_lookahead_size, 16);
    sdcard->read_buffer = app_buffer_aligned(sdcard->actor, sdcard->properties->fs_read_size, 16);
    sdcard->prog_buffer = app_buffer_aligned(sdcard->actor, sdcard->properties->fs_program_size, 16);

    sdcard->fs_config.read_buffer = sdcard->read_buffer->data;
    sdcard->fs_config.prog_buffer = sdcard->prog_buffer->data;
    sdcard->fs_config.lookahead_buffer = sdcard->lookahead_buffer->data;
    return 0;
}

static app_signal_t sdcard_mount(storage_sdcard_t *sdcard) {
    return app_publish(sdcard->actor->app, &((app_event_t){
                                               .type = APP_EVENT_MOUNT,
                                               .producer = sdcard->actor,
                                               .consumer = sdcard->sdio->actor,
                                           }))
}
static app_signal_t sdcard_unmount(storage_sdcard_t *sdcard) {
    return app_publish(sdcard->actor->app, &((app_event_t){
                                               .type = APP_EVENT_UNMOUNT,
                                               .producer = sdcard->actor,
                                               .consumer = sdcard->sdio->actor,
                                           }));
}
static app_signal_t sdcard_read(storage_sdcard_t *sdcard) {
    return app_publish(sdcard->actor->app, &((app_event_t){
                                               .type = APP_EVENT_UNMOUNT,
                                               .producer = sdcard->actor,
                                               .consumer = sdcard->sdio->actor,
                                           }));
}

static void sdcard_io_coroutine(void *context) {
    //storage_sdcard_t *sdcard = context;

}

static app_signal_t sdcard_start(storage_sdcard_t *sdcard) {
    sdcard_mount(sdcard);

    coru_create(sdcard->io, sdcard_io_coroutine, &sdcard, 128);

    actor_set_phase(sdcard->actor, ACTOR_PREPARING);
    return 0;
}


static app_signal_t sdcard_stop(storage_sdcard_t *sdcard) {
    return 0;
}

static app_signal_t sdcard_link(storage_sdcard_t *sdcard) {
    actor_link(sdcard->actor, (void **)&sdcard->sdio, sdcard->properties->sdio_index, NULL);
    return 0;
}
static app_signal_t sdcard_on_phase(storage_sdcard_t *sdcard, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PREPARING:
        sdcard_mount(sdcard);
        break;
    default:
        break;
    }
    return 0;
}

static app_signal_t sdcard_on_report(storage_sdcard_t *sdcard, app_event_t *event) {
    switch (event->type) {
    case APP_EVENT_MOUNT:
        app_thread_actor_schedule(sdcard->job.thread, sdcard->actor, sdcard->job.thread->current_time);
        break;
    case APP_EVENT_UNMOUNT:
        break;
    default:
        break;
    }
    return 0;
}

static app_signal_t sdcard_on_signal(storage_sdcard_t *sdcard, actor_t *actor, app_signal_t signal, void *source) {
    switch (signal) {
    default:
        break;
    }
    return 0;
}

static app_job_signal_t sdcard_job_diagnose(app_job_t *job) {
    switch (job->job_phase) {
    case 0:
        break;
    }

    return APP_JOB_SUCCESS;
}

static app_signal_t sdcard_worker_medium_priority(storage_sdcard_t *sdcard, app_event_t *event, actor_worker_t *tick,
                                                  app_thread_t *thread) {
    return app_job_execute_if_running_in_thread(&sdcard->job, thread);
}

static app_signal_t sdcard_on_input(storage_sdcard_t *sdcard, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_DIAGNOSE:
        return actor_event_handle_and_start_job(sdcard->actor, event, &sdcard->job, sdcard->actor->app->threads->medium_priority,
                                                sdcard_job_diagnose);
    default:
        break;
    }
    return 0;
}

actor_class_t storage_sdcard_class = {
    .type = STORAGE_SDCARD,
    .size = sizeof(storage_sdcard_t),
    .phase_subindex = STORAGE_SDCARD_PHASE,
    .validate = (app_method_t)sdcard_validate,
    .construct = (app_method_t)sdcard_construct,
    .link = (app_method_t)sdcard_link,
    .start = (app_method_t)sdcard_start,
    .stop = (app_method_t)sdcard_stop,
    .on_phase = (actor_on_phase_t)sdcard_on_phase,
    .on_signal = (actor_on_signal_t)sdcard_on_signal,
    .worker_input = (actor_on_worker_t)sdcard_on_input,
    .worker_medium_priority = (actor_on_worker_t)sdcard_worker_medium_priority,
    .property_write = sdcard_property_write,
};
