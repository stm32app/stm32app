#include "core/buffer.h"
#include "core/actor.h"
#include "lib/debug.h"

static app_signal_t app_buffer_reference(app_buffer_t *buffer) {
    buffer->users++;
    return 0;
}

static app_signal_t app_buffer_dereference(app_buffer_t *buffer) {
    configASSERT(buffer->users > 0);
    buffer->users--;
    return 0;
}

app_buffer_t *app_buffer_target(actor_t *owner, uint8_t *data, uint32_t size) {
    app_buffer_t *buffer;
    if (size == APP_BUFFER_DYNAMIC_SIZE) {
        // use given buffer as front
        buffer = (app_buffer_t *)data;
        app_buffer_reference(buffer);
    } else {
        debug_printf("│ │ ├ Allocate\t\t%s takes buffer of size %lu\n", get_actor_type_name(owner->class->type), size);
        buffer = app_buffer_take_from_pool(owner);
        if (buffer != NULL) {
            buffer->owner = owner;
            if (size > 0) {
                if (data != NULL) {
                    // use given chunk of memory directly in a buffer
                    // buffer will marked as unmanaged, so the memory will not be freed automatically
                    buffer->flags |= APP_BUFFER_UNMANAGED;
                    app_buffer_set_data(buffer, data, size);
                } else {
                    // allocate known amount of memory
                    if (app_buffer_set_size(buffer, size)) {
                        app_buffer_release(buffer, owner);
                    }
                }
            }
        }
    }
    return buffer;
}

// creates buffer that has its data aligned to specific number of bytes
// extra care needs to be taken when growing this buffer, the owner has to realign it
app_buffer_t *app_buffer_target_aligned(actor_t *owner, uint32_t size, uint8_t alignment) {
    // over-allocate, then trim
    app_buffer_t *buffer = app_buffer_target(owner, NULL, size + alignment - 1); //overallocate 
    if (buffer != NULL && alignment > 0) {
        app_buffer_align(buffer, alignment);
    }
    return buffer;
}

app_buffer_t *app_buffer_source(actor_t *owner, uint8_t *data, uint32_t size) {
    app_buffer_t *buffer = app_buffer_target(owner, data, size);
    buffer->size = buffer->allocated_size;
    return buffer;
}

app_buffer_t *app_buffer_source_copy(actor_t *owner, uint8_t *data, uint32_t size) {
    app_buffer_t *buffer = app_buffer_target(owner, NULL, 0);
    app_buffer_append(buffer, data, size);
    return buffer;
}
app_buffer_t *app_buffer_paginated(actor_t *owner, uint8_t *data, uint32_t size) {
    app_buffer_t *buffer = app_buffer_target(owner, NULL, 0);
    buffer->next = buffer;
    app_buffer_append(buffer, data, size);
    return buffer;
}

// paginated buffers are linked into circular loop, so meeing first page means last page is reached
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
            app_buffer_return_to_pool(buffer, actor);
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
        configASSERT(buffer->owner);
        if (buffer->owner->class->on_buffer) {
            return buffer->owner->class->on_buffer(buffer->owner->object, buffer, size);
        } else {
            app_buffer_reserve_with_limits(buffer, size, 0, 0, 0);
        }
    }
    return 0;
}

app_signal_t app_buffer_set_size(app_buffer_t *buffer, uint32_t size) {
    buffer->allocated_size = size;
    buffer->data = realloc(buffer->data, size);
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
    configASSERT(size);
    configASSERT(data);
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

// iterate pool of buffers to find
app_buffer_t *app_buffer_take_from_pool(actor_t *actor) {
    app_buffer_t *pool = actor->app->buffers;

    // try to find one of the buffers that arent used
    for (app_buffer_t *page = pool; page; page = app_buffer_get_next_page(page, pool)) {
        for (uint32_t offset = 0; offset < page->allocated_size; offset += sizeof(app_buffer_t)) {
            app_buffer_t *buffer = (app_buffer_t *)&page->data[offset];
            if (buffer->owner == NULL && buffer->data == NULL && buffer->allocated_size == 0) {
                return buffer;
            }
        }
    }

    // otherwise add another page of buffers
    app_buffer_append(pool, NULL, sizeof(app_buffer_t));

    return (app_buffer_t *)app_buffer_get_last_page(pool)->data;
}

void app_buffer_return_to_pool(app_buffer_t *buffer, actor_t *actor) {
    debug_printf("│ │ ├ Release\t\t%s buffer of size %lu/%lu\n", get_actor_type_name(actor->class->type), buffer->size,
                 buffer->allocated_size);
    for (app_buffer_t *page = buffer; page; page = app_buffer_get_next_page(page, buffer)) {
        // data is only freed when managed buffer is freed
        if (!(buffer->flags & APP_BUFFER_UNMANAGED)) {
            free(page->data - page->offset_from_allocation);
        }
        // buffers are already in pool, so they only need to be cleaned up for reuse
        memset(page, 0, sizeof(app_buffer_t));
    }
}


app_buffer_t *app_buffer_align(app_buffer_t *buffer, uint8_t alignment) {
    uint8_t offset = alignment - ((uint32_t) buffer->data) % alignment;
    if (offset > 0) {
        app_buffer_trim_left(buffer, offset); // shift pointer to aligned spot
        buffer->allocated_size -= offset; // hide over-allocated space
    }
    buffer->flags &= APP_BUFFER_ALIGNED;
    return buffer;
}


// allocate list of reusable buffers
app_buffer_t *app_buffer_malloc(actor_t *actor) {
    app_buffer_t *buffer = malloc(sizeof(app_buffer_t));
    *buffer = ((app_buffer_t){.next = buffer, .owner = actor});
    return buffer;
}

void app_buffer_set_next_page(app_buffer_t *buffer, app_buffer_t *next) {
    next->next = buffer->next;
    buffer->next = next;
}