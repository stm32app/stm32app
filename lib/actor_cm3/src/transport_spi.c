#include "spi.h"
#include "module/timer.h"
#include "actor_buffer.h"

uint8_t dummy_byte = 0;

/* SPI must be within range */
static actor_signal_t spi_validate(transport_spi_properties_t *properties) {
    return 0;
}

static actor_signal_t spi_construct(transport_spi_t *spi) {
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
    default:
        return 1;
    }
    return 0;
}

static actor_signal_t spi_destruct(transport_spi_t *spi) {
    (void)spi;
    return 0;
}

static actor_signal_t spi_start(transport_spi_t *spi) {
    (void)spi;
    rcc_periph_clock_enable(spi->clock);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_SPI1EN);
    // SPI1_I2SCFGR = 0; // disable i2s??

    debug_printf("    > SPI%i SS\n", spi->actor->seq + 1);
    gpio_configure_output_pullup(spi->properties->ss_port, spi->properties->ss_pin, GPIO_FAST);
    debug_printf("    > SPI%i SCK", spi->actor->seq + 1);
    gpio_configure_af_pullup(spi->properties->sck_port, spi->properties->sck_pin, GPIO_FAST, 5);
    debug_printf("    > SPI%i MOSI", spi->actor->seq + 1);
    gpio_configure_af_pullup(spi->properties->mosi_port, spi->properties->mosi_pin, GPIO_FAST, 5);
    debug_printf("    > SPI%i MISO", spi->actor->seq + 1);

    gpio_mode_setup(GPIOX(spi->properties->miso_port), GPIO_MODE_AF, GPIO_PUPD_NONE, 1 << spi->properties->miso_pin);
    gpio_set_af(GPIOX(spi->properties->miso_port), GPIO_AF5, 1 << spi->properties->miso_pin);

    /* Reset SPI, SPI_CR1 register cleared, SPI is disabled */
    spi_reset(spi->address);
    spi_disable(spi->address);

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

    spi_set_nss_high(spi->address);
    spi_disable_software_slave_management(spi->address);
    spi_enable_ss_output(spi->address);
    spi_disable_crc(spi->address);
    spi_set_master_mode(spi->address);

    spi_set_full_duplex_mode(spi->address);

    return 0;
}

static actor_signal_t spi_stop(transport_spi_t *spi) {
    spi_reset(spi->clock);
    spi_disable(spi->clock);
    return 0;
}

static actor_signal_t spi_allocate_rx_circular_buffer(transport_spi_t *spi) {
    spi->dma_rx_circular_buffer = actor_malloc(spi->properties->dma_rx_circular_buffer_size);
    spi->dma_rx_circular_buffer_cursor = 0;
    return spi->dma_rx_circular_buffer == NULL;
}

/* Set timer to signal actor in specified amount of time */
static actor_signal_t spi_schedule_rx_timeout(transport_spi_t *spi) {
    return module_timer_timeout(spi->actor->node->timer, spi->actor, (void *)ACTOR_REQUESTING, 1000 + spi->properties->dma_rx_idle_timeout);
}

static uint32_t spi_dma_get_required_rx_bytes(transport_spi_t *spi) {
    return spi->tx_bytes_target + spi->rx_bytes_target;
}

static uint32_t spi_dma_get_required_tx_bytes(transport_spi_t *spi) {
    return spi->tx_bytes_target + spi->rx_bytes_target;
}

static uint16_t spi_dma_get_effectve_rx_circular_buffer_size(transport_spi_t *spi) {
    uint32_t bytes = spi_dma_get_required_rx_bytes(spi);
    return spi->rx_bytes_target > 0 && bytes < spi->properties->dma_rx_circular_buffer_size ? bytes : spi->properties->dma_rx_circular_buffer_size;
    ;
}

/* Check if DMAs circular buffer position is still the same */
static actor_signal_t spi_dma_read_is_idle(transport_spi_t *spi) {
    return actor_dma_get_buffer_position(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream,
                                         spi_dma_get_effectve_rx_circular_buffer_size(spi)) == spi->dma_rx_circular_buffer_cursor;
}

// Initiate dma write. If size is 0 for the sent message, it will send zeroes indefinitely
static actor_signal_t spi_dma_write(transport_spi_t *spi, uint8_t *data) {
    actor_register_dma(spi->properties->dma_tx_unit, spi->properties->dma_tx_stream, spi->actor);
    uint32_t tx_bytes_required = spi_dma_get_required_tx_bytes(spi);

    debug_printf("   > SPI%u\t", spi->actor->seq + 1);
    debug_printf("TX started\tDMA%u(%u/%u)\t%lub payload\t%lub total\n", spi->properties->dma_tx_unit, spi->properties->dma_tx_stream,
               spi->properties->dma_tx_channel, spi->tx_bytes_target, tx_bytes_required);

    actor_dma_tx_start((uint32_t) & (SPI_DR(spi->address)), spi->properties->dma_tx_unit, spi->properties->dma_tx_stream,
                       spi->properties->dma_tx_channel, data == NULL ? &dummy_byte : data, tx_bytes_required, true, 1, 0, 1);
    spi_enable_tx_dma(spi->address);
    return 0;
}

