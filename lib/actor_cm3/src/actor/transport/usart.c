#include "usart.h"
#include <actor/lib/dma.h>

/* USART must be within range */
static actor_signal_t usart_validate(transport_usart_properties_t *properties) {
    return 0;
}

static actor_signal_t usart_construct(transport_usart_t *usart) {

    usart->dma_rx_address = dma_get_address(usart->properties->dma_rx_unit);
    usart->dma_tx_address = dma_get_address(usart->properties->dma_tx_unit);

    usart->dma_rx_circular_buffer = actor_malloc(usart->properties->dma_rx_circular_buffer_size);

    if (usart->dma_rx_address == 0 || usart->dma_tx_address == 0) {
        return 0;
    }

    switch (usart->actor->seq) {
    case 0:
        usart->address = USART1;
        usart->clock = RCC_USART1;
        break;
    case 1:
#ifdef USART2_BASE
        usart->address = USART2;
        usart->clock = RCC_USART2;
        break;
#endif
        return 1;
    case 2:
#ifdef USART3_BASE
        usart->address = USART3;
        usart->clock = RCC_USART3;
        break;
#endif
        return 1;
    case 3:
#ifdef USART4_BASE
        usart->address = USART4;
        usart->clock = RCC_USART4;
        break;
#endif
        return 1;
    default: return 1;
    }
    return 0;
}

int transport_usart_send(transport_usart_t *usart, char *data, int size) {
    (void)usart;
    (void)data;
    (void)size;
    return 0;
}

static uint16_t transport_usart_get_buffer_size(transport_usart_t *usart) {
    return usart->properties->dma_rx_circular_buffer_size;
}

static uint16_t transport_usart_get_buffer_size_left(transport_usart_t *usart) {
    return dma_get_number_of_data(usart->properties->dma_rx_unit, usart->properties->dma_rx_stream);
}

static uint16_t transport_usart_get_buffer_size_written(transport_usart_t *usart) {
    return transport_usart_get_buffer_size(usart) - transport_usart_get_buffer_size_left(usart);
}

static actor_signal_t usart_accept(transport_usart_t *usart, actor_t *target, void *argument) {
    usart->target_actor = target;
    usart->target_argument = argument;
    return 0;
}

static actor_signal_t usart_destruct(transport_usart_t *usart) {
    actor_free(usart->dma_rx_circular_buffer);
    return 0;
}

static void transport_usart_tx_dma_stop(transport_usart_t *usart) {
    actor_dma_tx_stop(usart->properties->dma_tx_unit, usart->properties->dma_tx_stream, usart->properties->dma_tx_channel);
    usart_disable_tx_dma(usart->address);
    // usart_disable_tx_complete_interrupt(usart->address);
}

/* Configure memory -> usart transfer*/
static void transport_usart_tx_dma_start(transport_usart_t *usart, uint8_t *data, uint16_t size) {
    transport_usart_tx_dma_stop(usart);
    actor_dma_tx_start((uint32_t) & (USART_DR(usart->address)), usart->properties->dma_tx_unit, usart->properties->dma_tx_stream,
                        usart->properties->dma_tx_channel, data, size, false, 1, 0, 1);
    usart_enable_tx_dma(usart->address);
}

static void transport_usart_rx_dma_stop(transport_usart_t *usart) {
    actor_dma_rx_stop(usart->properties->dma_tx_unit, usart->properties->dma_tx_stream, usart->properties->dma_tx_channel);
    usart_disable_rx_dma(usart->address);
    // usart_disable_tx_complete_interrupt(usart->address);
}

/* Configure memory <- usart transfer*/
static void transport_usart_rx_dma_start(transport_usart_t *usart, uint8_t *data, uint16_t size) {
    transport_usart_rx_dma_stop(usart);
    actor_dma_rx_start((uint32_t) & (USART_DR(usart->address)), usart->properties->dma_tx_unit, usart->properties->dma_tx_stream,
                        usart->properties->dma_tx_channel, data, size, false, 1, 0, 1);
    usart_enable_rx_dma(usart->address);
}

static actor_signal_t usart_start(transport_usart_t *usart) {
    usart_set_baudrate(usart->address, usart->properties->baudrate);
    usart_set_databits(usart->address, usart->properties->databits);
    usart_set_stopbits(usart->address, USART_STOPBITS_1);
    usart_set_mode(usart->address, USART_MODE_TX_RX);
    usart_set_parity(usart->address, USART_PARITY_NONE);
    usart_set_flow_control(usart->address, USART_FLOWCONTROL_NONE);

    // FIXME: Not available everywhere
    //usart_enable_idle_interrupt(usart->address);
    rcc_periph_clock_enable(usart->clock);

    usart_enable(usart->address);

    // transport_usart_rx_dma_start(usart);

    return 0;
}

static actor_signal_t usart_stop(transport_usart_t *usart) {
    (void)usart;
    return 0;
}
static actor_signal_t usart_signal_received(transport_usart_t *usart, actor_signal_t signal, actor_t *caller, void *arg) {
    switch (signal) {
    case ACTOR_SIGNAL_TX_COMPLETE: // DMA_TCIF, transfer complete
        actor_signal(usart->target_actor, ACTOR_SIGNAL_TX_COMPLETE, usart->actor, usart->target_argument);
        transport_usart_tx_dma_stop(usart);
        break;
    case ACTOR_SIGNAL_RX_COMPLETE: // USART IDLE, probably complete transfer
        actor_signal(usart->target_actor, ACTOR_SIGNAL_RX_COMPLETE, usart->actor, usart->target_argument);
        break;
    case ACTOR_SIGNAL_LINK:
        return usart_accept(usart, caller, arg);
    default:
        break;
    }

    return 0;
}

static actor_signal_t usart_phase_changed(transport_usart_t *usart, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_CONSTRUCTION:
        return usart_construct(usart);
    case ACTOR_PHASE_START:
        return usart_start(usart);
    case ACTOR_PHASE_STOP:
        return usart_stop(usart);
    case ACTOR_PHASE_DESTRUCTION:
        return usart_destruct(usart);
    default:
        break;
    }
    return 0;
}

actor_interface_t transport_usart_class = {
    .type = TRANSPORT_USART,
    .size = sizeof(transport_usart_t),
    .phase_subindex = TRANSPORT_USART_PHASE,
    .validate = (actor_method_t)usart_validate,
    .signal_received = (actor_signal_received_t)usart_signal_received,
    .phase_changed = (actor_phase_changed_t)usart_phase_changed,
};
