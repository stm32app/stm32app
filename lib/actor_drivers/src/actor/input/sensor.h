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

/* Start of autogenerated OD accessors */
typedef enum input_sensor_properties_indecies {
  INPUT_SENSOR_DISABLED = 0x01,
  INPUT_SENSOR_PORT = 0x02,
  INPUT_SENSOR_PIN = 0x03,
  INPUT_SENSOR_ADC_INDEX = 0x04,
  INPUT_SENSOR_ADC_CHANNEL = 0x05,
  INPUT_SENSOR_PHASE = 0x06
} input_sensor_properties_indecies_t;

/* 0x80XX06: Current phase of a device (one of values in DEVICE_PHASE enum) */
static inline void input_sensor_set_phase(input_sensor_t *sensor, uint8_t value) { 
    actor_set_property_numeric(sensor->actor, INPUT_SENSOR_PHASE, (uint32_t)(value), sizeof(uint8_t));
}
static inline uint8_t input_sensor_get_phase(input_sensor_t *sensor) {
    return *((uint8_t *) actor_get_property_pointer(sensor->actor, INPUT_SENSOR_PHASE));
}
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif