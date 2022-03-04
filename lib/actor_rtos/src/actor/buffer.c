#include <actor/buffer.h>

static actor_signal_t actor_buffer_reference(actor_buffer_t *buffer) {
    buffer->users++;
    return 0;
}

static actor_signal_t actor_buffer_dereference(actor_buffer_t *buffer) {
    actor_assert(buffer->users > 0);
    buffer->users--;
    return 0;
}

actor_buffer_t *actor_buffer_create_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    debug_printf("│ │ ├ Allocate\t\t%s takes buffer of size %lu\n", actor_stringify(owner), size);
    actor_buffer_t *buffer = actor_buffer_alloc(owner);
    if (buffer != NULL) {
        *buffer = (actor_buffer_t){.owner = owner, .options = options};

        if (options & ACTOR_BUFFER_CHUNKED) {
            buffer->next = buffer;
        }
        if (size > 0) {
            if (data != NULL) {
                // use given chunk of memory directly in a buffer
                // buffer will marked as unmanaged, so the memory will not be freed automatically
                buffer->options |= ACTOR_BUFFER_UNMANAGED;
                actor_buffer_set_data(buffer, data, size);
            } else {
                // allocate known amount of memory
                if (actor_buffer_set_size(buffer, size)) {
                    actor_buffer_release(buffer, owner);
                }
            }
        }
    }
    return buffer;
}

actor_buffer_t *actor_buffer_target_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    actor_buffer_t *buffer;
    if (size == ACTOR_BUFFER_DYNAMIC_SIZE) {
        // use given buffer as front
        buffer = (actor_buffer_t *)data;
        actor_buffer_reference(buffer);
    } else {
        buffer = actor_buffer_create_with_options(owner, data, size, options);
    }
    return buffer;
}

actor_buffer_t *actor_buffer_source_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    actor_assert(data);
    actor_assert(size > 0);
    actor_buffer_t *buffer = actor_buffer_target_with_options(owner, data, size, options);
    buffer->size = buffer->allocated_size;
    return buffer;
}

actor_buffer_t *actor_buffer_snapshot_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    actor_buffer_t *buffer = actor_buffer_target_with_options(owner, NULL, 0, options);
    actor_buffer_append(buffer, data, size);
    return buffer;
}

actor_buffer_t *actor_buffer_aligned_with_options(actor_t *owner, uint32_t size, uint8_t options, uint8_t alignment) {
    // over-allocate, then trim
    actor_buffer_t *buffer = actor_buffer_empty_with_options(owner, size + alignment - 1, options | ACTOR_BUFFER_ALIGNED); // overallocate
    if (buffer != NULL && alignment > 0) {
        actor_buffer_align(buffer, alignment);
    }
    return buffer;
}

// chunked buffers are linked into circular loop, so last page points to first
actor_buffer_t *actor_buffer_get_next_page(actor_buffer_t *buffer, actor_buffer_t *first) {
    if (buffer->next && buffer->next != first) {
        return buffer->next;
    } else {
        return NULL;
    }
}
actor_buffer_t *actor_buffer_get_last_page(actor_buffer_t *buffer) {
    actor_buffer_t *last = buffer;
    while (last->next && last->next != buffer) {
        last = last->next;
    }
    return last;
}

void actor_buffer_release(actor_buffer_t *buffer, actor_t *actor) {
    if (buffer != NULL) {
        if (buffer->owner != actor) {
            actor_buffer_dereference(buffer);
        }
        if (buffer->users == 0) {
            actor_buffer_destroy(buffer);
        }
    }
}

void actor_buffer_use(actor_buffer_t *buffer, actor_t *actor) {
    if (buffer->owner != actor) {
        actor_buffer_reference(buffer);
    }
}

actor_signal_t actor_buffer_reserve(actor_buffer_t *buffer, uint32_t size) {
    if (size > buffer->allocated_size) {
        // only managed buffers can be reallocated
        actor_assert(buffer->owner);
        actor_signal_t signal = actor_buffer_allocation_callback(buffer, size);
        if (signal == ACTOR_SIGNAL_NOT_IMPLEMENTED) {
            actor_buffer_reserve_with_limits(buffer, size, 0, 0, 0);
        }
    }
    return 0;
}

