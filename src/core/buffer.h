#include "core/types.h"

/* Buffer wraps a growable chunk of memory that can be filled with long messages. Buffer is owned by one of the
actors that is responsible for growing and deallocating the memory. Buffers are meant to be passed between actors:
  1) With full ownership transfer: ensuring memory hand-off without overhead of data copy.
  2) By claiming the use of the buffer, making the owner retain the memory until all users will release the buffer.
*/
struct app_buffer {
    actor_t *owner;                 // Actor that currently manages the buffer
    uint8_t *data;                  // Pointer to first byte of a payload
    uint32_t size;                  // Currently filled size of a buffer
    uint32_t allocated_size;        // Size of memory that can be used before needing to reallocate
    uint8_t offset_from_allocation; // size of optional prelude region before payload 
    uint8_t users;                  // Number of actors that currently use the buffer
};

/* An abstraction over buffer that is used in transport modules. It initializes
buffers in an efficient way based on read operation settings (variable vs known size, large or small).*/
struct app_double_buffer {
    uint32_t expected_size; // 0 if message to be receieved has unknown length
    app_buffer_t *input;
    app_buffer_t *output; // pointer to growable buffer structure that will be handed off
    /*app_buffer_t *current_buffer;
    uint8_t *ring_buffer; // ring buffer for unknown-length messages
    uint16_t ring_buffer_size;
    uint16_t ring_buffer_cursor;

    uint8_t *incoming_buffer; // pointer to the current buffer */
};

app_signal_t app_double_buffer_start(app_double_buffer_t *read, uint8_t *data, uint32_t size);
app_signal_t app_double_buffer_from_dma(app_double_buffer_t *read, uint16_t dma_buffer_bytes_left);
uint16_t app_double_buffer_get_sufficent_buffer_size(app_double_buffer_t *read);
app_signal_t app_double_buffer_stop(app_double_buffer_t *read, uint8_t **data, uint32_t *size);
void app_double_buffer_from_ring_buffer(app_double_buffer_t *read, uint16_t position);
#define app_double_buffer_uses_ring_buffer(read) ((read)->incoming_buffer == (read)->ring_buffer)
#define app_double_buffer_get_buffer(read) (read)->incoming_buffer
