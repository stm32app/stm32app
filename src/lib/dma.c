#include "dma.h"
#include "core/buffer.h"
#include "lib/bytes.h"

uint32_t dma_get_address(uint8_t index) {
    switch (index) {
#ifdef DMA2_BASE
    case 2:
        return DMA2;
#endif
    default:
        return DMA1;
    }
}

uint32_t dma_get_clock_address(uint8_t index) {
    switch (index) {
#ifdef DMA2_BASE
    case 2:
        return RCC_DMA2;
#endif
    default:
        return RCC_DMA1;
    }
}

static inline uint32_t dma_get_peripherial_clock(uint8_t index) {
    switch (index) {
#ifdef DMA2_BASE
    case 2:
#if defined(RCC_AHB1ENR_DMA2EN)
        return RCC_AHB1ENR_DMA2EN;
#elif defined(RCC_AHB2ENR_DMA2EN)
        return RCC_AHB2ENR_DMA2EN;
#endif
#endif
    default:
#if defined(RCC_AHB1ENR_DMA1EN)
        return RCC_AHB1ENR_DMA1EN;
#elif defined(RCC_AHB2ENR_DMA1EN)
        return RCC_AHB2ENR_DMA1EN;
#endif
    }
}

static inline volatile uint32_t *dma_get_ahb(uint8_t index) {
    switch (index) {
#ifdef DMA2_BASE
    case 2:
#if defined(RCC_AHB1ENR_DMA2EN)
        return &RCC_AHB1ENR;
#elif defined(RCC_AHB2ENR_DMA2EN)
        return &RCC_AHB2ENR;
#endif
#endif
    default:
#if defined(RCC_AHB1ENR)
        return &RCC_AHB1ENR;
#elif defined(RCC_AHB2ENR_DMA1EN)
        return &RCC_AHB2ENR;
#endif
    }
}

uint32_t nvic_dma_get_channel_base(uint8_t index) {
    switch (index) {
#ifdef DMA2_BASE
    case 2:
#ifdef NVIC_DMA2_CHANNEL1_IRQ
        return NVIC_DMA2_CHANNEL1_IRQ - 1;
#else
        return NVIC_DMA2_STREAM0_IRQ;
#endif
#endif
#ifdef DMA3_BASE
    case 3:
#ifdef NVIC_DMA3_CHANNEL1_IRQ
        return NVIC_DMA3_CHANNEL1_IRQ - 1;
#else
        return NVIC_DMA3_STREAM0_IRQ;
#endif
#endif
    default:
#ifdef NVIC_DMA1_CHANNEL1_IRQ
        return NVIC_DMA1_CHANNEL1_IRQ - 1;
#else
        return NVIC_DMA1_STREAM0_IRQ;
#endif
    }
}

uint8_t dma_get_interrupt_for_stream(uint32_t dma, uint8_t index) {
#ifdef DMA_CHANNEL1
    switch (dma) {
    case 2:
        return NVIC_DMA2_CHANNEL1_IRQ + index - 1;
    default:
        return NVIC_DMA1_CHANNEL1_IRQ + index - 1;
    }
#else
    switch (dma) {
    case 2:
        return NVIC_DMA2_STREAM1_IRQ + index - 1;
    default:
        return NVIC_DMA1_STREAM1_IRQ + index - 1;
    }

#endif
}

volatile actor_t *actors_dma[DMA_BUFFER_SIZE];

void actor_register_dma(uint8_t unit, uint8_t index, actor_t *actor) {
    actors_dma[DMA_INDEX(unit, index)] = actor;
};

void actor_unregister_dma(uint8_t unit, uint8_t index) {
    actors_dma[DMA_INDEX(unit, index)] = NULL;
};

void actors_dma_notify(uint8_t unit, uint8_t index) {
    debug_printf("> DMA%i interrupt:\t\t%u left\n", unit, dma_get_number_of_data(dma_get_address(unit), index));
    volatile actor_t *actor = actors_dma[DMA_INDEX(unit, index)];

    if (dma_get_interrupt_flag(dma_get_address(unit), index, DMA_TEIF | DMA_DMEIF | DMA_FEIF)) {

        error_printf("DMA%i error in channel %i %s%s%s \n", unit, index,
                     dma_get_interrupt_flag(dma_get_address(unit), index, DMA_TEIF) ? " TEIF" : "",
                     dma_get_interrupt_flag(dma_get_address(unit), index, DMA_DMEIF) ? " DMEIF" : "",
                     dma_get_interrupt_flag(dma_get_address(unit), index, DMA_FEIF) ? " FEIF" : "");

        if (actor->class->on_signal(actor->object, NULL, APP_SIGNAL_DMA_ERROR, actor_dma_pack_source(unit, index))) {
            dma_clear_interrupt_flags(dma_get_address(unit), index, DMA_TEIF | DMA_DMEIF  | DMA_FEIF);
        }
    } else if (dma_get_interrupt_flag(dma_get_address(unit), index, DMA_HTIF | DMA_TCIF)) {
        if (actor->class->on_signal(actor->object, NULL, APP_SIGNAL_DMA_TRANSFERRING, actor_dma_pack_source(unit, index)) == 0) {
            dma_clear_interrupt_flags(dma_get_address(unit), index, DMA_HTIF | DMA_TCIF);
        }
    }
    debug_printf("< DMA%i interrupt\n", unit);
}

