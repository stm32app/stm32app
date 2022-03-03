#ifndef INC_CORE_BUFFER
#define INC_CORE_BUFFER


#include <actor/event.h>
#include <actor/types.h>
#include <actor/env.h>

#ifndef ACTOR_BUFFER_INITIAL_SIZE
#define ACTOR_BUFFER_INITIAL_SIZE 32
#endif

#ifndef ACTOR_BUFFER_MAX_SIZE
#define ACTOR_BUFFER_MAX_SIZE 1024 * 16
#endif

#define ACTOR_BUFFER_DYNAMIC_SIZE ((uint32_t)-1)

typedef enum actor_buffer_flag {
    // Buffer memory is owned by some other party and it shouldnt be freed
    ACTOR_BUFFER_UNMANAGED = 1 << 0,
    // Buffer requires strict memory alignment (through overallocation)
    ACTOR_BUFFER_ALIGNED = 1 << 1,
    // Buffer target memory needs to have DMA access
    ACTOR_BUFFER_DMA = 1 << 2,
    // Buffer can be offloaded to slower memory without DMA access
    ACTOR_BUFFER_EXT = 1 << 3,
    // Buffer links to other buffers to grow instead of reallocation
    ACTOR_BUFFER_CHUNKED = 1 << 4,
} actor_buffer_flag_t;

