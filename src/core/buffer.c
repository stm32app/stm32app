#include "core/buffer.h"
#include "core/actor.h"
#include "lib/debug.h"

#define BUFFER_DEFAULT_SIZE 32
#define BUFFER_UNITS

app_signal_t *app_buffer_allocate(actor_t *owner, uint32_t size) {
  app_buffer_t *buffer = malloc(sizeof(app_buffer_t));
  if (buffer != NULL) {
    *buffer = (app_buffer_t) { .owner = owner, .size = size };
  }
  return buffer;
}

void app_buffer_free(app_buffer_t *buffer) {
  free(buffer);
}

app_signal_t app_buffer_free(app_buffer_t *buffer) {
}

app_signal_t app_buffer_claim(app_buffer_t *buffer, actor_t *actor) {
}

app_signal_t app_buffer_reserve(app_buffer_t *buffer, uint32_t size) {
    if (size > buffer->allocated_size) {
        if (buffer->owner->class->on_buffer) {
            buffer->owner->class->on_buffer(actor, buffer, size);
        }
        if (buffer->allocated_size == 0) {
            buffer->allocated_size = BUFFER_DEFAULT_SIZE;
        }
        while (buffer->allocated_size < size) {
            buffer->allocated_size *= 2;
        }
        if (realloc(buffer->data, buffer->allocated_size)) {
          return APP_SIGNAL_OUT_OF_MEMORY;
        }
    }
    return 0;
}

app_signal_t app_buffer_transfer(app_buffer_t *buffer, app_buffer_t *target_buffer) {
  if (target_buffer->allocated_size == 0 && buffer->users == 0) {
    target_buffer->data = buffer->data;
    target_buffer->size = buffer->size;
    target_buffer->allocated_size = buffer->allocated_size;
    target_buffer->offset_from_allocation = buffer->offset_from_allocation;
    return 0;
  } else {
    return APP_SIGNAL_FAILURE;
  }
}

app_signal_t app_buffer_read_from_ring_buffer(app_buffer_t *buffer, uint8_t *ring_buffer, uint16_t ring_buffer_size, uint16_t position, uint16_t previous_position) {
    //  Handle wraparound of circular buffer, copy memory into app_buffer that can grow automatically
    if (position > previous_position) {
        return app_buffer_append(buffer, &ring_buffer[previous_position], position - previous_position);
    } else {
        app_signal_t signal = app_buffer_append(buffer, &ring_buffer[previous_position],
                      ring_buffer_size - previous_position);
        if (signal == 0 && position > 0) {
            signal = app_buffer_append(buffer, &ring_buffer[0], position);
        }
        return signal;
    }
}

app_signal_t app_buffer_write(app_buffer_t *buffer, uint32_t position, uint8_t *data, uint32_t size) {
    // size 0 signals that data is actually another buffer
    if (data != NULL && size == 0) {
      app_buffer_t *other_buffer = (app_buffer_t *) data;
      // see if it's possible to claim the buffer
      if (!app_buffer_transfer(other_buffer, buffer)) {
        return 0;
      }
      // otherwise copy buffer data contents
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

app_signal_t app_buffer_add_user(app_buffer_t *buffer) {
    buffer->users++;
}

app_signal_t app_buffer_remove_user(app_buffer_t *buffer) {
    configASSERT(buffer->users > 0);
    buffer->users--;
    if (buffer->users == 0) {
        app_buffer_free(buffer);
    }
}




app_signal_t app_double_buffer_start(app_double_buffer_t *read, uint8_t *data, uint32_t size) {
    if (data) {

    }
    app_buffer_wrap()
    read->output->data = data;
    read->expected_size = size;
    read->ring_buffer_cursor = 0;

    // If read event provided a destination buffer, use that directly
    if (data != NULL) {
        app_buffer_wrap(&read->output, data, size);
    } else {
        // If number of bytes is known upfront, dma can write directly to the pool
        if (size != 0) {
            if (read->output->allocated_size == 0) {
                app_buffer_reserve(&read->output, size);
            }
            read->incoming_buffer = read->output->data;

            // Read op can opt in for ring buffer
        } else if (read->ring_buffer_size) {
            read->ring_buffer = malloc(read->ring_buffer_size);
            read->incoming_buffer = read->ring_buffer;

            // Otherwise
        } else {
            app_buffer_reserve(&read->output, read->output_initial_size == 0 ? 32 : read->output_initial_size);
            read->incoming_buffer = read->output->data;
        }
        if (read->incoming_buffer == NULL) {
            return APP_SIGNAL_OUT_OF_MEMORY;
        }
    }
    return 0;
}

app_signal_t app_double_buffer_stop(app_double_buffer_t *read, uint8_t **data, uint32_t *size) {
    *data = app_buffer_release(&read->output, size);
    return 0;
}

app_signal_t app_double_buffer_from_dma(app_double_buffer_t *read, uint16_t dma_buffer_bytes_left) {
    uint16_t position = app_double_buffer_get_sufficent_buffer_size(read) - dma_buffer_bytes_left;
    if (read->incoming_buffer == read->ring_buffer) {
        app_buffer_read_from_ring_buffer(read->output, read->ring_buffer, read->ring_buffer_size, position, read->ring_buffer_cursor);
        read->ring_buffer_cursor = position;
    } else {
        read->output->size = position;
    }
    return (position == read->expected_size) ? 0 : APP_SIGNAL_INCOMPLETE;
}

uint16_t app_double_buffer_get_sufficent_buffer_size(app_double_buffer_t *read) {
    if (read->incoming_buffer == read->ring_buffer) {
        return read->ring_buffer_size;
    } else {
        return read->output->allocated_size;
    }
}
