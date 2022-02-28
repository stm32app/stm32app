#include <app_buffer.h>
#include <app_rtos_macros.h>

static app_signal_t app_buffer_reference(app_buffer_t *buffer) {
    buffer->users++;
    return 0;
}

static app_signal_t app_buffer_dereference(app_buffer_t *buffer) {
    APP_ASSERT(buffer->users > 0);
    buffer->users--;
    return 0;
}

app_buffer_t *app_buffer_create_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    debug_printf("│ │ ├ Allocate\t\t%s takes buffer of size %lu\n", app_actor_stringify(owner), size);
    app_buffer_t *buffer = app_buffer_alloc(owner);
    if (buffer != NULL) {
        *buffer = (app_buffer_t){.owner = owner, .options = options};

        if (options & APP_BUFFER_CHUNKED) {
            buffer->next = buffer;
        }
        if (size > 0) {
            if (data != NULL) {
                // use given chunk of memory directly in a buffer
                // buffer will marked as unmanaged, so the memory will not be freed automatically
                buffer->options |= APP_BUFFER_UNMANAGED;
                app_buffer_set_data(buffer, data, size);
            } else {
                // allocate known amount of memory
                if (app_buffer_set_size(buffer, size)) {
                    app_buffer_release(buffer, owner);
                }
            }
        }
    }
    return buffer;
}

app_buffer_t *app_buffer_target_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    app_buffer_t *buffer;
    if (size == APP_BUFFER_DYNAMIC_SIZE) {
        // use given buffer as front
        buffer = (app_buffer_t *)data;
        app_buffer_reference(buffer);
    } else {
        buffer = app_buffer_create_with_options(owner, data, size, options);
    }
    return buffer;
}

app_buffer_t *app_buffer_source_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    APP_ASSERT(data);
    APP_ASSERT(size > 0);
    app_buffer_t *buffer = app_buffer_target_with_options(owner, data, size, options);
    buffer->size = buffer->allocated_size;
    return buffer;
}

app_buffer_t *app_buffer_snapshot_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options) {
    app_buffer_t *buffer = app_buffer_target_with_options(owner, NULL, 0, options);
    app_buffer_append(buffer, data, size);
    return buffer;
}

app_buffer_t *app_buffer_aligned_with_options(actor_t *owner, uint32_t size, uint8_t options, uint8_t alignment) {
    // over-allocate, then trim
    app_buffer_t *buffer = app_buffer_empty_with_options(owner, size + alignment - 1, options | APP_BUFFER_ALIGNED); // overallocate
    if (buffer != NULL && alignment > 0) {
        app_buffer_align(buffer, alignment);
    }
    return buffer;
}

// chunked buffers are linked into circular loop, so last page points to first
app_buffer_t *app_buffer_get_next_page(app_buffer_t *buffer, app_buffer_t *first) {
    if (buffer->next && buffer->next != first) {
        return buffer->next;
    } else {
        return NULL;
    }
}
app_buffer_t *app_buffer_get_last_page(app_buffer_t *buffer) {
    app_buffer_t *last = buffer;
    while (last->next && last->next != buffer) {
        last = last->next;
    }
    return last;
}

void app_buffer_release(app_buffer_t *buffer, actor_t *actor) {
    if (buffer != NULL) {
        if (buffer->owner != actor) {
            app_buffer_dereference(buffer);
        }
        if (buffer->users == 0) {
            app_buffer_destroy(buffer);
        }
    }
}

void app_buffer_use(app_buffer_t *buffer, actor_t *actor) {
    if (buffer->owner != actor) {
        app_buffer_reference(buffer);
    }
}

app_signal_t app_buffer_reserve(app_buffer_t *buffer, uint32_t size) {
    if (size > buffer->allocated_size) {
        // only managed buffers can be reallocated
        APP_ASSERT(buffer->owner);
        app_signal_t signal = app_buffer_allocation_callback(buffer, size);
        if (signal == APP_SIGNAL_NOT_IMPLEMENTED) {
            app_buffer_reserve_with_limits(buffer, size, 0, 0, 0);
        }
    }
    return 0;
}

