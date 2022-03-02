#ifndef INC_LIB_DMA
#define INC_LIB_DMA

/* Generalize dma api for STMF1 that dont support DMA streams with STMF2+ that do.
The former need to configure stream to be the same as channel*/

#include <actor/env.h>
#include <actor/actor.h>
#include <actor/types.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rcc.h>

#define DMA_UNITS 2
#define DMA_CHANNELS 8
#define DMA_INDEX(unit, index) ((unit - 1) * (DMA_CHANNELS + 1) + index)
#define DMA_BUFFER_SIZE (DMA_UNITS * (DMA_CHANNELS + 1))
#define DMA_UNREGISTERED_INDEX ((uint8_t)255)

/* Layer of compatibility with F1 */
#ifdef DMA_CHANNEL1
#define dma_enable_stream dma_enable_channel
#define dma_disable_stream dma_disable_channel
#define dma_stream_reset dma_reset_channel
#define dma_channel_select(dma, stream, channel)
#else
#define dma_set_read_from_peripheral(unit, stream) dma_set_transfer_mode(unit, stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM)
#define dma_set_read_from_memory(unit, stream) dma_set_transfer_mode(unit, stream, DMA_SxCR_DIR_MEM_TO_PERIPHERAL)
#endif
uint8_t dma_get_interrupt_for_stream(uint32_t dma, uint8_t index);

/* Define generic constants for psize/msize*/
#ifdef DMA_CCR_PSIZE_8BIT

#define DMA_PSIZE_8BIT DMA_CCR_PSIZE_8BIT
#define DMA_PSIZE_16BIT DMA_CCR_PSIZE_16BIT
#define DMA_PSIZE_32BIT DMA_CCR_PSIZE_32BIT

#define DMA_MSIZE_8BIT DMA_CCR_MSIZE_8BIT
#define DMA_MSIZE_16BIT DMA_CCR_MSIZE_16BIT
#define DMA_MSIZE_32BIT DMA_CCR_MSIZE_32BIT

#define DMA_PL_LOW DMA_CCR_PL_LOW
#define DMA_PL_MEDIUM DMA_CCR_PL_MEDIUM
#define DMA_PL_HIGH DMA_CCR_PL_HIGH
#define DMA_PL_VERY_HIGH DMA_CCR_PL_VERY_HIGH

#else

#define DMA_PSIZE_8BIT DMA_SxCR_PSIZE_8BIT
#define DMA_PSIZE_16BIT DMA_SxCR_PSIZE_16BIT
#define DMA_PSIZE_32BIT DMA_SxCR_PSIZE_32BIT

#define DMA_MSIZE_8BIT DMA_SxCR_MSIZE_8BIT
#define DMA_MSIZE_16BIT DMA_SxCR_MSIZE_16BIT
#define DMA_MSIZE_32BIT DMA_SxCR_MSIZE_32BIT

#define DMA_PL_LOW DMA_SxCR_PL_LOW
#define DMA_PL_MEDIUM DMA_SxCR_PL_MEDIUM
#define DMA_PL_HIGH DMA_SxCR_PL_HIGH
#define DMA_PL_VERY_HIGH DMA_SxCR_PL_VERY_HIGH

#endif

uint32_t dma_get_address(uint8_t index);
uint32_t dma_get_clock_address(uint8_t index);
uint32_t nvic_dma_get_channel_base(uint8_t index);

void actor_dma_tx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *data, size_t size,
                        bool circular_mode, uint8_t width, uint8_t fifo_threshold, bool prefer_burst);
void actor_dma_rx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *buffer, size_t buffer_size,
                        bool circular_mode, uint8_t width, uint8_t fifo_threshold, bool prefer_burst);

void actor_dma_tx_stop(uint8_t unit, uint8_t stream, uint8_t channel);
void actor_dma_rx_stop(uint8_t unit, uint8_t stream, uint8_t channel);

// Figure out which burst size can be used safely for given settings
uint32_t actor_dma_get_safe_burst_size(uint8_t *data, size_t size, uint8_t width, uint8_t fifo_threshold);

/* Make actor be notified through interrupts */
void actor_register_dma(uint8_t unit, uint8_t index, actor_t *actor);
/* Remove interrupt notification assignment */
void actor_unregister_dma(uint8_t unit, uint8_t index);
/* Notify registered actor */
void actors_dma_notify(uint8_t unit, uint8_t index);

uint32_t actor_dma_get_buffer_position(uint8_t unit, uint8_t index, uint32_t buffer_size);

/* Combine DMA unit and index into a pointer */
void *actor_dma_pack_source(uint8_t unit, uint8_t index);

/* Check if pointer contains packed unit/index info */
bool actor_dma_match_source(void *source, uint8_t unit, uint8_t index);
void actor_dma_flush_stream(uint8_t unit, uint8_t stream);

#endif