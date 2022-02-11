#include "blank.h"

static ODR_t blank_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    actor_blank_t *blank = stream->object;
    (void)blank;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t blank_validate(actor_blank_properties_t *properties) {
    return 0;
}

static app_signal_t blank_construct(actor_blank_t *blank) {
    return 0;
}

static app_signal_t blank_start(actor_blank_t *blank) {
    return 0;
}

static app_signal_t blank_stop(actor_blank_t *blank) {
    return 0;
}

static app_signal_t blank_link(actor_blank_t *blank) {
    return 0;
}

static app_signal_t blank_on_phase(actor_blank_t *blank, actor_phase_t phase) {
    return 0;
}

static app_signal_t blank_on_input(actor_blank_t *blank, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    return 0;
}

static app_signal_t blank_on_signal(actor_blank_t *blank, actor_t *actor, app_signal_t signal, void *source) {
    return 0;
}

actor_class_t actor_blank_class = {
    .type = 0,//SYSTEM_MCU,
    .size = sizeof(actor_blank_t),
    .phase_subindex = 0,//ACTOR_BLANK_PHASE,
    .validate = (app_method_t)blank_validate,
    .construct = (app_method_t)blank_construct,
    .link = (app_method_t)blank_link,
    .start = (app_method_t)blank_start,
    .stop = (app_method_t)blank_stop,
    .on_phase = (actor_on_phase_t)blank_on_phase,
    .on_signal = (actor_on_signal_t)blank_on_signal,
//    .tick_input = (actor_on_worker_t) blank_on_input,
    .property_write = blank_property_write,
};