static void *app_buffer_realloc(app_buffer_t *buffer, uint32_t size) {
    if (buffer->options & APP_BUFFER_DMA) {
        return APP_REALLOC_DMA(buffer->data, size);
    } else if (buffer->options & APP_BUFFER_EXT) {
        return APP_REALLOC_EXT(buffer->data, size);
    } else {
        return APP_REALLOC(buffer->data, size);
    }
}

app_signal_t app_buffer_set_size(app_buffer_t *buffer, uint32_t size) {
    buffer->allocated_size = size;
    buffer->data = app_buffer_realloc(buffer, size);
    if (buffer->data == NULL) {
        return APP_SIGNAL_OUT_OF_MEMORY;
    } else {
        return 0;
    }
}

app_signal_t app_buffer_reserve_with_limits(app_buffer_t *buffer, uint32_t size, uint32_t initial_size, uint32_t block_size,
                                            uint32_t max_size) {
    uint32_t new_size = buffer->allocated_size ? buffer->allocated_size : initial_size ? initial_size : APP_BUFFER_INITIAL_SIZE;
    if (max_size == 0)
        max_size = APP_BUFFER_MAX_SIZE;

    while (new_size < size) {
        new_size += block_size ? block_size : new_size;
    }

    if (new_size > max_size) {
        if (max_size >= size) {
            new_size = max_size;
        } else {
            return APP_SIGNAL_OUT_OF_MEMORY;
        }
    }
    return app_buffer_set_size(buffer, size);
}

app_signal_t app_buffer_read_from_ring_buffer(app_buffer_t *buffer, uint8_t *ring_buffer, uint16_t ring_buffer_size,
                                              uint16_t previous_position, uint16_t position) {
    //  Handle wraparound of circular buffer, copy memory into app_buffer that can grow automatically
    if (position > previous_position) {
        return app_buffer_append(buffer, &ring_buffer[previous_position], position - previous_position);
    } else {
        app_signal_t signal = app_buffer_append(buffer, &ring_buffer[previous_position], ring_buffer_size - previous_position);
        if (signal == 0 && position > 0) {
            signal = app_buffer_append(buffer, &ring_buffer[0], position);
        }
        return signal;
    }
}

app_signal_t app_buffer_write(app_buffer_t *buffer, uint32_t position, uint8_t *data, uint32_t size) {
    // size -1 signals that data points to another app_buffer_t
    if (data != NULL && size == APP_BUFFER_DYNAMIC_SIZE) {
        app_buffer_t *other_buffer = (app_buffer_t *)data;
        data = other_buffer->data;
        size = other_buffer->size;
    }
    app_signal_t signal = app_buffer_reserve(buffer, position + size);
    if (signal) {
        return signal;
    }
    if (data != NULL) {
        buffer->size += size;
        memcpy(&buffer->data[position], data, size);
    }
    return 0;
}

app_signal_t app_buffer_append(app_buffer_t *buffer, uint8_t *data, uint32_t size) {
    return app_buffer_write(buffer, buffer->size, data, size);
}

app_signal_t app_buffer_trim_left(app_buffer_t *buffer, uint8_t offset) {
    buffer->data += offset;
    if (buffer->size)
        buffer->size -= offset;
    buffer->offset_from_allocation += offset;
    return 0;
}

app_signal_t app_buffer_trim_right(app_buffer_t *buffer, uint8_t offset) {
    buffer->size -= offset;
    return 0;
}

app_signal_t app_buffer_set_data(app_buffer_t *buffer, uint8_t *data, uint32_t size) {
    APP_ASSERT(size);
    APP_ASSERT(data);
    buffer->data = data;
    buffer->allocated_size = size;
    return 0;
}

