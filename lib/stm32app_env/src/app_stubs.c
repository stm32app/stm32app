#include <app_stubs.h>
#include <app_types.h>
#include "stdlib.h"

__attribute__((weak)) void app_disable_interrupts(void) {
}

__attribute__((weak)) void app_enable_interrupts(void) {
}


__attribute__((weak)) void *actor_unbox(actor_t *actor) {
  return actor;
}

__attribute__((weak)) char *app_actor_stringify(actor_t *actor) {
  return "ACTOR";
};

__attribute__((weak)) void *app_malloc(uint32_t size) {
  return malloc(size);
};

__attribute__((weak)) void app_free(void *ptr) {
  free(ptr);
};