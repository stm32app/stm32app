#ifndef _DEV_EPAPER_H_
#define _DEV_EPAPER_H_

#define screen_epaper_FULL 0
#define screen_epaper_PART 1

#include <actor/actor.h>
#include <actor/transport/spi.h>
/* Start of autogenerated OD types */
/* 0x9000: Screen Epaper 1
   E-ink screen with low power consumption and low update frequency */
typedef struct screen_epaper_properties {
    uint8_t parameter_count;
    uint16_t spi_index;
    uint8_t dc_port;
    uint8_t dc_pin;
    uint8_t cs_port;
    uint8_t cs_pin;
    uint8_t busy_port;
    uint8_t busy_pin;
    uint8_t reset_port;
    uint8_t reset_pin;
    uint16_t width;
    uint16_t height;
    uint16_t mode;
    uint8_t phase;
} screen_epaper_properties_t;
/* End of autogenerated OD types */

struct screen_epaper {
    actor_t *actor;
    screen_epaper_properties_t *properties;
    transport_spi_t *spi;
    uint8_t resetting_phase;
    uint8_t initializing_phase;
};

extern actor_class_t screen_epaper_class;


#endif