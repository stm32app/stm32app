#include "can.h"
#include <actor/lib/gpio.h>


static actor_signal_t can_validate(transport_can_properties_t *properties) {
    return properties->tx_port == 0 || properties->rx_port == 0;
}

static actor_signal_t can_construct(transport_can_t *can) {
    return 0;
}

static actor_signal_t can_start(transport_can_t *can) {
    debug_printf("    > CAN%i TX ", can->actor->seq + 1);
    gpio_configure_output(can->properties->tx_port, can->properties->tx_pin, 0);
    debug_printf("    > CAN%i RX ", can->actor->seq + 1);
    gpio_configure_input(can->properties->rx_port, can->properties->rx_pin);

#ifdef STM32F1
    if (can->properties->tx_port == 2 && can->properties->tx_pin == 8) {
        rcc_periph_clock_enable(RCC_AFIO);
        gpio_primary_remap(                   // Map CAN1 to use PB8/PB9
            AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, // Optional
            AFIO_MAPR_CAN1_REMAP_PORTB);
    }
#endif

    // Only configure CAN interface if it's not claimed by CANopenNode
    if (can->canopen == NULL) {
    }

    return 0;
}

static actor_signal_t can_stop(transport_can_t *can) {
    (void)can;
    return 0;
}

// CANopenNode's driver configures its CAN interface so we dont have to
static actor_signal_t can_accept(transport_can_t *can, actor_t *origin, void *arg) {
    (void)arg;
    can->canopen = origin;
    return 0;
}

static actor_signal_t can_signal_receieved(transport_can_t *can, actor_signal_t signal, actor_t *caller, void *arg) {
    switch (signal) {
        case ACTOR_SIGNAL_LINK:
            return can_accept(can, caller, arg);
        default:
            break;
    }
    return 0;
}

static actor_signal_t can_phase_changed(transport_can_t *can, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_START:
        return can_start(can);
    case ACTOR_PHASE_STOP:
        return can_stop(can);
    case ACTOR_PHASE_CONSTRUCTION:
        return can_construct(can);
    default:
        return 0;
    }
}

actor_interface_t transport_can_class = {
    .type = TRANSPORT_CAN,
    .size = sizeof(transport_can_t),
    .phase_subindex = TRANSPORT_CAN_PHASE,
    .validate = (actor_method_t)can_validate,
    .signal_received = (actor_signal_received_t)  can_signal_receieved,
    .phase_changed = (actor_phase_changed_t)  can_phase_changed,
};
