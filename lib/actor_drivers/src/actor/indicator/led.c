#include "led.h"
#include <actor/lib/gpio.h>

static actor_signal_t led_property_after_change(indicator_led_t *led, uint8_t index, void *value, void *old) {
    switch (index) {
    case INDICATOR_LED_DUTY_CYCLE:
        if (*((uint8_t *)value) == 0) {
            actor_set_phase(led->actor, ACTOR_PHASE_IDLE);
            actor_gpio_clear(led->properties->port, led->properties->pin);
        } else if (*((uint8_t *)old) == 0) {
            actor_set_phase(led->actor, ACTOR_PHASE_OPERATION);
            actor_gpio_set(led->properties->port, led->properties->pin);
        }
        return ACTOR_SIGNAL_OK;
    default:
        return ACTOR_SIGNAL_UNAFFECTED;
    }
}

static actor_signal_t led_validate(indicator_led_properties_t *properties) {
    return properties->port == 0 || properties->pin == 0;
}

static actor_signal_t led_start(indicator_led_t *led) {
    debug_printf("    > LED%i", led->actor->seq + 1);
    gpio_configure_output_pulldown(led->properties->port, led->properties->pin, GPIO_MEDIUM);
    (void)led;
    actor_set_phase(led->actor, ACTOR_PHASE_IDLE);
    return 0;
}

static actor_signal_t led_stop(indicator_led_t *led) {
    (void)led;
    return 0;
}

static actor_signal_t led_phase_changed(indicator_led_t *led, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_START:
        return led_start(led);
    case ACTOR_PHASE_STOP:
        return led_stop(led);
    default:
        return 0;
    }
}

actor_interface_t indicator_led_class = {
    .type = INDICATOR_LED,
    .size = sizeof(indicator_led_t),
    .phase_subindex = INDICATOR_LED_PHASE,
    .validate = (actor_method_t)led_validate,
    .phase_changed = (actor_phase_changed_t)led_phase_changed,
    .property_after_change = (actor_property_changed_t) led_property_after_change,
};
