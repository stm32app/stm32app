#ifndef LIB_HEAP_H
#define LIB_HEAP_H


#include <stddef.h>
#include "FreeRTOS.h"
#include "multi_heap.h"

#define malloc(size) (debug_printf("    ! Malloc %db\t\t%s \n", size, __func__), multi_malloc(size))
#define malloc_dma(size) (debug_printf("    ! Malloc DMA %db\t\t%s \n", size, __func__), multi_malloc_dma(size))
#define malloc_ext(size) (debug_printf("    ! Malloc EXT %db\t\t%s \n", size, __func__), multi_malloc_ext(size))
#define malloc_int(size) (debug_printf("    ! Malloc INT %db\t\t%s \n", size, __func__), multi_malloc_int(size))
#define calloc(size) (debug_printf("    ! Calloc %db\t\t%s \n", size, __func__), multi_calloc(size))
#define calloc_dma(size) (debug_printf("    ! Calloc DMA %db\t\t%s \n", size, __func__), multi_calloc_dma(size))
#define calloc_ext(size) (debug_printf("    ! Calloc EXT %db\t\t%s \n", size, __func__), multi_calloc_ext(size))
#define calloc_int(size) (debug_printf("    ! Calloc INT %db\t\t%s \n", size, __func__), multi_calloc_int(size))
#define realloc(ptr, size) (debug_printf("    ! Realloc %lub\t\t%s \n", size, __func__), multi_realloc(ptr, size))
#define realloc_dma(ptr, size) (debug_printf("    ! Realloc DMA %lub\t\t%s \n", size, __func__), multi_realloc_dma(ptr, size))
#define realloc_ext(ptr, size) (debug_printf("    ! Realloc EXT %lub\t\t%s \n", size, __func__), multi_realloc_ext(ptr, size))
#define realloc_int(ptr, size) (debug_printf("    ! Realloc INT %lub\t\t%s \n", size, __func__), multi_realloc_int(ptr, size))
#define free(ptr) (debug_printf("    ! Free %s\t\t\n", __func__), multi_free(ptr))

#endif