void *actor_dma_pack_source(uint8_t unit, uint8_t index) {
    return (void *)(uint32_t)((unit << 0) + (index << 8));
}

bool_t actor_dma_match_source(void *source, uint8_t unit, uint8_t index) {
    return unit == (((uint32_t)source) & 0xff) && index == (((uint32_t)source) >> 8 & 0xff);
}

uint32_t actor_dma_get_buffer_position(uint8_t unit, uint8_t index, uint32_t buffer_size) {
    return buffer_size - dma_get_number_of_data(dma_get_address(unit), index);
}
// memory location of a buffer has to be aligned to burst_size * byte_width to ensure single AHB copy does cross 1kb boundary
// buffers allocated with `app_buffer_aligned` are guaranteed to be aligned, while others may be aligned by accident
uint32_t actor_dma_get_safe_burst_size(uint8_t *data, size_t size, uint8_t width, uint8_t fifo_threshold) {
    uint32_t address = (uint32_t)data;
    uint32_t start_page_address = address / 1024;
    uint32_t end_page_address = (address + size * width) / 1024;
    // if data range doesn cross 1024 boundary, byte alignment isnt a concern
    // otherwise see the best alignment of data buffer
    uint8_t alignment = start_page_address == end_page_address ? 16 : get_maximum_byte_alignment((uint32_t)&data, 16);
    // See AN4031, p 13, Table 4: Possible burst configurations
    switch (fifo_threshold) {
    case 4:
        if (width == 1) {
            return 4; // 1 burst of 4 bytes
        }
        break;
    case 3:
        if (width == 1) {
            return 4; // 3 bursts of 4 bytes
        }
        break;
    case 2:
        switch (width) {
        case 1:
            if (alignment % 8 == 0) {
                return 8; // 1 burst of 8 bytes
            } else if (alignment % 4 == 0) {
                return 4; // 2 bursts of 4 bytes
            }
            break;
        case 2:
            if (alignment % 8 == 0) {
                return 4; // 1 burst of 4 half-words
            }
            break;
        }
        break;
    default:
        switch (width) {
        case 1:
            if (alignment % 16 == 0) {
                return 16; // 1 burst of 16 bytes
            } else if (alignment % 8 == 0) {
                return 8; // 2 bursts of 8 bytes
            } else if (alignment % 4 == 0) {
                return 4; // 4 bursts of 4 bytes
            }
            break;
        case 2:
            if (alignment % 16 == 0) {
                return 8; // 1 burst of 8 half-words
            } else if (alignment % 8 == 0) {
                return 4; // 2 bursts of 4 half-words
            }
            break;
        case 4:
            if (alignment % 16 == 0) {
                return 4; // 1 burst of 16 words;
            }
            break;
        }
    }
    return 1; // no bursts
}

void actor_dma_rx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *data, size_t size,
                        bool_t circular_mode, uint8_t width, uint8_t fifo_threshold, bool_t prefer_burst) {
    uint32_t dma_address = dma_get_address(unit);

    rcc_periph_clock_enable(dma_get_clock_address(unit));
    rcc_peripheral_enable_clock(dma_get_ahb(unit), dma_get_peripherial_clock(unit));

    dma_stream_reset(dma_address, stream);
    dma_disable_stream(dma_address, stream);
    dma_channel_select(dma_address, stream, channel << DMA_SxCR_CHSEL_SHIFT);

   // dma_set_peripheral_flow_control(dma_address, stream);
     dma_set_dma_flow_control(dma_address, stream);

    dma_disable_peripheral_increment_mode(dma_address, stream);
    dma_enable_memory_increment_mode(dma_address, stream);

    dma_set_peripheral_address(dma_address, stream, periphery_address);

    dma_set_read_from_peripheral(dma_address, stream);
    if (circular_mode) {
        dma_enable_circular_mode(dma_address, stream);
        dma_enable_half_transfer_interrupt(dma_address, stream);
    }
    switch (width) {
    case 4:
        dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_32BIT);
        dma_set_memory_size(dma_address, stream, DMA_MSIZE_32BIT);
        break;
    case 2:
        dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_16BIT);
        dma_set_memory_size(dma_address, stream, DMA_MSIZE_16BIT);
        break;
    default:
        dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_8BIT);
        dma_set_memory_size(dma_address, stream, DMA_MSIZE_8BIT);
    }
