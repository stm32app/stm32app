#include "membuf.h"
#include "lib/heap.h"
// https://github.com/liigo/membuf
// by Liigo.

void membuf_init(membuf_t *buf, uint32_t initial_buffer_size) {
		buf->size = 0;
		buf->block_size = 0;
    buf->data = initial_buffer_size > 0 ? (unsigned char *)malloc(initial_buffer_size) : NULL;
    buf->buffer_size = initial_buffer_size;
    buf->uses_local_buffer = 0;
}

void membuf_init_local(membuf_t *buf, void *local_buffer, uint32_t local_buffer_size) {
		buf->size = 0;
		buf->block_size = 0;
    buf->data = (local_buffer && (local_buffer_size > 0)) ? local_buffer : NULL;
    buf->buffer_size = local_buffer_size;
    buf->uses_local_buffer = 1;
}

void membuf_init_move_from(membuf_t *buf, membuf_t *other) {
    if (other->uses_local_buffer) {
        membuf_init(buf, 0);
        if (other->size > 0)
            membuf_append(buf, other->data, other->size);
    } else {
        *buf = *other;
    }
    memset(other, 0, sizeof(membuf_t)); // other is hollowed now
}

void membuf_uninit(membuf_t *buf) {
    if (!buf->uses_local_buffer && buf->data)
        free(buf->data);
    membuf_reset(buf);
}

void membuf_reserve(membuf_t *buf, uint32_t extra_size) {
    if (extra_size > buf->buffer_size - buf->size) {
        uint32_t new_data_size = buf->size + extra_size;
        uint32_t new_buffer_size = buf->buffer_size == 0 ? new_data_size : buf->buffer_size / buf->block_size + buf->block_size;
        // malloc/realloc new buffer
        if (buf->uses_local_buffer) {
            void *local = buf->data;
            buf->data = realloc(NULL, new_buffer_size); // alloc new buffer
            memcpy(buf->data, local, buf->size);        // copy local bufer to new buffer
            buf->uses_local_buffer = 0;
        } else {
            buf->data = realloc(buf->data, new_buffer_size); // realloc new buffer
        }
        buf->buffer_size = new_buffer_size;
    }
}

uint32_t membuf_append(membuf_t *buf, void *data, uint32_t size) {
    configASSERT(data && size > 0);
    membuf_reserve(buf, size);
    memmove(buf->data + buf->size, data, size);
    buf->size += size;
    return (buf->size - size);
}

uint32_t membuf_append_zeros(membuf_t *buf, uint32_t size) {
    membuf_reserve(buf, size);
    memset(buf->data + buf->size, 0, size);
    buf->size += size;
    return (buf->size - size);
}

void membuf_insert(membuf_t *buf, uint32_t offset, void *data, uint32_t size) {
    configASSERT(offset < buf->size);
    membuf_reserve(buf, size);
    memcpy(buf->data + offset + size, buf->data + offset, buf->size - offset);
    memcpy(buf->data + offset, data, size);
    buf->size += size;
}

void membuf_remove(membuf_t *buf, uint32_t offset, uint32_t size) {
    configASSERT(offset < buf->size);
    if (offset + size >= buf->size) {
        buf->size = offset;
    } else {
        memmove(buf->data + offset, buf->data + offset + size, buf->size - offset - size);
        buf->size -= size;
    }
}

void *membuf_release(membuf_t *buf, uint32_t *psize) {
    void *result = buf->data;
    if (psize)
        *psize = buf->size;
    if (buf->uses_local_buffer && buf->size > 0) {
        result = malloc(buf->size);
        memcpy(result, buf->data, buf->size);
    }
    membuf_reset(buf);
    return result;
}

void membuf_reset(membuf_t *buf) {
    buf->data = NULL;
    buf->size = 0;
    buf->buffer_size = 0;
    buf->uses_local_buffer = 0;
}