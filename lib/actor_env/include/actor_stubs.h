#ifndef INC_ENV_MACROS
#define INC_ENV_MACROS

#include "actor_types.h"

#ifndef ACTOR_MALLOC
#include "stdlib.h"
#endif

void actor_disable_interrupts(void);
void actor_enable_interrupts(void);

void actor_node_error_report(actor_node_t* node, const uint8_t errorBit, uint16_t errorCode, uint32_t index);
void actor_node_error_reset(actor_node_t* node, const uint8_t errorBit, uint16_t errorCode, uint32_t index);

#define actor_error_report(actor, errorBit, errorCode) actor_node_error_report(actor->node, errorBit, errorCode, actor->index)
#define actor_error_reset(actor, errorBit, errorCode) actor_node_error_report(actor->node, errorBit, errorCode, actor->index)

char *actor_node_stringify(actor_t *actor);
void *actor_unbox(actor_t *actor);

#ifndef debug_printf
#define debug_printf(...)
#endif

#ifndef actor_assert
#define actor_assert(x)                                                                                                                      \
    if (!(x)) {                                                                                                                            \
        debug_printf("Assert failed\n");                                                                                                   \
        while (1) {                                                                                                                        \
        }                                                                                                                                  \
    }
#endif



#endif