#ifndef STMF1
    if (fifo_threshold) {
        dma_set_fifo_threshold(dma_address, stream, (fifo_threshold - 1) << 0);
        dma_enable_fifo_error_interrupt(dma_address, stream);
        dma_enable_fifo_mode(dma_address, stream);
    } else {
        dma_enable_direct_mode(dma_address, stream);
        dma_enable_direct_mode_error_interrupt(dma_address, stream);
    }
    if (prefer_burst && fifo_threshold) {
        switch (actor_dma_get_safe_burst_size(data, size, width, fifo_threshold)) {
        case 16:
            //dma_set_memory_burst(dma_address, stream, DMA_SxCR_MBURST_INCR16);
            dma_set_peripheral_burst(dma_address, stream, DMA_SxCR_PBURST_INCR16);
            break;
        case 8:
            //dma_set_memory_burst(dma_address, stream, DMA_SxCR_MBURST_INCR8);
            dma_set_peripheral_burst(dma_address, stream, DMA_SxCR_PBURST_INCR8);
            break;
        case 4:
            //dma_set_memory_burst(dma_address, stream, DMA_SxCR_MBURST_INCR4);
            dma_set_peripheral_burst(dma_address, stream, DMA_SxCR_PBURST_INCR4);
            break;
        }
    }
#endif

    dma_set_priority(dma_address, stream, DMA_PL_VERY_HIGH);

    dma_set_memory_address(dma_address, stream, (uint32_t)data);
    dma_set_number_of_data(dma_address, stream, size);


    //dma_enable_half_transfer_interrupt(dma_address, stream);
    dma_enable_transfer_complete_interrupt(dma_address, stream);
    dma_enable_transfer_error_interrupt(dma_address, stream);

    nvic_set_priority(nvic_dma_get_channel_base(unit) + stream, 5 << 4);
    nvic_enable_irq(nvic_dma_get_channel_base(unit) + stream);

    dma_enable_stream(dma_address, stream);
}

void actor_dma_rx_stop(uint8_t unit, uint8_t stream, uint8_t channel) {
    uint32_t dma_address = dma_get_address(unit);
    nvic_disable_irq(nvic_dma_get_channel_base(unit) + stream);
    dma_disable_stream(dma_address, stream);
}

void actor_dma_tx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *data, size_t size,
                        bool_t circular_mode, uint8_t width, uint8_t fifo_threshold, bool_t prefer_burst) {
    uint32_t dma_address = dma_get_address(unit);

    rcc_periph_clock_enable(dma_get_clock_address(unit));
    rcc_peripheral_enable_clock(dma_get_ahb(unit), dma_get_peripherial_clock(unit));

    dma_stream_reset(dma_address, stream);
    dma_channel_select(dma_address, stream, channel << DMA_SxCR_CHSEL_SHIFT);

    dma_set_peripheral_address(dma_address, stream, periphery_address);
    dma_set_memory_address(dma_address, stream, (uint32_t)data);
    dma_set_number_of_data(dma_address, stream, size);
    dma_disable_peripheral_increment_mode(dma_address, stream);

    dma_set_read_from_memory(dma_address, stream);
    dma_enable_memory_increment_mode(dma_address, stream);
    if (circular_mode) {
        dma_enable_circular_mode(dma_address, stream);
    }

    switch (width) {
    case 4:
        dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_32BIT);
        dma_set_memory_size(dma_address, stream, DMA_MSIZE_32BIT);
        break;
    case 2:
        dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_16BIT);
        dma_set_memory_size(dma_address, stream, DMA_MSIZE_16BIT);
        break;
    default:
        dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_8BIT);
        dma_set_memory_size(dma_address, stream, DMA_MSIZE_8BIT);
    }
