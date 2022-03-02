#include "stdlib.h"
#include <actor/stubs.h>
#include <actor/types.h>

__attribute__((weak)) void actor_disable_interrupts(void) {
}

__attribute__((weak)) void actor_enable_interrupts(void) {
}

__attribute__((weak)) actor_t *actor_unbox(actor_t *actor) {
    return actor;
}

__attribute__((weak)) void *actor_box(actor_t *actor) {
    return actor;
}

__attribute__((weak)) char *actor_node_stringify(actor_t *actor) {
    return "ACTOR";
};

__attribute((weak)) void actor_node_error_report(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index){

};

__attribute((weak)) void actor_node_error_reset(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index){

};