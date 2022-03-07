#ifndef INC_CONFIG_HEAP
#define INC_CONFIG_HEAP

/*
- HEAP_NUM : how many heap memory region(s) (1..8)
- HEAP_REGIONS : memory address and size for regions (see the example)
- configTOTAL_HEAP_SIZE : shared (with stack and bss) memory region heap size
- RTOSREGION : which memory region does freertos use? (if you don't use freertos, you can delete it)
- MALLOC_REGION : which memory region(s) does the malloc, calloc, realloc, heapsize functions ?
                  - if you don't use this functions, you can delete it
                  - the order is also important, looking for free memory in the order listed
                  - unused regions can be marked with -1
                  - the number of region parameters must be equal to HEAP_NUM
- MALLOC_DMAREGION : which memory region(s) does the multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma functions ?
                  - the parameterization is the same as MALLOC_REGION
- MALLOC_INTREGION : which memory region(s) does the multi_malloc_fast, multi_calloc_fast, multi_realloc_fast, heapsize_int functions ?
                  - the parameterization is the same as MALLOC_REGION
- MALLOC_EXTREGION : which memory region(s) does the multi_malloc_ext, multi_calloc_ext, multi_realloc_ext, heapsize_ext functions ?
                  - the parameterization is the same as MALLOC_REGION
*/

/* Heap region number (1..8) */

#define configTOTAL_HEAP_SIZE ((size_t)(50 * 1024))

#define HEAP_NUM 3
#define HEAP_REGIONS {{(uint8_t *)&ucHeap0, sizeof(ucHeap0)}, {(uint8_t *)0x10000000, 0x10000}, {(uint8_t *)0x68000000, 0x100000}};
//#define RTOSREGION 0               // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 0
#define MALLOC_REGION 0, 1, 2      // malloc, calloc, realloc, heapsize : region 2 + region 1 + region 0
#define MALLOC_DMAREGION 0, -1, -1 // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 0
#define MALLOC_INTREGION 0, 1, -1  // multi_malloc_fast, multi_calloc_fast, multi_realloc_fast, heapsize_int : region 1 + region 0
#define MALLOC_EXTREGION 2, -1, -1 // multi_malloc_ext, multi_calloc_ext, multi_realloc_ext, heapsize_ext : region 2


#endif