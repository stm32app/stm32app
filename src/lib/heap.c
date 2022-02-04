#include <stddef.h>
#include "lib/heap.h"
#include <string.h>

void *pvPortRealloc(void *mem, size_t newsize)
{
    if (newsize == 0) {
        vPortFree(mem);
        return (void *) 0;
    }

    void *p;
    p = pvPortMalloc(newsize);
    if (p) {
        /* zero the memory */
        if (mem != (void *) 0) {
            memcpy(p, mem, newsize);
            vPortFree(mem);
        }
    }
    return p;
}