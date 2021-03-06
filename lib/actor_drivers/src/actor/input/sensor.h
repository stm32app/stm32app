#ifndef INC_DEV_SENSOR
#define INC_DEV_SENSOR

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/actor.h>
#include <actor/module/adc.h>

/* Start of autogenerated OD types */
/* 0x8000: Input Sensor 1
   A sensor that measures a single analog value (i.e. current meter, tank level meter) */
typedef struct input_sensor_properties {
    uint8_t parameter_count;
    uint16_t disabled; // Values other than zero will prevent device from initializing 
    uint8_t port; // Analog GPIO port (1 for A, 2 for B, etc), required 
    uint8_t pin; // Analog GPIO pin, required 
    uint16_t adc_index; // OD Index of ADC unit that will take measurements, must play well with the gpio setup 
    uint8_t adc_channel; // ADC channel unique for unit to handle measurements 
    uint8_t phase; // Current phase of a device (one of values in DEVICE_PHASE enum) 
} input_sensor_properties_t;
/* End of autogenerated OD types */

struct input_sensor {
    actor_t *actor;
    input_sensor_properties_t *properties;
    actor_t *target_actor;
    void *target_argument;
    module_adc_t *adc;
};


extern actor_class_t input_sensor_class;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif