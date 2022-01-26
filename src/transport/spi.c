#include "spi.h"
#include "module/timer.h"

volatile uint8_t dummy_byte = 0;

/* SPI must be within range */
static app_signal_t spi_validate(transport_spi_properties_t *properties) {
    return 0;
}

static app_signal_t spi_construct(transport_spi_t *spi) {
    spi->rx_pool.v_limit = spi->properties->rx_pool_max_size;
    spi->rx_pool.v_blksize = spi->properties->rx_pool_block_size;

    switch (spi->actor->seq) {
    case 0:
        spi->clock = RCC_SPI1;
        spi->address = SPI1;
        break;
    case 1:
#ifdef SPI2_BASE
        spi->clock = RCC_SPI2;
        spi->address = SPI2;
        break;
#else
        return 1;
#endif
    case 2:
#ifdef SPI3_BASE
        spi->clock = RCC_SPI3;
        spi->address = SPI3;
        break;
#else
        return 1;
#endif
    case 3:
#ifdef SPI4_BASE
        spi->clock = RCC_SPI4;
        spi->address = SPI4;
        break;
#else
        return 1;
#endif
    case 4:
#ifdef SPI5_BASE
        spi->clock = RCC_SPI5;
        spi->address = SPI5;
        break;
#else
        return 1;
#endif
    case 5:
#ifdef SPI6_BASE
        spi->clock = RCC_SPI6;
        spi->address = SPI6;
        break;
#else
        return 1;
#endif
    default: return 1;
    }
    return 0;
}

static app_signal_t spi_destruct(transport_spi_t *spi) {
    (void)spi;
    return 0;
}

static app_signal_t spi_start(transport_spi_t *spi) {
    (void)spi;
    rcc_periph_clock_enable(spi->clock);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_SPI1EN);
    SPI1_I2SCFGR = 0; // disable i2s??

    log_printf("    > SPI%i SS\n", spi->actor->seq + 1);
    gpio_configure_output_af_pullup(spi->properties->ss_port, spi->properties->ss_pin, GPIO_FAST, 5);
    actor_gpio_set(spi->properties->ss_port, spi->properties->ss_pin);
    log_printf("    > SPI%i SCK", spi->actor->seq + 1);
    gpio_configure_output_af_pullup(spi->properties->sck_port, spi->properties->sck_pin, GPIO_FAST, 5);
    log_printf("    > SPI%i MOSI", spi->actor->seq + 1);
    gpio_configure_output_af_pullup(spi->properties->mosi_port, spi->properties->mosi_pin, GPIO_FAST, 5);
    log_printf("    > SPI%i MISO", spi->actor->seq + 1);
    gpio_configure_input(spi->properties->miso_port, spi->properties->miso_pin);

    /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
    spi_reset(spi->address);

    if (TRANSPORT_SPI_MODE_FULL_DUPLEX == spi->properties->mode) {
        spi_set_full_duplex_mode(spi->address);
    }

    /* Set up SPI in Master mode with:
     * Clock baud rate: 1/64 of peripheral clock frequency
     * Clock polarity: Idle High
     * Clock phase: Data valid on 2nd clock pulse
     * Data frame format: 8-bit
     * Frame format: MSB First
     */
    uint8_t clock, polarity;
    switch (spi->properties->mode) {
    case 3:
        clock = SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE;
        polarity = SPI_CR1_CPHA_CLK_TRANSITION_2;
        break;
    case 2:
        clock = SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE;
        polarity = SPI_CR1_CPHA_CLK_TRANSITION_1;
        break;
    case 1:
        clock = SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE;
        polarity = SPI_CR1_CPHA_CLK_TRANSITION_2;
        break;
    default:
        clock = SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE;
        polarity = SPI_CR1_CPHA_CLK_TRANSITION_1;
        break;
    }
    if (!spi->properties->is_slave) {
        spi_init_master(spi->address, SPI_CR1_BAUDRATE_FPCLK_DIV_256, clock, polarity, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
    } else {
        return 1;
    }

    /*
     * Set NSS management to software.
     *
     * Note:
     * Setting nss high is very important, even if we are controlling the GPIO
     * ourselves this bit needs to be at least set to 1, otherwise the spi
     * peripheral will not send any data out.
     */
    // spi_enable_software_slave_management(spi->address);
    // spi_set_nss_high(spi->address);
    spi_disable_software_slave_management(spi->address);
    spi_enable_ss_output(spi->address);
    spi_enable(spi->address);
    return 0;
}

