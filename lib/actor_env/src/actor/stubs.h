#ifndef INC_ENV_MACROS
#define INC_ENV_MACROS

#include <actor/types.h>

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
actor_t *actor_box(void *object);


#define actor_malloc(size) (trace_printf("    ! Malloc %db\t\t%s \n", size, __func__), multi_malloc(size))
#define actor_malloc_dma(size) (trace_printf("    ! Malloc DMA %db\t\t%s \n", size, __func__), multi_malloc_dma(size))
#define actor_malloc_ext(size) (trace_printf("    ! Malloc EXT %db\t\t%s \n", size, __func__), multi_malloc_ext(size))
#define actor_malloc_fast(size) (trace_printf("    ! Malloc INT %db\t\t%s \n", size, __func__), multi_malloc_fast(size))
#define actor_calloc(number, size) (trace_printf("    ! Calloc %db\t\t%s \n", size, __func__), multi_calloc(number, size))
#define actor_calloc_dma(number, size) (trace_printf("    ! Calloc DMA %db\t\t%s \n", size, __func__), multi_calloc_dma(number, size))
#define actor_calloc_ext(number, size) (trace_printf("    ! Calloc EXT %db\t\t%s \n", size, __func__), multi_calloc_ext(number, size))
#define actor_calloc_fast(number, size) (trace_printf("    ! Calloc INT %db\t\t%s \n", size, __func__), multi_calloc_fast(number, size))
#define actor_realloc(ptr, size) (trace_printf("    ! Realloc %lub\t\t%s \n", size, __func__), multi_realloc(ptr, size))
#define actor_realloc_dma(ptr, size) (trace_printf("    ! Realloc DMA %lub\t\t%s \n", size, __func__), multi_realloc_dma(ptr, size))
#define actor_realloc_ext(ptr, size) (trace_printf("    ! Realloc EXT %lub\t\t%s \n", size, __func__), multi_realloc_ext(ptr, size))
#define actor_realloc_fast(ptr, size) (trace_printf("    ! Realloc INT %lub\t\t%s \n", size, __func__), multi_realloc_fast(ptr, size))
#define actor_free(ptr) (trace_printf("    ! Free %s\t\t\n", __func__), multi_free(ptr))



#endif