#ifndef INC_CONFIG_HEAP
#define INC_CONFIG_HEAP

/*-----------------------------------------------------------------------------
  Parameter section (set the parameters)
*/

/*
Setting:
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
This example contains 1 regions (stm32f103c8t)
- 0: default 20kB ram       (0x20000000..0x20004FFF, shared with stack and bss)
  #define HEAP_NUM          1
  #define HEAP_REGIONS      {(uint8_t *) &ucHeap0, sizeof(ucHeap0)};
  #define RTOSREGION        0  // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 0
  #define MALLOC_REGION     0  // malloc, calloc, realloc, heapsize : region 0
  #define MALLOC_DMAREGION  0  // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 0
This example contains 2 regions (stm32f407vet or stm32f407zet board)
- 0: default 128kB ram      (0x20000000..0x2001FFFF, shared with stack and bss)
- 1: ccm internal 64kB ram  (0x10000000..0x1000FFFF)
  #define HEAP_NUM          2
  #define HEAP_REGIONS      {{ (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                             { (uint8_t *) 0x18000000, 0x10000       }};
  #define RTOSREGION        0      // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 0
  #define MALLOC_REGION     1,  0  // malloc, calloc, realloc, heapsize : region 1 + region 0
  #define MALLOC_DMAREGION  0, -1  // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 0
This example contains 3 regions (stm32f407zet board with external 1MB sram chip with FSMC)
- 0: default 128kB ram      (0x20000000..0x2001FFFF, shared with stack and bss)
- 1: ccm internal 64kB ram  (0x10000000..0x1000FFFF)
- 2: external 1MB sram      (0x68000000..0x680FFFFF)
  #define HEAP_NUM          3
  #define HEAP_REGIONS      {{ (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                             { (uint8_t *) 0x10000000, 0x10000       },\
                             { (uint8_t *) 0x68000000, 0x100000      }};
  #define RTOSREGION        0          // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 0
  #define MALLOC_REGION     2,  1,  0  // malloc, calloc, realloc, heapsize : region 2 + region 1 + region 0
  #define MALLOC_DMAREGION  0, -1, -1  // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 0
  #define MALLOC_INTREGION  1,  0, -1  // multi_malloc_fast, multi_calloc_fast, multi_realloc_fast, heapsize_int : region 1 + region 0
  #define MALLOC_EXTREGION  2, -1, -1  // multi_malloc_ext, multi_calloc_ext, multi_realloc_ext, heapsize_ext : region 2
This example contains 3 regions (stm32f429zit discovery with external 8MB sdram chip with FMC)
- 0: default 192kB ram      (0x20000000..0x2002FFFF, shared with stack and bss)
- 1: ccm internal 64kB ram  (0x10000000..0x1000FFFF)
- 2: external 8MB sdram     (0xD0000000..0xD07FFFFF)
  #define HEAP_NUM          3
  #define HEAP_REGIONS      {{ (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                             { (uint8_t *) 0x10000000, 0x10000       },\
                             { (uint8_t *) 0xD0000000, 0x800000      }};
  #define RTOSREGION        0          // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 0
  #define MALLOC_REGION     2,  1,  0  // malloc, calloc, realloc, heapsize : region 2 + region 1 + region 0
  #define MALLOC_DMAREGION  0, -1, -1  // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 0
  #define MALLOC_INTREGION  1,  0, -1  // multi_malloc_fast, multi_calloc_fast, multi_realloc_fast, heapsize_int : region 1 + region 0
  #define MALLOC_EXTREGION  2, -1, -1  // multi_malloc_ext, multi_calloc_ext, multi_realloc_ext, heapsize_ext : region 2
This example contains 5 regions (stm32h743vit board)
- 0: ITC internal 64kB      (0x00000000..0x0000FFFF)
- 1: DTC internal 128kB     (0x20000000..0x2001FFFF)
- 2: D1 internal 512kB ram  (0x24000000..0x2407FFFF, shared with stack and bss)
- 3: D2 internal 288kB ram  (0x30000000..0x30047FFF)
- 4: D3 internal 64kB ram   (0x38000000..0x3800FFFF)
  #define HEAP_NUM          5
  #define HEAP_REGIONS      {{ (uint8_t *) 0x00000000, 0x10000       },\
                             { (uint8_t *) 0x20000000, 0x20000       },\
                             { (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                             { (uint8_t *) 0x30000000, 0x48000       },\
                             { (uint8_t *) 0x38000000, 0x10000       }};
  #define RTOSREGION        2                  // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 2
  #define MALLOC_REGION     0,  1,  4,  3,  2  // malloc, calloc, realloc, heapsize : region 0 + region 1 + region 4 + region 3 + region 2
  #define MALLOC_DMAREGION  4,  3,  2, -1, -1  // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 4 + region 3
+ region 2 This example contains 6 regions (stm32h743zit for external 8MB sdram chip with FMC)
- 0: ITC internal 64kB      (0x00000000..0x0000FFFF)
- 1: DTC internal 128kB     (0x20000000..0x2001FFFF)
- 2: D1 internal 512kB ram  (0x24000000..0x2407FFFF, shared with stack and bss)
- 3: D2 internal 288kB ram  (0x30000000..0x30047FFF)
- 4: D3 internal 64kB ram   (0x38000000..0x3800FFFF)
- 5: external 8MB sdram     (0xD0000000..0xD07FFFFF)
  #define HEAP_NUM          6
  #define HEAP_REGIONS      {{ (uint8_t *) 0x00000000, 0x10000       },\
                             { (uint8_t *) 0x20000000, 0x20000       },\
                             { (uint8_t *) &ucHeap0, sizeof(ucHeap0) },\
                             { (uint8_t *) 0x30000000, 0x48000       },\
                             { (uint8_t *) 0x38000000, 0x10000       },\
                             { (uint8_t *) 0xD0000000, 0x800000      }};
  #define RTOSREGION        2                      // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 2
  #define MALLOC_REGION     0,  1,  4,  3,  2,  5  // malloc, calloc, realloc, heapsize : region 0 + region 1 + region 4 + region 3 + region
2 + region 5 #define MALLOC_DMAREGION  4,  3,  2, -1, -1, -1  // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma :
region 4 + region 3 + region 2 #define MALLOC_INTREGION  0,  1,  4,  3,  2, -1  // multi_malloc_fast, multi_calloc_fast, multi_realloc_fast,
heapsize_int : region 0 + region 1 + region 4 + region 3 + region 2 #define MALLOC_EXTREGION  5, -1, -1, -1, -1, -1  // multi_malloc_ext,
multi_calloc_ext, multi_realloc_ext, heapsize_ext : region 5
*/

/* Heap region number (1..8) */

#define HEAP_NUM 3
#define CCM_OFFSET 1024
#define HEAP_REGIONS {{(uint8_t *)&ucHeap0, sizeof(ucHeap0)}, {(uint8_t *)0x10000000 + CCM_OFFSET, 0x10000 - CCM_OFFSET}, {(uint8_t *)0x68000000, 0x100000}};
//#define RTOSREGION 0               // pvPortMalloc, vPortFree, xPortGetFreeHeapSize : region 0
#define MALLOC_REGION 0, 1, 2      // malloc, calloc, realloc, heapsize : region 2 + region 1 + region 0
#define MALLOC_DMAREGION 0, -1, -1 // multi_malloc_dma, multi_calloc_dma, multi_realloc_dma, heapsize_dma : region 0
#define MALLOC_INTREGION 1, 0, -1  // multi_malloc_fast, multi_calloc_fast, multi_realloc_fast, heapsize_int : region 1 + region 0
#define MALLOC_EXTREGION 2, -1, -1 // multi_malloc_ext, multi_calloc_ext, multi_realloc_ext, heapsize_ext : region 2


#endif