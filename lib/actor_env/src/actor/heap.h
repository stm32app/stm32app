#include <actor/types.h>
#ifndef INC_ACTOR_HEAP
#define INC_ACTOR_HEAP

#define ACTOR_REGION_DEFAULT (1 << 0)
#define ACTOR_REGION_FAST (1 << 1)
#define ACTOR_REGION_EXT (1 << 2)
#define ACTOR_REGION_DMA (1 << 3)

#define actor_malloc(size) actor_malloc_region(0, size)
#define actor_calloc(number, size) actor_calloc_region(0, number, size)
#define actor_realloc(ptr, size) actor_realloc_region(0, ptr, size)
#define actor_free(ptr) actor_free_region(0, ptr)

void *actor_malloc_region(uint8_t region, size_t size);
void *actor_calloc_region(uint8_t region, size_t number, size_t size);
void *actor_realloc_region(uint8_t region, void *ptr, size_t size);
void actor_free_region(uint8_t region, void *ptr);
size_t actor_size_region(uint8_t region);

#endif