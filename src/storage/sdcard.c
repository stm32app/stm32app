#include "sdcard.h"

static ODR_t sdcard_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    storage_sdcard_t *sdcard = stream->object;
    (void)sdcard;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t sdcard_validate(storage_sdcard_properties_t *properties) {
    return 0;
}

static app_signal_t sdcard_construct(storage_sdcard_t *sdcard) {
    return 0;
}

static app_signal_t sdcard_start(storage_sdcard_t *sdcard) {
    return 0;
}

static app_signal_t sdcard_stop(storage_sdcard_t *sdcard) {
    return 0;
}

static app_signal_t sdcard_link(storage_sdcard_t *sdcard) {
    return 0;
}

static app_signal_t sdcard_on_phase(storage_sdcard_t *sdcard, actor_phase_t phase) {
    return 0;
}

static app_signal_t sdcard_on_signal(storage_sdcard_t *sdcard, actor_t *actor, app_signal_t signal, void *source) {
    return 0;
}

static app_signal_t sdcard_on_input(storage_sdcard_t *sdcard, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
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
    .property_write = sdcard_property_write,
};
