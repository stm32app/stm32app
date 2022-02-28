#ifndef INC_CORE_BUFFER
#define INC_CORE_BUFFER

#include <multi_heap.h>

#include <app_event.h>
#include <app_types.h>
#include <app_env.h>

#ifndef APP_BUFFER_INITIAL_SIZE
#define APP_BUFFER_INITIAL_SIZE 32
#endif

#ifndef APP_BUFFER_MAX_SIZE
#define APP_BUFFER_MAX_SIZE 1024 * 16
#endif

#define APP_BUFFER_DYNAMIC_SIZE ((uint32_t)-1)

typedef enum app_buffer_flag {
    // Buffer memory is owned by some other party and it shouldnt be freed
    APP_BUFFER_UNMANAGED = 1 << 0,
    // Buffer requires strict memory alignment (through overallocation)
    APP_BUFFER_ALIGNED = 1 << 1,
    // Buffer target memory needs to have DMA access
    APP_BUFFER_DMA = 1 << 2,
    // Buffer can be offloaded to slower memory without DMA access
    APP_BUFFER_EXT = 1 << 3,
    // Buffer links to other buffers to grow instead of reallocation
    APP_BUFFER_CHUNKED = 1 << 4,
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
    uint8_t options;
};

/*          R/W?        Copies data?      Can use buffers    Memory-aligned?
Acquired    W           N                 N                  N
Allocated   W           N/A               N/A                N
Target      W           N                 Y                  N
Aligned     W           N                 N                  Y
Source      R           N                 Y                  N
Snapshot    R           Y                 N                  N
*/

// Get growable buffer of specified `size` *to write to*, optionally using given `data` pointer as initial allocation, accepts `options`
app_buffer_t *app_buffer_create_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Get an empty buffer of given `size` *to write to*, either pointing to given `data` or freshly allocated
#define app_buffer_create(owner, data, size) app_buffer_create_with_options(owner, data, size, 0)
// Get a buffer wrapper without allocating data
#define app_buffer_wrapper(owner) app_buffer_create_with_options(owner, NULL, 0, 0)

// Get growable buffer *to write to* with freshly allocated memory of specified `size`, accepts `options`
#define app_buffer_empty_with_options(owner, size, options) app_buffer_create_with_options(owner, NULL, size, options)
// Get growable buffer *to write to* with freshly allocated memory of specified `size`
#define app_buffer_empty(owner, size) app_buffer_empty_with_options(owner, size, 0)

// Get an empty buffer of given `size` *to write to*, either pointing to given `data` or freshly allocated
// When `size` is `-1`, it assumes data points to another buffer that can be used directly with reference tracking
// Accepts `options`
app_buffer_t *app_buffer_target_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Get an empty buffer of given `size` *to write to*, either pointing to given `data` or freshly allocated
// When `size` is `-1`, it assumes data points to another buffer that can be used directly with reference tracking
#define app_buffer_target(owner, data, size) app_buffer_target_with_options(owner, data, size, 0)
#define app_buffer_target_aligned(owner, data, size, alignment)                                                                            \
    (data == NULL ? app_buffer_aligned(owner, size, 16) : app_buffer_target_with_options(owner, data, size, 0))

// Allocate an empty oversized buffer *to write to*, with shifted data pointer matching byte `alignment`, accepts options
app_buffer_t *app_buffer_aligned_with_options(actor_t *owner, uint32_t size, uint8_t options, uint8_t alignment);
// Allocate an empty oversized buffer *to write to*, with shifted data pointer matching byte `alignment`
#define app_buffer_aligned(owner, size, alignment) app_buffer_aligned_with_options(owner, size, alignment, 0)

// Wrap given memory into a buffer *to read from*, so its interal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer that can be used directly with reference tracking
// Accepts `options`
app_buffer_t *app_buffer_source_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Wrap given memory into a buffer *to read from*, so its interal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer that can be used directly with reference tracking
#define app_buffer_source(owner, data, size) app_buffer_source_with_options(owner, data, size, 0)

// Copy given memory into a new buffer *to read from*, its internal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer, but it will not be referenced directly
// Accepts options
app_buffer_t *app_buffer_snapshot_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Copy given memory into a new buffer *to read from*, its internal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer, but it will not be referenced directly
#define app_buffer_snapshot(owner, data, size) app_buffer_snapshot_with_options(owner, data, size, 0)

void *app_pool_allocate(app_buffer_t *pool, uint16_t size);
void app_pool_free(void *memory, uint16_t size);
app_buffer_t *app_pool_allocate_buffer(app_buffer_t *pool);
void app_buffer_destroy(app_buffer_t *buffer);
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

app_buffer_t *app_buffer_align(app_buffer_t *buffer, uint8_t alignment);

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

app_buffer_t *app_buffer_alloc(actor_t *actor);
void app_buffer_free(app_buffer_t *buffer);

app_signal_t app_buffer_allocation_callback(app_buffer_t *buffer, uint32_t size);

#endif