#include "blank.h"

static ODR_t blank_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    actor_blank_t *blank = stream->object;
    (void)blank;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static actor_signal_t blank_validate(actor_blank_properties_t *properties) {
    return 0;
}

static actor_signal_t blank_construct(actor_blank_t *blank) {
    return 0;
}

static actor_signal_t blank_start(actor_blank_t *blank) {
    return 0;
}

static actor_signal_t blank_stop(actor_blank_t *blank) {
    return 0;
}

static actor_signal_t blank_link(actor_blank_t *blank) {
    return 0;
}

static actor_signal_t blank_phase_changed(actor_blank_t *blank, actor_phase_t phase) {
    return 0;
}

static actor_signal_t blank_signal_received(actor_blank_t *blank, actor_signal_t signal, actor_t *caller, void *source) {
    return 0;
}

static actor_signal_t blank_on_input(actor_blank_t *blank, actor_message_t *event, actor_worker_t *tick, actor_thread_t *thread) {
    return 0;
}

static actor_worker_callback_t blank_worker_assignment(actor_blank_t *blank, actor_thread_t *thread) {
    if (thread == blank->actor->node->input) {
        return (actor_worker_callback_t) blank_on_input;
    }
}

actor_interface_t actor_blank_class = {
    .type = 0,//ACTOR_MCU,
    .size = sizeof(actor_blank_t),
    .phase_subindex = 0,//ACTOR_BLANK_PHASE,
    .validate = (actor_method_t)blank_validate,
    .construct = blank_construct,
    .link = blank_link,
    .start = blank_start,
    .stop = blank_stop,
    .phase_changed = (actor_phase_changed_t)  blank_phase_changed,
    .signal_received = (actor_signal_received_t)blank_signal_received,
    .worker_assignment = blank_worker_assignment,
    .property_write = blank_property_write,
};
