#include <actor/heap.h>
#include <stdlib.h>

__attribute__((weak)) void *_actor_malloc_region(uint8_t region, size_t size) {
    return malloc(size);
};
__attribute__((weak)) void *_actor_calloc_region(uint8_t region, size_t number, size_t size) {
    return calloc(number, size);
};
__attribute__((weak)) void *_actor_realloc_region(uint8_t region, void *ptr, size_t size) {
    return realloc(ptr, size);
};
__attribute__((weak)) void _actor_free_region(uint8_t region, void *ptr) {
    return free(ptr);
};
__attribute__((weak)) size_t _actor_size_region(uint8_t region) {
    return 0;
};