#include "circuit.h"
#include <actor/lib/gpio.h>

static actor_signal_t circuit_property_after_change(device_circuit_t *circuit, uint8_t index, void *value, void *old) {
    device_circuit_set_state(circuit, device_circuit_get_state(circuit));
    switch (index) {
    case DEVICE_CIRCUIT_DUTY_CYCLE:
        debug_printf("OD - Circuit [%X] duty cycle:  %i\n", circuit->actor->seq, circuit->properties->duty_cycle);
        break;
    case DEVICE_CIRCUIT_CONSUMERS:
        debug_printf("OD - Circuit [%X] consumers: %i\n", circuit->actor->seq, circuit->properties->consumers);
        break;
    }
    return ACTOR_SIGNAL_OK;
}

/* Circuit needs its relay GPIO configured */
static actor_signal_t circuit_validate(device_circuit_properties_t *properties) {
    return properties->port == 0 || properties->pin == 0;
}

static actor_signal_t circuit_link(device_circuit_t *circuit) {
    return actor_link(circuit->actor, (void **)&circuit->current_sensor, circuit->properties->sensor_index, NULL) +
           actor_link(circuit->actor, (void **)&circuit->psu, circuit->properties->psu_index, NULL);
}

// receive value from current sensor
static actor_signal_t circuit_value_received(device_circuit_t *circuit, actor_t *actor, void *value, void *argument) {
    (void)argument;
    if (circuit->current_sensor == actor) {
        device_circuit_set_current(circuit->actor, (uint32_t) value);
    }
    return 1;
}

static actor_signal_t circuit_start(device_circuit_t *circuit) {
    gpio_configure_input_analog(circuit->properties->port, circuit->properties->pin);
    // actor_gpio_set_state(device_circuit_get_state(circuit));

    return 0;
}

static actor_signal_t circuit_stop(device_circuit_t *circuit) {
    (void)circuit;
    return 0;
}

bool device_circuit_get_state(device_circuit_t *circuit) {
    return circuit->properties->duty_cycle > 0 || circuit->properties->consumers > 0;
}

void device_circuit_set_state(device_circuit_t *circuit, bool state) {
    gpio_set_state(circuit->properties->port, circuit->properties->pin, state);
    if (circuit->psu) {
        device_circuit_set_consumers(circuit->psu, device_circuit_get_consumers(circuit->psu) + (state ? 1 : -1));
    }
}

static actor_signal_t circuit_phase_changed(device_circuit_t *circuit, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_LINKAGE:
        return circuit_link(circuit);
    case ACTOR_PHASE_START:
        return circuit_start(circuit);
    case ACTOR_PHASE_STOP:
        return circuit_stop(circuit);
    default:
        break;
    }
    return 0;
}

actor_interface_t device_circuit_class = {
    .type = DEVICE_CIRCUIT,
    .size = sizeof(device_circuit_t),
    .phase_subindex = DEVICE_CIRCUIT_PHASE,
    .phase_changed = (actor_phase_changed_t) circuit_phase_changed,
    .validate = (actor_method_t)circuit_validate,
    .value_received = (actor_value_received_t) circuit_value_received,
    .property_after_change = (actor_property_changed_t) circuit_property_after_change,
};