static actor_signal_t spi_dma_read(transport_spi_t *spi) {
    if (spi->dma_rx_circular_buffer == NULL) {
        int error = spi_allocate_rx_circular_buffer(spi);
        if (error != 0) {
            return error;
        }
    }
    actor_register_dma(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->actor);
    uint16_t buffer_size = spi_dma_get_effectve_rx_circular_buffer_size(spi);
    uint32_t rx_bytes_required = spi_dma_get_required_rx_bytes(spi);

    debug_printf("   > SPI%u\t", spi->actor->seq + 1);
    debug_printf("RX started\tDMA%u(%u/%u)\t%lub payload\t%lub total\n", spi->properties->dma_rx_unit, spi->properties->dma_rx_stream,
               spi->properties->dma_rx_channel, spi->rx_bytes_target, rx_bytes_required);
    actor_dma_rx_start((uint32_t) & (SPI_DR(spi->address)), spi->properties->dma_rx_unit, spi->properties->dma_rx_stream,
                       spi->properties->dma_rx_channel, spi->dma_rx_circular_buffer, buffer_size,
                       rx_bytes_required == 0 || rx_bytes_required > buffer_size, 1, 0, 1);
    spi_enable_rx_dma(spi->address);
    // schedule timeout to detect end of rx transmission
    spi_schedule_rx_timeout(spi);
    return 0;
}

static actor_signal_t spi_dma_transceive(transport_spi_t *spi, uint8_t *data, size_t size, size_t response_size) {
    /*
    spi_enable(spi->address);

        spi_xfer(spi->address, 0xAB);
        spi_disable(spi->address);
    spi_enable(spi->address);
        uint8_t sr1;
        do {
            spi_xfer(spi->address,0x05);
            sr1 = spi_xfer(spi->address,0x00);
            error_printf("busy %u\n", sr1);
        } while (sr1 & 0x01);

        spi_disable(spi->address);
    spi_enable(spi->address);

        do {
            spi_xfer(spi->address,0x05);
            sr1 = spi_xfer(spi->address,0x00);
            error_printf("busy %u\n", sr1);
        } while (sr1 & 0x01);
        spi_disable(spi->address);
    spi_enable(spi->address);

        spi_xfer(spi->address,0x9f);


        uint8_t ma = spi_xfer(spi->address,0x0);		 // Manuf.
        uint8_t me = spi_xfer(spi->address,0x0);// Memory Type
        uint8_t ca = spi_xfer(spi->address,0x0);// Capacity

    error_printf("result %u %u %u\n", ma, me, ca);*/

    spi->rx_bytes_target = response_size;
    spi->tx_bytes_target = size;
    spi->dma_rx_circular_buffer_cursor = 0;
    volatile uint8_t temp_data __attribute__((unused));
    while (SPI_SR(spi->address) & (SPI_SR_RXNE | SPI_SR_OVR)) {
        error_printf("Reading spi dr???\n");
        temp_data = SPI_DR(spi->address);
    }
    spi_dma_write(spi, data);
    spi_dma_read(spi);
    spi_enable(spi->address);
    return 0;
}

/* Send the resulting read contents back via a queue */
static actor_signal_t spi_dma_read_complete(transport_spi_t *spi) {
    debug_printf("   > SPI%u\t", spi->actor->seq + 1);
    actor_dma_tx_stop(spi->properties->dma_tx_unit, spi->properties->dma_tx_stream, spi->properties->dma_tx_channel);
    debug_printf("   > SPI%u\t", spi->actor->seq + 1);
    actor_dma_rx_stop(spi->properties->dma_rx_unit, spi->properties->dma_rx_stream, spi->properties->dma_rx_channel);
    module_timer_clear(spi->actor->node->timer, spi->actor, (void *)ACTOR_REQUESTING);

    spi_disable(spi->address);

    volatile uint8_t temp_data __attribute__((unused));
    while (SPI_SR(spi->address) & (SPI_SR_RXNE | SPI_SR_OVR)) {
        error_printf("Reading spi dr?!?!?\n");
        temp_data = SPI_DR(spi->address);
    }

    actor_buffer_t *buffer = actor_double_buffer_detach(spi->ring_buffer, &spi->output_buffer, spi->actor);
    actor_buffer_trim_left(buffer, spi->tx_bytes_target);

    actor_event_finalize(spi->actor, &spi->processed_event);
    actor_publish(spi->actor->node, &((actor_event_t){
        .type = ACTOR_EVENT_RESPONSE, 
        .producer = spi->actor, 
        .consumer = spi->processed_event.producer,
        .data = (uint8_t *) buffer,
        .size = ACTOR_BUFFER_DYNAMIC_SIZE
    }));
    actor_worker_catchup(spi->actor, NULL);

    return 0;
}

