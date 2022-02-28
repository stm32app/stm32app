#ifndef INC_SYSTEM_MCU
#define INC_SYSTEM_MCU

#ifdef __cplusplus
extern "C" {
#endif

#include <libopencm3/stm32/rcc.h>
#include "core/app.h"

/* Start of autogenerated OD types */
/* 0x6000: System MCUnull */
typedef struct system_mcu_properties {
    uint8_t parameter_count;
    char family[8];
    char board_type[10];
    uint32_t storage_index;
    uint8_t phase;
    int16_t cpu_temperature;
    uint32_t startup_time; // In milliseconds 
} system_mcu_properties_t;
/* End of autogenerated OD types */

struct system_mcu {
    actor_t *actor;
    system_mcu_properties_t *properties;
    actor_t *storage;
    const struct rcc_clock_scale *clock;
};

extern actor_class_t system_mcu_class;

/* Start of autogenerated OD accessors */
typedef enum system_mcu_properties_indecies {
  SYSTEM_MCU_FAMILY = 0x01,
  SYSTEM_MCU_BOARD_TYPE = 0x02,
  SYSTEM_MCU_STORAGE_INDEX = 0x03,
  SYSTEM_MCU_PHASE = 0x04,
  SYSTEM_MCU_CPU_TEMPERATURE = 0x05,
  SYSTEM_MCU_STARTUP_TIME = 0x06
} system_mcu_properties_indecies_t;

/* 0x60XX04: null */
static inline void system_mcu_set_phase(system_mcu_t *mcu, uint8_t value) { 
    actor_set_property_numeric(mcu->actor, SYSTEM_MCU_PHASE, (uint32_t)(value), sizeof(uint8_t));
}
static inline uint8_t system_mcu_get_phase(system_mcu_t *mcu) {
    return *((uint8_t *) actor_get_property_pointer(mcu->actor, SYSTEM_MCU_PHASE));
}
/* 0x60XX05: null */
static inline void system_mcu_set_cpu_temperature(system_mcu_t *mcu, int16_t value) { 
    actor_set_property_numeric(mcu->actor, SYSTEM_MCU_CPU_TEMPERATURE, (uint32_t)(value), sizeof(int16_t));
}
static inline int16_t system_mcu_get_cpu_temperature(system_mcu_t *mcu) {
    return *((int16_t *) actor_get_property_pointer(mcu->actor, SYSTEM_MCU_CPU_TEMPERATURE));
}
/* 0x60XX06: In milliseconds */
static inline void system_mcu_set_startup_time(system_mcu_t *mcu, uint32_t value) { 
    actor_set_property_numeric(mcu->actor, SYSTEM_MCU_STARTUP_TIME, (uint32_t)(value), sizeof(uint32_t));
}
static inline uint32_t system_mcu_get_startup_time(system_mcu_t *mcu) {
    return *((uint32_t *) actor_get_property_pointer(mcu->actor, SYSTEM_MCU_STARTUP_TIME));
}
/* End of autogenerated OD accessors */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif