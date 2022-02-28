#include "module_mcu.h"

static ODR_t mcu_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    system_mcu_t *mcu = stream->object;
    (void)mcu;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static actor_signal_t mcu_validate(system_mcu_properties_t *properties) {
    return 0;
}

static actor_signal_t mcu_construct(system_mcu_t *mcu) {
    return 0;
}

static actor_signal_t mcu_start(system_mcu_t *mcu) {
#if defined(STM32F1)
    mcu->clock = &rcc_hsi_propertiess[RCC_CLOCK_HSE8_72MHZ];
#elif defined(STM32F4)
    mcu->clock = &rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ];
#endif
    rcc_clock_setup_pll(mcu->clock);
    return 0;
}

static actor_signal_t mcu_stop(system_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static actor_signal_t mcu_link(system_mcu_t *mcu) {
    (void)mcu;
    return actor_link(mcu->actor, (void **)&mcu->storage, mcu->properties->storage_index, NULL);
}

static actor_signal_t mcu_phase(system_mcu_t *mcu, actor_phase_t phase) {
    (void)mcu;
    (void)phase;
    return 0;
}

actor_class_t system_mcu_class = {
    .type = SYSTEM_MCU,
    .size = sizeof(system_mcu_t),
    .phase_subindex = SYSTEM_MCU_PHASE,
    .validate = (actor_method_t)mcu_validate,
    .construct = (actor_method_t)mcu_construct,
    .link = (actor_method_t)mcu_link,
    .start = (actor_method_t)mcu_start,
    .stop = (actor_method_t)mcu_stop,
    .on_phase = (actor_on_phase_t)mcu_phase,
    .property_write = mcu_property_write,
};
