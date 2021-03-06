#ifndef INC_MODULE_MCU
#define INC_MODULE_MCU

#ifdef __cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/rcc.h>
#include <actor/env.h>
#include <actor/actor.h>

/* Start of autogenerated OD types */
/* 0x6000: Module MCUnull */
typedef struct module_mcu_properties {
    uint8_t parameter_count;
    char family[8];
    char board_type[10];
    uint32_t storage_index;
    uint8_t phase;
    int16_t cpu_temperature;
    uint32_t startup_time; // In milliseconds 
} module_mcu_properties_t;
/* End of autogenerated OD types */

struct module_mcu {
    actor_t *actor;
    module_mcu_properties_t *properties;
    actor_t *storage;
    const struct rcc_clock_scale *clock;
};

extern actor_class_t module_mcu_class;




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