static app_signal_t spi_stop(transport_spi_t *spi) {
    spi_reset(spi->clock);
    spi_disable(spi->clock);
    return 0;
}

static app_signal_t spi_allocate_rx_buffer(transport_spi_t *spi) {
    spi->rx_buffer = malloc(spi->properties->rx_buffer_size);
    spi->rx_buffer_cursor = 0;
    return spi->rx_buffer == NULL;
}

/* Set timer to signal actor in specified amount of time */
static app_signal_t spi_schedule_rx_timeout(transport_spi_t *spi) {
    return module_timer_timeout(spi->actor->app->timer, spi->actor, (void *)ACTOR_REQUESTING, 1000 + spi->properties->dma_rx_idle_timeout);
}

static uint32_t spi_dma_get_required_rx_bytes(transport_spi_t *spi) {
    return spi->tx_bytes_target + spi->rx_bytes_target;
}

static uint32_t spi_dma_get_required_tx_bytes(transport_spi_t *spi) {
    return spi->tx_bytes_target + spi->rx_bytes_target;
}


static uint16_t spi_dma_get_effectve_rx_buffer_size(transport_spi_t *spi) {
    uint32_t bytes = spi_dma_get_required_rx_bytes(spi);
    return spi->rx_bytes_target > 0 && bytes < spi->properties->rx_buffer_size ? bytes : spi->properties->rx_buffer_size;;
}

/* Check if DMAs circular buffer position is still the same */
static app_signal_t spi_dma_read_is_idle(transport_spi_t *spi) {
    return actor_dma_get_buffer_position(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi_dma_get_effectve_rx_buffer_size(spi)) ==
           spi->rx_buffer_cursor;
}

// Initiate dma write. If size is 0 for the sent message, it will send zeroes indefinitely
static app_signal_t spi_dma_write(transport_spi_t *spi, uint8_t *data) {
    actor_register_dma(spi->properties->dma_tx_unit, spi->properties->dma_tx_stream, spi->actor);
    uint32_t tx_bytes_required = spi_dma_get_required_tx_bytes(spi);

    log_printf("   > SPI%u\t", spi->actor->seq + 1);
    log_printf("TX started\tDMA%u(%u/%u)\t%ub payload\t%ub total\n", spi->properties->dma_tx_unit, spi->properties->dma_tx_stream, spi->properties->dma_tx_channel, spi->tx_bytes_target, tx_bytes_required);
    
    actor_dma_tx_start((uint32_t) & (SPI_DR(spi->address)), spi->properties->dma_tx_unit, spi->properties->dma_tx_stream,
                       spi->properties->dma_tx_channel, data == NULL ? dummy_byte : data, tx_bytes_required, true);
    spi_enable_tx_dma(spi->address);
    return 0;
}

