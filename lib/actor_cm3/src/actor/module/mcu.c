#include <actor/module/mcu.h>

static actor_signal_t mcu_validate(module_mcu_properties_t *properties) {
    return 0;
}

static actor_signal_t mcu_construct(module_mcu_t *mcu) {
    return 0;
}

static actor_signal_t mcu_start(module_mcu_t *mcu) {
#if defined(STM32F1)
    mcu->clock = &rcc_hsi_propertiess[RCC_CLOCK_HSE8_72MHZ];
#elif defined(STM32F4)
    mcu->clock = &rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ];
#endif
    rcc_clock_setup_pll(mcu->clock);
    return 0;
}

static actor_signal_t mcu_stop(module_mcu_t *mcu) {
    (void)mcu;
    return 0;
}

static actor_signal_t mcu_phase_changed(module_mcu_t *mcu, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_START:
        return mcu_start(mcu);
    case ACTOR_PHASE_STOP:
        return mcu_stop(mcu);
    case ACTOR_PHASE_CONSTRUCTION:
        return mcu_construct(mcu);
//    case ACTOR_PHASE_DESTRUCTION:
//        return mcu_destruct(mcu);
    default:
        return 0;
    }
}

actor_interface_t module_mcu_class = {
    .type = MODULE_MCU,
    .size = sizeof(module_mcu_t),
    .phase_subindex = MODULE_MCU_PHASE,
    .validate = (actor_method_t)mcu_validate,
    .phase_changed = (actor_phase_changed_t)  mcu_phase_changed,
};