/* Buffer wraps a growable chunk of memory that can be filled with long messages. Buffer is owned by one of the
actors that is responsible for growing and deallocating the memory. Buffers are meant to be passed between actors:
  1) With full ownership transfer: ensuring memory hand-off without overhead of data copy.
  2) By claiming the use of the buffer, making the owner retain the memory until all users will release the buffer.
*/
struct actor_buffer {
    actor_t *owner;                 // Actor that currently manages the buffer
    uint8_t *data;                  // Pointer to first byte of a payload
    actor_buffer_t *next;             // Circular linked list when buffer can be allocated in pages
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
actor_buffer_t *actor_buffer_create_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Get an empty buffer of given `size` *to write to*, either pointing to given `data` or freshly allocated
#define actor_buffer_create(owner, data, size) actor_buffer_create_with_options(owner, data, size, 0)
// Get a buffer wrapper without allocating data
#define actor_buffer_wrapper(owner) actor_buffer_create_with_options(owner, NULL, 0, 0)

// Get growable buffer *to write to* with freshly allocated memory of specified `size`, accepts `options`
#define actor_buffer_empty_with_options(owner, size, options) actor_buffer_create_with_options(owner, NULL, size, options)
// Get growable buffer *to write to* with freshly allocated memory of specified `size`
#define actor_buffer_empty(owner, size) actor_buffer_empty_with_options(owner, size, 0)

// Get an empty buffer of given `size` *to write to*, either pointing to given `data` or freshly allocated
// When `size` is `-1`, it assumes data points to another buffer that can be used directly with reference tracking
// Accepts `options`
actor_buffer_t *actor_buffer_target_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Get an empty buffer of given `size` *to write to*, either pointing to given `data` or freshly allocated
// When `size` is `-1`, it assumes data points to another buffer that can be used directly with reference tracking
#define actor_buffer_target(owner, data, size) actor_buffer_target_with_options(owner, data, size, 0)
#define actor_buffer_target_aligned(owner, data, size, alignment)                                                                            \
    (data == NULL ? actor_buffer_aligned(owner, size, 16) : actor_buffer_target_with_options(owner, data, size, 0))

// Allocate an empty oversized buffer *to write to*, with shifted data pointer matching byte `alignment`, accepts options
actor_buffer_t *actor_buffer_aligned_with_options(actor_t *owner, uint32_t size, uint8_t options, uint8_t alignment);
// Allocate an empty oversized buffer *to write to*, with shifted data pointer matching byte `alignment`
#define actor_buffer_aligned(owner, size, alignment) actor_buffer_aligned_with_options(owner, size, alignment, 0)

// Wrap given memory into a buffer *to read from*, so its interal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer that can be used directly with reference tracking
// Accepts `options`
actor_buffer_t *actor_buffer_source_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Wrap given memory into a buffer *to read from*, so its interal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer that can be used directly with reference tracking
#define actor_buffer_source(owner, data, size) actor_buffer_source_with_options(owner, data, size, 0)

// Copy given memory into a new buffer *to read from*, its internal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer, but it will not be referenced directly
// Accepts options
actor_buffer_t *actor_buffer_snapshot_with_options(actor_t *owner, uint8_t *data, uint32_t size, uint8_t options);
// Copy given memory into a new buffer *to read from*, its internal `size` will equal given `size`
// Size of -1 indicates that `data` points to another buffer, but it will not be referenced directly
#define actor_buffer_snapshot(owner, data, size) actor_buffer_snapshot_with_options(owner, data, size, 0)

void *actor_pool_allocate(actor_buffer_t *pool, uint16_t size);
void actor_pool_free(void *memory, uint16_t size);
actor_buffer_t *actor_pool_allocate_buffer(actor_buffer_t *pool);
void actor_buffer_destroy(actor_buffer_t *buffer);
actor_buffer_t *actor_buffer_malloc(actor_t *actor);

actor_buffer_t *actor_buffer_get_next_page(actor_buffer_t *buffer, actor_buffer_t *first);
actor_buffer_t *actor_buffer_get_last_page(actor_buffer_t *buffer);
void actor_buffer_set_next_page(actor_buffer_t *buffer, actor_buffer_t *next);

// Copy data from circular buffer to a growable buffer between `previous_position` and `position`, handling wraparound at `ring_buffer_size`
actor_signal_t actor_buffer_read_from_ring_buffer(actor_buffer_t *buffer, uint8_t *ring_buffer, uint16_t ring_buffer_size,
                                              uint16_t previous_position, uint16_t position);

actor_signal_t actor_buffer_reserve(actor_buffer_t *buffer, uint32_t size);
actor_signal_t actor_buffer_reserve_with_limits(actor_buffer_t *buffer, uint32_t size, uint32_t initial_size, uint32_t block_size,
                                            uint32_t max_size);

actor_signal_t actor_buffer_set_size(actor_buffer_t *buffer, uint32_t size);

actor_signal_t actor_buffer_set_data(actor_buffer_t *buffer, uint8_t *data, uint32_t size);

actor_buffer_t *actor_double_buffer_get_input(actor_buffer_t *back, actor_buffer_t *front);

void actor_buffer_release(actor_buffer_t *buffer, actor_t *actor);
void actor_buffer_use(actor_buffer_t *buffer, actor_t *actor);

actor_signal_t actor_buffer_write(actor_buffer_t *buffer, uint32_t position, uint8_t *data, uint32_t size);
actor_signal_t actor_buffer_append(actor_buffer_t *buffer, uint8_t *data, uint32_t size);

#define actor_double_buffer_get_input_size(back, front) actor_double_buffer_get_input(back, front)->allocated_size

actor_signal_t actor_buffer_trim_left(actor_buffer_t *buffer, uint8_t offset);
actor_signal_t actor_buffer_trim_right(actor_buffer_t *buffer, uint8_t offset);

actor_buffer_t *actor_buffer_align(actor_buffer_t *buffer, uint8_t alignment);

/*
  Double buffer is used when receiving data of unknown length externally (e.g. via DMA). It is a combination of circular and growable
  buffer. Back buffer is filled continuously wrapping around, every time the end of allocated memory is reached. Data is being copied from
  circular buffer to growable front buffer periodically (e.g. on half/complete transfer interrupts). Front buffer will grow and reallocate
  on demand. Using two buffers incurs overhead for memory copy, so the following helpers will only use it when necessary.
*/

/* Initialize buffers for reception of data of unknown size.
  - If destination memory chunk is provided, it will be used by front buffer but it will NOT ever be freed by it
  - Both buffers will be used if `expected_size` is set to zero which indicates unknown size.
  - Expected size of `-1` indicates that `destination` points to a `actor_buffer_t` that can be used as `front`
*/
actor_buffer_t *actor_double_buffer_target(actor_buffer_t *back, actor_t *owner, uint8_t *data, uint32_t size);
// Release the locks to front and back buffers to make double buffer ready for next transaction
actor_signal_t actor_double_buffer_dereference(actor_buffer_t *back, actor_buffer_t *front, actor_t *owner);
// Release the locks, clear pointer to front buffer and return it
actor_buffer_t *actor_double_buffer_detach(actor_buffer_t *back, actor_buffer_t **front, actor_t *owner);

actor_signal_t actor_double_buffer_ingest_external_write(actor_buffer_t *back, actor_buffer_t *front, uint16_t position);

uint16_t actor_double_buffer_get_sufficent_buffer_size(actor_double_buffer_t *read);
actor_signal_t actor_double_buffer_stop(actor_buffer_t *back, actor_buffer_t *front, uint8_t **data, uint32_t *size);
void actor_double_buffer_from_ring_buffer(actor_double_buffer_t *read, uint16_t position);
#define actor_double_buffer_targets_ring_buffer(front, back) actor_double_buffer_get_output(front, back) == front
#define actor_double_buffer_get_buffer(read) (read)->incoming_buffer

actor_buffer_t *actor_buffer_alloc(actor_t *actor);
void actor_buffer_free(actor_buffer_t *buffer);

actor_signal_t actor_buffer_allocation_callback(actor_buffer_t *buffer, uint32_t size);

#endif