/*
multi_heap.h
Created on: dec 14, 2020
    Author: Benjami
It is a multi-region memory management features abilities:
- manage multiple memory regions (up to 8 regions)
- freertos compatibility function name (pvPortMalloc, vPortFree, xPortGetFreeHeapSize)
- management of regions with different characteristics (dma capable, internal, external memory)
- you can set the order in which each region is occupied when reserving memory
- region independent free function (finds in which region the memory to be released is located)
- querying the total amount of free memory in several memory regions
*/

#ifndef __MULTI_HEAP_H_
#define __MULTI_HEAP_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "HeapConfig.h"
#include <stdint.h>
#include <stddef.h>

#define HEAP_SHARED    ucHeap0[configTOTAL_HEAP_SIZE]  /* shared (with stack and bss) memory region heap */

/*-----------------------------------------------------------------------------
  Fix section, do not change
*/

/* User definied memory region */
void    *multiRegionMalloc(uint32_t i, size_t xWantedSize);
void    *multiRegionCalloc(uint32_t i, size_t nmemb, size_t xWantedSize);
void    *multiRegionRealloc(uint32_t i, void *pv, size_t xWantedSize);
void    multiRegionFree(uint32_t i, void *pv);
size_t  multiRegionGetFreeHeapSize(uint32_t i);
size_t  multiRegionGetHeapSize(uint32_t i);
size_t  multiRegionGetMinimumEverFreeHeapSize(uint32_t i);
void *multiRegionGetHeapStartAddress(uint32_t i);
int32_t multiRegionSearch(void * pv);

/* Freertos memory region (RTOSREGION) */
#ifndef PORTABLE_H
  void    *pvPortMalloc(size_t xWantedSize);
  void    vPortFree(void *pv);
  size_t  xPortGetFreeHeapSize(void);
  size_t  xPortGetMinimumEverFreeHeapSize(void);
  void    vPortInitialiseBlocks(void);
#endif

/* Default memory region(s) (MALLOC_REGION) */
void    *multi_malloc(size_t xWantedSize);
void    *multi_calloc(size_t nmemb, size_t xWantedSize);
void    *multi_realloc(void *pv, size_t xWantedSize); /* if pv is valid -> new pointer region is equal previsous pointer region */
size_t  heapsize(void);

/* DMA capable memory region(s) (MALLOC_DMAREGION) */
void    *multi_malloc_dma(size_t xWantedSize);
void    *multi_calloc_dma(size_t nmemb, size_t xWantedSize);
void    *multi_realloc_dma(void *pv, size_t xWantedSize); /* if pv is valid -> new pointer region is equal previsous pointer region */
size_t  heapsize_dma(void);

/* Internal memory region(s) (MALLOC_INTREGION) */
void    *multi_malloc_int(size_t xWantedSize);
void    *multi_calloc_int(size_t nmemb, size_t xWantedSize);
void    *multi_realloc_int(void *pv, size_t xWantedSize); /* if pv is valid -> new pointer region is equal previsous pointer region */
size_t  heapsize_int(void);

/* External memory region(s)  (MALLOC_EXTREGION)*/
void    *multi_malloc_ext(size_t xWantedSize);
void    *multi_calloc_ext(size_t nmemb, size_t xWantedSize);
void    *multi_realloc_ext(void *pv, size_t xWantedSize); /* if pv is valid -> new pointer region is equal previsous pointer region */
size_t  heapsize_ext(void);

/* Region independence free (it first finds the region) */
void    multi_free(void *pv);

#ifdef __cplusplus
}
#endif

#endif /* __MULTI_HEAP_H_ */