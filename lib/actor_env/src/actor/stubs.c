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
    return (actor_t *) object;
}

__attribute__((weak)) char *actor_node_stringify(actor_t *actor) {
    return "ACTOR";
};
__attribute__((weak)) char *actor_event_stringify(actor_event_t *event) {
    return "EVENT";
};
__attribute__((weak)) char *actor_signal_stringify(int signal) {
    return "SIGNAL";
};

__attribute__((weak)) void actor_node_error_report(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index){

};

__attribute__((weak)) void actor_node_error_reset(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index){

};


__attribute__((weak)) void *actor_malloc(uint32_t size) { return  malloc(size); };
__attribute__((weak)) void *actor_malloc_dma(uint32_t size){ return malloc(size); };
__attribute__((weak)) void *actor_malloc_ext(uint32_t size){ return malloc(size); };
__attribute__((weak)) void *actor_malloc_fast(uint32_t size){ return malloc(size); };
__attribute__((weak)) void *actor_calloc(uint32_t number, uint32_t size) { return calloc(number, size); };;
__attribute__((weak)) void *actor_calloc_dma(uint32_t number, uint32_t size){ return calloc(number, size); };;
__attribute__((weak)) void *actor_calloc_ext(uint32_t number, uint32_t size){ return calloc(number, size); };;
__attribute__((weak)) void *actor_calloc_fast(uint32_t number, uint32_t size){ return calloc(number, size); };;
__attribute__((weak)) void *actor_realloc(void *ptr, uint32_t size){ return realloc(ptr, size); };;
__attribute__((weak)) void *actor_realloc_dma(void *ptr, uint32_t size){ return realloc(ptr, size);  };;
__attribute__((weak)) void *actor_realloc_ext(void *ptr, uint32_t size){ return realloc(ptr, size); };;
__attribute__((weak)) void *actor_realloc_fast(void *ptr, uint32_t size){ return realloc(ptr, size); };;
__attribute__((weak)) void actor_free(void *ptr){ free(ptr); };;