static void *actor_buffer_realloc(actor_buffer_t *buffer, uint32_t size) {
    if (buffer->options & ACTOR_BUFFER_DMA) {
        return actor_realloc_dma(buffer->data, size);
    } else if (buffer->options & ACTOR_BUFFER_EXT) {
        return actor_realloc_ext(buffer->data, size);
    } else {
        return actor_realloc(buffer->data, size);
    }
}

actor_signal_t actor_buffer_set_size(actor_buffer_t *buffer, uint32_t size) {
    buffer->allocated_size = size;
    buffer->data = actor_buffer_realloc(buffer, size);
    if (buffer->data == NULL) {
        return ACTOR_SIGNAL_OUT_OF_MEMORY;
    } else {
        return 0;
    }
}

actor_signal_t actor_buffer_reserve_with_limits(actor_buffer_t *buffer, uint32_t size, uint32_t initial_size, uint32_t block_size,
                                            uint32_t max_size) {
    uint32_t new_size = buffer->allocated_size ? buffer->allocated_size : initial_size ? initial_size : ACTOR_BUFFER_INITIAL_SIZE;
    if (max_size == 0)
        max_size = ACTOR_BUFFER_MAX_SIZE;

    while (new_size < size) {
        new_size += block_size ? block_size : new_size;
    }

    if (new_size > max_size) {
        if (max_size >= size) {
            new_size = max_size;
        } else {
            return ACTOR_SIGNAL_OUT_OF_MEMORY;
        }
    }
    return actor_buffer_set_size(buffer, size);
}

actor_signal_t actor_buffer_read_from_ring_buffer(actor_buffer_t *buffer, uint8_t *ring_buffer, uint16_t ring_buffer_size,
                                              uint16_t previous_position, uint16_t position) {
    //  Handle wraparound of circular buffer, copy memory into actor_buffer that can grow automatically
    if (position > previous_position) {
        return actor_buffer_append(buffer, &ring_buffer[previous_position], position - previous_position);
    } else {
        actor_signal_t signal = actor_buffer_append(buffer, &ring_buffer[previous_position], ring_buffer_size - previous_position);
        if (signal == 0 && position > 0) {
            signal = actor_buffer_append(buffer, &ring_buffer[0], position);
        }
        return signal;
    }
}

actor_signal_t actor_buffer_write(actor_buffer_t *buffer, uint32_t position, uint8_t *data, uint32_t size) {
    // size -1 signals that data points to another actor_buffer_t
    if (data != NULL && size == ACTOR_BUFFER_DYNAMIC_SIZE) {
        actor_buffer_t *other_buffer = (actor_buffer_t *)data;
        data = other_buffer->data;
        size = other_buffer->size;
    }
    actor_signal_t signal = actor_buffer_reserve(buffer, position + size);
    if (signal) {
        return signal;
    }
    if (data != NULL) {
        buffer->size += size;
        memcpy(&buffer->data[position], data, size);
    }
    return 0;
}

actor_signal_t actor_buffer_append(actor_buffer_t *buffer, uint8_t *data, uint32_t size) {
    return actor_buffer_write(buffer, buffer->size, data, size);
}

actor_signal_t actor_buffer_trim_left(actor_buffer_t *buffer, uint8_t offset) {
    buffer->data += offset;
    if (buffer->size)
        buffer->size -= offset;
    buffer->offset_from_allocation += offset;
    return 0;
}

actor_signal_t actor_buffer_trim_right(actor_buffer_t *buffer, uint8_t offset) {
    buffer->size -= offset;
    return 0;
}

actor_signal_t actor_buffer_set_data(actor_buffer_t *buffer, uint8_t *data, uint32_t size) {
    actor_assert(size);
    actor_assert(data);
    buffer->data = data;
    buffer->allocated_size = size;
    return 0;
}

actor_buffer_t *actor_double_buffer_target(actor_buffer_t *back, actor_t *owner, uint8_t *data, uint32_t size) {
    actor_buffer_t *front = actor_buffer_target(owner, data, size);
    // only use double buffer for input of unknown length
    if (size == 0) {
        actor_buffer_reference(back);
    }
    return front;
}

