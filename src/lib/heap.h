#ifndef LIB_HEAP_H
#define LIB_HEAP_H

#include <stddef.h>
#include <stddef.h>
#include "FreeRTOS.h"
void *pvPortRealloc(void *mem, size_t newsize);

#define malloc(size) pvPortMalloc(size)
#define free(pointer) vPortFree(pointer)
#define realloc(memory, size) pvPortRealloc(memory, size)

#endif