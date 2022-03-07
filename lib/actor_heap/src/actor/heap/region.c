#include "region.h"


void *actor_malloc_region(uint8_t region, size_t size) {
  if (region & ACTOR_REGION_DMA) {
    return actor_malloc_dma(size);
  }
  if (region & ACTOR_REGION_FAST) {
    return actor_malloc_fast(size);
  }
  if (region & ACTOR_REGION_EXT) {
    return actor_malloc_ext(size);
  }
  return actor_malloc_default(size);
};
void *actor_calloc_region(uint8_t region, size_t number, size_t size) {
  if (region & ACTOR_REGION_DMA) {
    return actor_calloc_dma(number, size);
  }
  if (region & ACTOR_REGION_FAST) {
    return actor_calloc_fast(number, size);
  }
  if (region & ACTOR_REGION_EXT) {
    return actor_calloc_ext(number, size);
  }
    return actor_calloc_default(number, size);
};
void *actor_realloc_region(uint8_t region, void *ptr, size_t size) {
  if (region & ACTOR_REGION_DMA) {
    return actor_realloc_dma(ptr, size);
  }
  if (region & ACTOR_REGION_FAST) {
    return actor_realloc_fast(ptr, size);
  }
  if (region & ACTOR_REGION_EXT) {
    return actor_realloc_ext(ptr, size);
  }
  return actor_realloc_default(ptr, size);
};
void actor_free_region(uint8_t region, void *ptr) {
    return actor_free_default(ptr);
};
size_t actor_size_region(uint8_t region) {
  if (region & ACTOR_REGION_DMA) {
    return heapsize_dma();
  }
  if (region & ACTOR_REGION_FAST) {
    return heapsize_fast();
  }
  if (region & ACTOR_REGION_EXT) {
    return heapsize_ext();
  }
  return heapsize_default();
};