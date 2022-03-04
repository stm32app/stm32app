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

#include <stdint.h>
#include <stddef.h>

#ifdef HEAP_CONFIG
#define HEAP_CONFIG_PATH(x) HEAP_CONFIG_PATH2(x)
#define HEAP_CONFIG_PATH2(x) #x
#include HEAP_CONFIG_PATH(HEAP_CONFIG)
#else
#define HEAP_NUM 1
#define HEAP_REGIONS {{(uint8_t *)&ucHeap0, sizeof(ucHeap0)}};
#define RTOSREGION 0               // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 0
#define MALLOC_REGION 0      // malloc, calloc, realloc, heapsize : region 2 + region 1 + region 0
#define MALLOC_DMAREGION 0 // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 0
#define MALLOC_INTREGION 0  // multi_mallocfast, multi_calloc_fast, multi_realloc_fast, heapsize_int : region 1 + region 0
#define MALLOC_EXTREGION 0 // multi_malloc_ext, multi_calloc_ext, multi_realloc_ext, heapsize_ext : region 2
#endif


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
/*#ifndef PORTABLE_H
  void    *pvPortMalloc(size_t xWantedSize);
  void    vPortFree(void *pv);
  size_t  xPortGetFreeHeapSize(void);
  size_t  xPortGetMinimumEverFreeHeapSize(void);
  void    vPortInitialiseBlocks(void);
#endif*/

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
void    *multi_malloc_fast(size_t xWantedSize);
void    *multi_calloc_fast(size_t nmemb, size_t xWantedSize);
void    *multi_realloc_fast(void *pv, size_t xWantedSize); /* if pv is valid -> new pointer region is equal previsous pointer region */
size_t  heapsize_fast(void);

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