static app_signal_t spi_dma_read(transport_spi_t *spi) {
    if (spi->rx_buffer == NULL) {
        int error = spi_allocate_rx_buffer(spi);
        if (error != 0) {
            return error;
        }
    }
    actor_register_dma(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->actor);
    uint16_t buffer_size = spi_dma_get_effectve_rx_buffer_size(spi);
    uint32_t rx_bytes_required = spi_dma_get_required_rx_bytes(spi);

    log_printf("   > SPI%u\t", spi->actor->seq + 1);
    log_printf("RX started\tDMA%u(%u/%u)\t%ub payload\t%ub total\n", spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->properties->dma_rx_channel, spi->rx_bytes_target, rx_bytes_required);
    actor_dma_rx_start((uint32_t) & (SPI_DR(spi->address)), spi->properties->dma_rx_unit, spi->properties->dma_rx_stream,
                       spi->properties->dma_rx_channel, spi->rx_buffer, buffer_size, rx_bytes_required == 0 || rx_bytes_required > buffer_size);
    spi_enable_rx_dma(spi->address);
    // schedule timeout to detect end of rx transmission
    spi_schedule_rx_timeout(spi);
    return 0;
}

static app_signal_t spi_dma_transceive(transport_spi_t *spi, uint8_t *data, size_t size, size_t response_size) {
    spi->rx_bytes_target = response_size;
    spi->tx_bytes_target = size;
    spi_dma_write(spi, data);
    spi_dma_read(spi);
    return 0;
}


// todo: Read DR register
static app_signal_t spi_dma_write_complete(transport_spi_t *spi) {
    log_printf("   > SPI%u\t", spi->actor->seq + 1);
    actor_dma_tx_stop(spi->properties->dma_tx_unit, spi->properties->dma_tx_stream, spi->properties->dma_tx_channel);
    actor_event_finalize(spi->actor, &spi->processed_event);
    actor_tick_catchup(spi->actor, spi->actor->ticks->input);
}

/* Send the resulting read contents back via a queue */
static app_signal_t spi_dma_read_complete(transport_spi_t *spi) {
    log_printf("   > SPI%u\t", spi->actor->seq + 1);
    actor_dma_rx_stop(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->properties->dma_rx_channel);
    module_timer_clear(spi->actor->app->timer, spi->actor, (void *)ACTOR_REQUESTING);
    app_event_t *response = app_event_from_vpool(
        &(app_event_t){.type = APP_EVENT_RESPONSE, .producer = spi->actor, .consumer = spi->processed_event.producer}, &spi->rx_pool);
    
    // shift pointer by tx bytes
    response->argument = (void *) spi->tx_bytes_target;
    response->data = response->data - spi->tx_bytes_target;
    response->size -= spi->tx_bytes_target;


    actor_event_finalize(spi->actor, &spi->processed_event);
    app_publish(spi->actor->app, response);
    actor_tick_catchup(spi->actor, spi->actor->ticks->input);
}


static app_signal_t spi_dma_read_possibly_complete(transport_spi_t *spi, bool_t is_idle) {

    // copy receieved dma chunk from circular buffer into a growable pool
    actor_dma_ingest(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->rx_buffer, spi_dma_get_effectve_rx_buffer_size(spi),
                     &spi->rx_buffer_cursor, &spi->rx_pool);

    uint32_t rx_bytes_required = spi_dma_get_required_rx_bytes(spi);

    // RX is complete if it receieved all expected bytes. In case message has unknown length, RX is expected to become idle
    if (spi->rx_bytes_target == 0 ? is_idle : spi->rx_pool.v_off == rx_bytes_required) {
        spi_dma_read_complete(spi);
        return spi_dma_write_complete(spi);
    }

    // RX has to rely on polling for DMA to become idle if:
    if (spi->rx_bytes_target == 0 ||                                // - RX size is unknown
        (rx_bytes_required >= spi->properties->rx_buffer_size && // - OR RX size is known to be larger than DMA circular buffer
         rx_bytes_required - spi->rx_pool.v_off < spi->properties->rx_buffer_size)) { // and it is is expecting the last chunk
        spi_schedule_rx_timeout(spi);
    }

    // Otherwise it has to wait for the next interrupt
    return 1;
}