app_buffer_t *app_double_buffer_target(app_buffer_t *back, actor_t *owner, uint8_t *data, uint32_t size) {
    app_buffer_t *front = app_buffer_target(owner, data, size);
    // only use double buffer for input of unknown length
    if (size == 0) {
        app_buffer_reference(back);
    }
    return front;
}

app_signal_t app_double_buffer_dereference(app_buffer_t *back, app_buffer_t *front, actor_t *owner) {
    if (front->owner == owner) {
        if (back->users > 0) {
            app_buffer_dereference(back);
        }
    } else {
        app_buffer_dereference(front);
    }
    return 0;
}

app_buffer_t *app_double_buffer_detach(app_buffer_t *back, app_buffer_t **front, actor_t *owner) {
    app_buffer_t *buffer = *front;
    app_double_buffer_dereference(back, *front, owner);
    *front = NULL;
    return buffer;
}

app_buffer_t *app_double_buffer_get_input(app_buffer_t *back, app_buffer_t *front) {
    return back->users > 0 ? back : front;
}

app_signal_t app_double_buffer_ingest_external_write(app_buffer_t *back, app_buffer_t *front, uint16_t position) {
    if (app_double_buffer_get_input(back, front) == back) {
        // copy data from back to front buffer, taking wraparound into account
        app_signal_t signal = app_buffer_read_from_ring_buffer(front, back->data, back->allocated_size, back->size, position);
        back->size = position;
        return signal;
    } else {
        // assume data is written into the front buffer externally
        front->size = position;
        return 0;
    }
}

void *app_pool_allocate(app_buffer_t *pool, uint16_t size) {
    // try to find a memory region that is all zeroes
    for (app_buffer_t *page = pool; page; page = app_buffer_get_next_page(page, pool)) {
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
    if (app_buffer_append(pool, NULL, sizeof(app_buffer_t))) {
        return NULL;
    }

    // return first byte of a new page
    return app_buffer_get_last_page(pool)->data;
}

void app_pool_free(void *memory, uint16_t size) {
    memset(memory, 0, size);
}

void app_buffer_destroy(app_buffer_t *buffer) {
    for (app_buffer_t *page = buffer; page; page = app_buffer_get_next_page(page, buffer)) {
        // data is only freed when managed buffer is freed
        if (!(buffer->options & APP_BUFFER_UNMANAGED)) {
            APP_FREE(page->data - page->offset_from_allocation);
        }
        app_buffer_free(page);
    }
}

app_buffer_t *app_buffer_align(app_buffer_t *buffer, uint8_t alignment) {
    uint8_t offset = alignment - ((uintXX_t)buffer->data) % alignment; // fixme: 64 vs 32
    if (offset > 0) {
        app_buffer_trim_left(buffer, offset); // shift pointer to aligned spot
        buffer->allocated_size -= offset;     // hide over-allocated space
    }
    buffer->options |= APP_BUFFER_ALIGNED;
    return buffer;
}

// allocate initial list of reusable buffers
app_buffer_t *app_buffer_malloc(actor_t *actor) {
    app_buffer_t *buffer = APP_MALLOC(sizeof(app_buffer_t));
    *buffer = ((app_buffer_t){.next = buffer, .owner = actor});
    return buffer;
}

void app_buffer_set_next_page(app_buffer_t *buffer, app_buffer_t *next) {
    next->next = buffer->next;
    buffer->next = next;
}

__attribute__((weak)) app_buffer_t *app_buffer_alloc(actor_t *actor) {
    return app_malloc(sizeof(app_buffer_t));
}
__attribute__((weak)) void app_buffer_free(app_buffer_t *buffer) {
    app_free(buffer);
}

__attribute__((weak)) app_signal_t app_buffer_allocation_callback(app_buffer_t *buffer, uint32_t size) {
    return APP_SIGNAL_NOT_IMPLEMENTED;
}