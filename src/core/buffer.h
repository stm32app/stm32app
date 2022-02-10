#ifndef INC_CORE_BUFFER
#define INC_CORE_BUFFER

#include "core/types.h"

#define APP_BUFFER_INITIAL_SIZE 32
#define APP_BUFFER_MAX_SIZE 1024 * 16
#define APP_BUFFER_UNITS
#define APP_BUFFER_DYNAMIC_SIZE ((uint32_t)-1)

typedef enum app_buffer_flag {
  APP_BUFFER_UNMANAGED = 1,
  APP_BUFFER_PROGRESS = 2
} app_buffer_flag_t;

/* Buffer wraps a growable chunk of memory that can be filled with long messages. Buffer is owned by one of the
actors that is responsible for growing and deallocating the memory. Buffers are meant to be passed between actors:
  1) With full ownership transfer: ensuring memory hand-off without overhead of data copy.
  2) By claiming the use of the buffer, making the owner retain the memory until all users will release the buffer.
*/
struct app_buffer {
    actor_t *owner;                 // Actor that currently manages the buffer
    uint8_t *data;                  // Pointer to first byte of a payload
    app_buffer_t *next;             // Circular linked list when buffer can be allocated in pages
    uint32_t size;                  // Currently filled size of a buffer
    uint32_t allocated_size;        // Size of memory that can be used before needing to reallocate
    uint8_t offset_from_allocation; // size of optional prelude region before payload
    uint8_t users;                  // Number of actors that currently use the buffer
    uint8_t flags;
};

#define app_buffer_allocate(owner) app_buffer_target(owner, NULL, 0)
app_buffer_t *app_buffer_target(actor_t *owner, uint8_t *data, uint32_t size);
app_buffer_t *app_buffer_source(actor_t *owner, uint8_t *data, uint32_t size);
app_buffer_t *app_buffer_source_copy(actor_t *owner, uint8_t *data, uint32_t size);
app_buffer_t *app_buffer_paginated(actor_t *owner, uint8_t *data, uint32_t size);

app_buffer_t *app_buffer_take_from_pool(actor_t *actor);
void app_buffer_return_to_pool(app_buffer_t *buffer, actor_t *actor);
app_buffer_t *app_buffer_malloc(actor_t *actor);

app_buffer_t *app_buffer_get_next_page(app_buffer_t *buffer, app_buffer_t *first);
app_buffer_t *app_buffer_get_last_page(app_buffer_t *buffer);
void app_buffer_set_next_page(app_buffer_t *buffer, app_buffer_t *next);

// Copy data from circular buffer to a growable buffer between `previous_position` and `position`, handling wraparound at `ring_buffer_size`
app_signal_t app_buffer_read_from_ring_buffer(app_buffer_t *buffer, uint8_t *ring_buffer, uint16_t ring_buffer_size,
                                              uint16_t previous_position, uint16_t position);

app_signal_t app_buffer_reserve(app_buffer_t *buffer, uint32_t size);
app_signal_t app_buffer_reserve_with_limits(app_buffer_t *buffer, uint32_t size, uint32_t initial_size, uint32_t block_size,
                                            uint32_t max_size);

app_signal_t app_buffer_set_size(app_buffer_t *buffer, uint32_t size);

app_signal_t app_buffer_set_data(app_buffer_t *buffer, uint8_t *data, uint32_t size);

app_buffer_t *app_double_buffer_get_input(app_buffer_t *back, app_buffer_t *front);

void app_buffer_release(app_buffer_t *buffer, actor_t *actor);
void app_buffer_use(app_buffer_t *buffer, actor_t *actor);

app_signal_t app_buffer_write(app_buffer_t *buffer, uint32_t position, uint8_t *data, uint32_t size);
app_signal_t app_buffer_append(app_buffer_t *buffer, uint8_t *data, uint32_t size);

#define app_double_buffer_get_input_size(back, front) app_double_buffer_get_input(back, front)->allocated_size

app_signal_t app_buffer_trim_left(app_buffer_t *buffer, uint8_t offset);
app_signal_t app_buffer_trim_right(app_buffer_t *buffer, uint8_t offset);

/*
  Double buffer is used when receiving data of unknown length externally (e.g. via DMA). It is a combination of circular and growable
  buffer. Back buffer is filled continuously wrapping around, every time the end of allocated memory is reached. Data is being copied from
  circular buffer to growable front buffer periodically (e.g. on half/complete transfer interrupts). Front buffer will grow and reallocate
  on demand. Using two buffers incurs overhead for memory copy, so the following helpers will only use it when necessary.
*/

/* Initialize buffers for reception of data of unknown size.
  - If destination memory chunk is provided, it will be used by front buffer but it will NOT ever be freed by it
  - Both buffers will be used if `expected_size` is set to zero which indicates unknown size.
  - Expected size of `-1` indicates that `destination` points to a `app_buffer_t` that can be used as `front`
*/
app_buffer_t *app_double_buffer_target(app_buffer_t *back, actor_t *owner, uint8_t *data, uint32_t size);
// Release the locks to front and back buffers to make double buffer ready for next transaction
app_signal_t app_double_buffer_dereference(app_buffer_t *back, app_buffer_t *front, actor_t *owner);
// Release the locks, clear pointer to front buffer and return it
app_buffer_t *app_double_buffer_detach(app_buffer_t *back, app_buffer_t **front, actor_t *owner);

app_signal_t app_double_buffer_ingest_external_write(app_buffer_t *back, app_buffer_t *front, uint16_t position);

uint16_t app_double_buffer_get_sufficent_buffer_size(app_double_buffer_t *read);
app_signal_t app_double_buffer_stop(app_buffer_t *back, app_buffer_t *front, uint8_t **data, uint32_t *size);
void app_double_buffer_from_ring_buffer(app_double_buffer_t *read, uint16_t position);
#define app_double_buffer_targets_ring_buffer(front, back) app_double_buffer_get_output(front, back) == front
#define app_double_buffer_get_buffer(read) (read)->incoming_buffer

#endif