#ifndef STMF1
    if (fifo_threshold) {
        dma_set_fifo_threshold(dma_address, stream, (fifo_threshold - 1) << 0);
        dma_enable_fifo_error_interrupt(dma_address, stream);
        dma_enable_fifo_mode(dma_address, stream);
    } else {
        dma_enable_direct_mode(dma_address, stream);
        dma_enable_direct_mode_error_interrupt(dma_address, stream);
    }
    if (prefer_burst && fifo_threshold) {
        switch (actor_dma_get_safe_burst_size(data, size, width, fifo_threshold)) {
        case 16:
            dma_set_memory_burst(dma_address, stream, DMA_SxCR_MBURST_INCR16);
            dma_set_peripheral_burst(dma_address, stream, DMA_SxCR_PBURST_INCR16);
            break;
        case 8:
            dma_set_memory_burst(dma_address, stream, DMA_SxCR_MBURST_INCR8);
            dma_set_peripheral_burst(dma_address, stream, DMA_SxCR_PBURST_INCR8);
            break;
        case 4:
            dma_set_memory_burst(dma_address, stream, DMA_SxCR_MBURST_INCR4);
            dma_set_peripheral_burst(dma_address, stream, DMA_SxCR_PBURST_INCR4);
            break;
        }
    }
#endif
    dma_set_priority(dma_address, stream, DMA_PL_HIGH);

    dma_enable_transfer_complete_interrupt(dma_address, stream);
    dma_enable_transfer_error_interrupt(dma_address, stream);
    //dma_enable_half_transfer_interrupt(dma_address, stream);

    nvic_set_priority(nvic_dma_get_channel_base(unit) + stream, 8 << 4);
    nvic_enable_irq(nvic_dma_get_channel_base(unit) + stream);

    dma_enable_stream(dma_address, stream);
}

void actor_dma_tx_stop(uint8_t unit, uint8_t stream, uint8_t channel) {
    uint32_t dma_address = dma_get_address(unit);
    nvic_disable_irq(nvic_dma_get_channel_base(unit) + stream);
    dma_disable_stream(dma_address, stream);
}

#ifdef DMA_CHANNEL1

void dma1_channel1_isr(void) {
    actors_dma_notify(1, 1);
}
void dma1_channel2_isr(void) {
    actors_dma_notify(1, 2);
}
void dma1_channel3_isr(void) {
    actors_dma_notify(1, 3);
}
void dma1_channel4_isr(void) {
    actors_dma_notify(1, 4);
}
void dma1_channel5_isr(void) {
    actors_dma_notify(1, 5);
}
void dma1_channel6_isr(void) {
    actors_dma_notify(1, 6);
}
void dma1_channel7_isr(void) {
    actors_dma_notify(1, 7);
}
void dma1_channel8_isr(void) {
    actors_dma_notify(1, 8);
}

void dma2_channel1_isr(void) {
    actors_dma_notify(2, 1);
}
void dma2_channel2_isr(void) {
    actors_dma_notify(2, 2);
}
void dma2_channel3_isr(void) {
    actors_dma_notify(2, 3);
}
void dma2_channel4_isr(void) {
    actors_dma_notify(2, 4);
}
void dma2_channel5_isr(void) {
    actors_dma_notify(2, 5);
}
void dma2_channel6_isr(void) {
    actors_dma_notify(2, 6);
}
void dma2_channel7_isr(void) {
    actors_dma_notify(2, 7);
}
void dma2_channel8_isr(void) {
    actors_dma_notify(2, 8);
}

#else

void dma1_stream0_isr(void) {
    actors_dma_notify(1, 0);
}
void dma1_stream1_isr(void) {
    actors_dma_notify(1, 1);
}
void dma1_stream2_isr(void) {
    actors_dma_notify(1, 2);
}
void dma1_stream3_isr(void) {
    actors_dma_notify(1, 3);
}
void dma1_stream4_isr(void) {
    actors_dma_notify(1, 4);
}
void dma1_stream5_isr(void) {
    actors_dma_notify(1, 5);
}
void dma1_stream6_isr(void) {
    actors_dma_notify(1, 6);
}
void dma1_stream7_isr(void) {
    actors_dma_notify(1, 7);
}

void dma2_stream0_isr(void) {
    actors_dma_notify(2, 0);
}
void dma2_stream1_isr(void) {
    actors_dma_notify(2, 1);
}
void dma2_stream2_isr(void) {
    actors_dma_notify(2, 2);
}
void dma2_stream3_isr(void) {
    actors_dma_notify(2, 3);
}
void dma2_stream4_isr(void) {
    actors_dma_notify(2, 4);
}
void dma2_stream5_isr(void) {
    actors_dma_notify(2, 5);
}
void dma2_stream6_isr(void) {
    actors_dma_notify(2, 6);
}
void dma2_stream7_isr(void) {
    actors_dma_notify(2, 7);
}
#endif
