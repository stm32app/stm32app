#include "stdlib.h"
#include <actor/stubs.h>
#include <actor/types.h>

__attribute__((weak)) void actor_disable_interrupts(void) {
}

__attribute__((weak)) void actor_enable_interrupts(void) {
}

__attribute__((weak)) void *actor_unbox(actor_t *actor) {
    return actor;
}

__attribute__((weak)) actor_t *actor_box(void *object) {
    return (actor_t *)object;
}

__attribute__((weak)) char *actor_stringify(actor_t *actor) {
    return "ACTOR";
};
__attribute__((weak)) char *actor_phase_stringify(int phase) {
    return "PHASE";
};
__attribute__((weak)) char *actor_message_stringify(actor_message_t *event) {
    return "EVENT";
};
__attribute__((weak)) char *actor_signal_stringify(int signal) {
    return "SIGNAL";
};

__attribute__((weak)) void actor_node_error_report(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index){

};

__attribute__((weak)) void actor_node_error_reset(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index){

};