static app_signal_t spi_dma_write_possibly_complete(transport_spi_t *spi) {
    if (spi->tx_bytes_target == 0) {

    } else {
        if (actor_dma_get_buffer_position(spi->properties->dma_tx_unit, spi->properties->dma_tx_stream, spi->tx_bytes_target) !=
            spi->tx_bytes_target) {
            return 1;
        }
    }
    return spi_dma_write_complete(spi);
}

static app_signal_t spi_on_write(transport_spi_t *spi, app_event_t *event) {
    return spi_dma_transceive(spi, event->data, event->size, (uint32_t)event->argument);
}

static app_signal_t spi_on_read(transport_spi_t *spi, app_event_t *event) {
    return spi_dma_transceive(spi, dummy_byte, 0, event->size);
}

static app_signal_t spi_on_transfer(transport_spi_t *spi, app_event_t *event) {
    return spi_dma_transceive(spi, event->data, event->size, (uint32_t)event->argument);
}
    /*
    if (read_size == 0 && write_size == 0) {
        // Read of unknown length
        // Sending: Dummy bytes circularily
        // End condition: Read is idle (hopefully)
        spi_dma_read(spi, 0);
        return spi_dma_write(spi, dummy_byte, 1);
    } else if (read_size > 0 && write_size == 0) {
        // Read of known length
        // Sending: Dummy bytes circularily
        // End condition: Read is complete
        spi_dma_read(spi, read_size);
        return spi_dma_write(spi, dummy_byte, 1);
    } else if (read_size > 0 && write_size > 0) {
        // Write and read of known lengths
        // Sending: Message circularily (if write is longer that read there will be "garbage")
        // End condition: Read is complete
        spi_dma_read(spi, read_size + write_size);
        return spi_dma_write(spi, event->data, write_size);
    } else if (read_size == 0 && write_size > 0) {
        // Read of known length, doesnt fit in a buffer
        // Sending: Message circularily (will yield some garbage)
        // End condition: Read is done, limit has to be set dynamically
        spi_dma_read(spi, 0);
        return spi_dma_write(spi, event->data, write_size);
    }*/

static app_signal_t spi_on_signal(transport_spi_t *spi, actor_t *actor, app_signal_t signal, void *source) {
    switch (signal) {
    // DMA interrupts on buffer filling up half-way or all the way up
    case APP_SIGNAL_DMA_TRANSFERRING:
        if (actor_dma_match_source(source, spi->properties->dma_rx_unit, spi->properties->dma_rx_stream)) {
            spi_dma_read_possibly_complete(spi, false);
        } else {
            spi_dma_write_possibly_complete(spi);
        }
        break;
    // Timer module helps SPI to poll for idle line when RX size is unknown
    case APP_SIGNAL_TIMEOUT:
        if ((uint32_t)source == ACTOR_REQUESTING) {
            spi_dma_read_possibly_complete(spi, spi_dma_read_is_idle(spi));
        }
        break;
    default: break;
    }

    return 0;
}
static app_signal_t spi_tick_input(transport_spi_t *spi, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_READ: return actor_event_handle_and_process(spi->actor, event, &spi->processed_event, spi_on_read);
    case APP_EVENT_WRITE: return actor_event_handle_and_process(spi->actor, event, &spi->processed_event, spi_on_write);
    case APP_EVENT_TRANSFER: return actor_event_handle_and_process(spi->actor, event, &spi->processed_event, spi_on_transfer);
    default: return 0;
    }
}

actor_class_t transport_spi_class = {
    .type = TRANSPORT_SPI,
    .size = sizeof(transport_spi_t),
    .phase_subindex = TRANSPORT_SPI_PHASE,
    .validate = (app_method_t)spi_validate,
    .construct = (app_method_t)spi_construct,
    .destruct = (app_method_t)spi_destruct,
    .start = (app_method_t)spi_start,
    .tick_input = (actor_on_tick_t)spi_tick_input,
    .on_signal = (actor_on_signal_t)spi_on_signal,
    .stop = (app_method_t)spi_stop,
};