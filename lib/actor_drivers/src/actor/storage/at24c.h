#ifndef INC_STORAGE_AT24C
#define INC_STORAGE_AT24C

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/actor.h>

/* Start of autogenerated OD types */
/* 0x7400: Storage AT24C
   I2C-based EEPROM */
typedef struct storage_at24c_properties {
    uint8_t parameter_count;
    uint16_t i2c_index;
    uint8_t i2c_address;
    uint16_t start_address;
    uint16_t page_size;
    uint16_t size;
    uint8_t phase;
} storage_at24c_properties_t;
/* End of autogenerated OD types */

struct storage_at24c {
    actor_t *actor;
    storage_at24c_properties_t *properties;
    transport_i2c_t *i2c;
    actor_job_t *job;
    actor_buffer_t *source_buffer;
    actor_buffer_t *target_buffer;
};

extern actor_interface_t storage_at24c_class;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif