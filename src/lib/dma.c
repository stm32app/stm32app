#include "dma.h"
#include "core/buffer.h"

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
    debug_printf("> DMA%i interrupt\n", unit);
    volatile actor_t *actor = actors_dma[DMA_INDEX(unit, index)];

    if (dma_get_interrupt_flag(dma_get_address(unit), index, DMA_TEIF | DMA_DMEIF /* | DMA_FEIF*/)) {

        error_printf("DMA%i error in channel %i %s%s%s \n", unit, index,
                     dma_get_interrupt_flag(dma_get_address(unit), index, DMA_TEIF) ? " TEIF" : "",
                     dma_get_interrupt_flag(dma_get_address(unit), index, DMA_DMEIF) ? " DMEIF" : "",
                     dma_get_interrupt_flag(dma_get_address(unit), index, DMA_FEIF) ? " FEIF" : "");

        if (actor->class->on_signal(actor->object, NULL, APP_SIGNAL_DMA_ERROR, actor_dma_pack_source(unit, index))) {
            dma_clear_interrupt_flags(dma_get_address(unit), index, DMA_TEIF | DMA_DMEIF /* | DMA_FEIF*/);
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

void actor_dma_rx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *buffer, size_t buffer_size,
                        bool_t circular_mode) {
    uint32_t dma_address = dma_get_address(unit);

    rcc_periph_clock_enable(dma_get_clock_address(unit));
    rcc_peripheral_enable_clock(dma_get_ahb(unit), dma_get_peripherial_clock(unit));

    dma_stream_reset(dma_address, stream);
    dma_disable_stream(dma_address, stream);
    dma_channel_select(dma_address, stream, channel << DMA_SxCR_CHSEL_SHIFT);

    dma_disable_peripheral_increment_mode(dma_address, stream);
    dma_enable_memory_increment_mode(dma_address, stream);

    dma_set_peripheral_address(dma_address, stream, periphery_address);
    dma_set_memory_address(dma_address, stream, (uint32_t)buffer);
    dma_set_number_of_data(dma_address, stream, buffer_size);

    dma_set_read_from_peripheral(dma_address, stream);
    if (circular_mode) {
        dma_enable_circular_mode(dma_address, stream);
        dma_enable_half_transfer_interrupt(dma_address, stream);
    } else {
        //dma_enable_circular_mode(dma_address, stream);
        dma_disable_half_transfer_interrupt(dma_address, stream);
        //dma_enable_half_transfer_interrupt(dma_address, stream);
    }

    dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_8BIT);
    dma_set_memory_size(dma_address, stream, DMA_MSIZE_8BIT);
    dma_set_priority(dma_address, stream, DMA_PL_VERY_HIGH);

    dma_enable_direct_mode(dma_address, stream);
    dma_disable_fifo_error_interrupt(dma_address, stream);
    dma_enable_direct_mode_error_interrupt(dma_address, stream);

    dma_enable_transfer_complete_interrupt(dma_address, stream);

    nvic_set_priority(nvic_dma_get_channel_base(unit) + stream, 8 << 4);
    nvic_enable_irq(nvic_dma_get_channel_base(unit) + stream);

    dma_enable_stream(dma_address, stream);
}

void actor_dma_rx_stop(uint8_t unit, uint8_t stream, uint8_t channel) {
    uint32_t dma_address = dma_get_address(unit);
    nvic_disable_irq(nvic_dma_get_channel_base(unit) + stream);
    dma_disable_stream(dma_address, stream);
}

void actor_dma_tx_start(uint32_t periphery_address, uint8_t unit, uint8_t stream, uint8_t channel, uint8_t *data, size_t size,
                        bool_t circular_mode) {
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

    dma_set_peripheral_size(dma_address, stream, DMA_PSIZE_8BIT);
    dma_set_memory_size(dma_address, stream, DMA_MSIZE_8BIT);
    dma_set_priority(dma_address, stream, DMA_PL_HIGH);

    dma_enable_direct_mode(dma_address, stream);
    dma_disable_fifo_error_interrupt(dma_address, stream);
    dma_enable_direct_mode_error_interrupt(dma_address, stream);

    dma_enable_transfer_complete_interrupt(dma_address, stream);
    dma_enable_transfer_error_interrupt(dma_address, stream);
    dma_enable_half_transfer_interrupt(dma_address, stream);

    nvic_set_priority(nvic_dma_get_channel_base(unit) + stream, 8 << 4);
    nvic_enable_irq(nvic_dma_get_channel_base(unit) + stream);

    dma_enable_stream(dma_address, stream);
}

void actor_dma_tx_stop(uint8_t unit, uint8_t stream, uint8_t channel) {
    uint32_t dma_address = dma_get_address(unit);
    nvic_enable_irq(nvic_dma_get_channel_base(unit) + stream);
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