actor_signal_t actor_double_buffer_dereference(actor_buffer_t *back, actor_buffer_t *front, actor_t *owner) {
    if (front->owner == owner) {
        if (back->users > 0) {
            actor_buffer_dereference(back);
        }
    } else {
        actor_buffer_dereference(front);
    }
    return 0;
}

actor_buffer_t *actor_double_buffer_detach(actor_buffer_t *back, actor_buffer_t **front, actor_t *owner) {
    actor_buffer_t *buffer = *front;
    actor_double_buffer_dereference(back, *front, owner);
    *front = NULL;
    return buffer;
}

actor_buffer_t *actor_double_buffer_get_input(actor_buffer_t *back, actor_buffer_t *front) {
    return back->users > 0 ? back : front;
}

actor_signal_t actor_double_buffer_ingest_external_write(actor_buffer_t *back, actor_buffer_t *front, uint16_t position) {
    if (actor_double_buffer_get_input(back, front) == back) {
        // copy data from back to front buffer, taking wraparound into account
        actor_signal_t signal = actor_buffer_read_from_ring_buffer(front, back->data, back->allocated_size, back->size, position);
        back->size = position;
        return signal;
    } else {
        // assume data is written into the front buffer externally
        front->size = position;
        return 0;
    }
}

void *actor_pool_allocate(actor_buffer_t *pool, uint16_t size) {
    // try to find a memory region that is all zeroes
    for (actor_buffer_t *page = pool; page; page = actor_buffer_get_next_page(page, pool)) {
        for (uint32_t offset = 0; offset < page->allocated_size; offset += size) {
            uint16_t i = 0;
            for (; i < size; i++) {
                if (page->data[offset + i] == 0) {
                    break;
                }
            }
            if (i == size) {
                return &page->data[offset];
            }
        }
    }
    // otherwise add another page of buffers
    if (actor_buffer_append(pool, NULL, sizeof(actor_buffer_t))) {
        return NULL;
    }

    // return first byte of a new page
    return actor_buffer_get_last_page(pool)->data;
}

void actor_pool_free(void *memory, uint16_t size) {
    memset(memory, 0, size);
}

void actor_buffer_destroy(actor_buffer_t *buffer) {
    for (actor_buffer_t *page = buffer; page; page = actor_buffer_get_next_page(page, buffer)) {
        // data is only freed when managed buffer is freed
        if (!(buffer->options & ACTOR_BUFFER_UNMANAGED)) {
            actor_free(page->data - page->offset_from_allocation);
        }
        actor_buffer_free(page);
    }
}

actor_buffer_t *actor_buffer_align(actor_buffer_t *buffer, uint8_t alignment) {
    uint8_t offset = alignment - ((uintXX_t)buffer->data) % alignment; // fixme: 64 vs 32
    if (offset > 0) {
        actor_buffer_trim_left(buffer, offset); // shift pointer to aligned spot
        buffer->allocated_size -= offset;     // hide over-allocated space
    }
    buffer->options |= ACTOR_BUFFER_ALIGNED;
    return buffer;
}

// allocate initial list of reusable buffers
actor_buffer_t *actor_buffer_malloc(actor_t *actor) {
    actor_buffer_t *buffer = actor_malloc(sizeof(actor_buffer_t));
    *buffer = ((actor_buffer_t){.next = buffer, .owner = actor});
    return buffer;
}

void actor_buffer_set_next_page(actor_buffer_t *buffer, actor_buffer_t *next) {
    next->next = buffer->next;
    buffer->next = next;
}

__attribute__((weak)) actor_buffer_t *actor_buffer_alloc(actor_t *actor) {
    return actor_malloc(sizeof(actor_buffer_t));
}
__attribute__((weak)) void actor_buffer_free(actor_buffer_t *buffer) {
    actor_free(buffer);
}

__attribute__((weak)) actor_signal_t actor_buffer_allocation_callback(actor_buffer_t *buffer, uint32_t size) {
    return ACTOR_SIGNAL_NOT_IMPLEMENTED;
}