static actor_signal_t spi_dma_read_possibly_complete(transport_spi_t *spi, bool is_idle) {
    // copy receieved dma chunk from circular buffer into a growable pool
    uint16_t buffer_size = actor_double_buffer_get_input_size(spi->ring_buffer, spi->output_buffer);
    uint16_t bytes_left = dma_get_number_of_data(dma_get_address(spi->properties->dma_rx_unit), spi->properties->dma_rx_stream);
    actor_double_buffer_ingest_external_write(spi->ring_buffer, spi->output_buffer, buffer_size - bytes_left);

    uint32_t rx_bytes_required = spi_dma_get_required_rx_bytes(spi);

    // RX is complete if it receieved all expected bytes. In case message has unknown length, RX is expected to become idle
    if (spi->rx_bytes_target == 0 ? is_idle : spi->output_buffer->size == rx_bytes_required) {
        spi_dma_read_complete(spi);
        return 0;
    }

    // RX has to rely on polling for DMA to become idle if:
    if (spi->rx_bytes_target == 0 ||                                      // - RX size is unknown
        (rx_bytes_required >= spi->properties->dma_rx_circular_buffer_size && // - OR RX size is known to be larger than DMA circular buffer
         rx_bytes_required - spi->output_buffer->size < spi->properties->dma_rx_circular_buffer_size)) { // and it is is expecting the last chunk
        spi_schedule_rx_timeout(spi);
    }

    // Otherwise it has to wait for the next interrupt
    return 1;
}

static actor_signal_t spi_dma_write_possibly_complete(transport_spi_t *spi) {
    if (spi->tx_bytes_target == 0) {

    } else {
        if (actor_dma_get_buffer_position(spi->properties->dma_tx_unit, spi->properties->dma_tx_stream, spi->tx_bytes_target) !=
            spi->tx_bytes_target) {
            return 1;
        }
    }
    return 0;
    //return spi_dma_write_complete(spi);
}

static actor_signal_t spi_on_write(transport_spi_t *spi, actor_event_t *event) {
    return spi_dma_transceive(spi, event->data, event->size, (uint32_t)event->argument);
}

static actor_signal_t spi_on_read(transport_spi_t *spi, actor_event_t *event) {
    return spi_dma_transceive(spi, &dummy_byte, 0, event->size);
}

static actor_signal_t spi_on_transfer(transport_spi_t *spi, actor_event_t *event) {
    return spi_dma_transceive(spi, event->data, event->size, (uint32_t)event->argument);
}

static actor_signal_t spi_on_signal(transport_spi_t *spi, actor_t *actor, actor_signal_t signal, void *source) {
    switch (signal) {
    // DMA interrupts on buffer filling up half-way or all the way up
    case ACTOR_SIGNAL_DMA_TRANSFERRING:
        if (actor_dma_match_source(source, spi->properties->dma_rx_unit, spi->properties->dma_rx_stream)) {
            spi_dma_read_possibly_complete(spi, false);
        } else {
            // spi_dma_write_possibly_complete(spi);
        }
        break;
    // Timer module helps SPI to poll for idle line when RX size is unknown
    case ACTOR_SIGNAL_TIMEOUT:
        if ((uint32_t)source == ACTOR_REQUESTING) {
            spi_dma_read_possibly_complete(spi, spi_dma_read_is_idle(spi));
        }
        break;
    default:
        break;
    }

    return 0;
}
static actor_signal_t spi_worker_input(transport_spi_t *spi, actor_event_t *event, actor_worker_t *tick, actor_thread_t *thread) {
    switch (event->type) {
    case ACTOR_EVENT_READ:
        return actor_event_handle_and_process(spi->actor, event, &spi->processed_event, spi_on_read);
    case ACTOR_EVENT_WRITE:
        return actor_event_handle_and_process(spi->actor, event, &spi->processed_event, spi_on_write);
    case ACTOR_EVENT_TRANSFER:
        return actor_event_handle_and_process(spi->actor, event, &spi->processed_event, spi_on_transfer);
    default:
        return 0;
    }
}

static actor_signal_t spi_on_event_report(transport_spi_t *spi, actor_event_t *event) {
    if (event->type == ACTOR_EVENT_RESPONSE) {
    }
    return 0;
}

static actor_worker_callback_t spi_on_worker_assignment(transport_spi_t *spi, actor_thread_t *thread) {
    if (spi->actor->node->input == thread) {
        return (actor_worker_callback_t) spi_worker_input;
    }
    return NULL;
}

actor_class_t transport_spi_class = {
    .type = TRANSPORT_SPI,
    .size = sizeof(transport_spi_t),
    .phase_subindex = TRANSPORT_SPI_PHASE,
    .validate = (actor_method_t)spi_validate,
    .construct = (actor_method_t)spi_construct,
    .destruct = (actor_method_t)spi_destruct,
    .start = (actor_method_t)spi_start,
    .on_event_report = (actor_on_event_report_t)spi_on_event_report,
    .on_signal = (actor_on_signal_t)spi_on_signal,
    .on_worker_assignment = (actor_on_worker_assignment_t) spi_on_worker_assignment,
    .stop = (actor_method_t)spi_stop,
};