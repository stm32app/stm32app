#ifndef INC_ENV_MACROS
#define INC_ENV_MACROS

#include "app_types.h"

#ifndef APP_MALLOC
#include "stdlib.h"
#endif

void app_disable_interrupts(void);
void app_enable_interrupts(void);

void *app_malloc(uint32_t size);
void app_free(void *ptr);

#ifndef APP_MALLOC
#define APP_MALLOC malloc
#endif

#ifndef APP_REALLOC
#define APP_REALLOC realloc
#endif

#ifndef APP_REALLOC_DMA
#define APP_REALLOC_DMA realloc
#endif

#ifndef APP_REALLOC_EXT
#define APP_REALLOC_EXT realloc
#endif

#ifndef APP_FREE
#define APP_FREE free
#endif

char *app_actor_stringify(actor_t *actor);
void *actor_unbox(actor_t *actor);

#ifndef debug_printf
#define debug_printf(...)
#endif

#ifndef APP_ASSERT
#define APP_ASSERT(x)                                                                                                                      \
    if (!(x)) {                                                                                                                            \
        debug_printf("Assert failed\n");                                                                                                   \
        while (1) {                                                                                                                        \
        }                                                                                                                                  \
    }
#endif

#endif