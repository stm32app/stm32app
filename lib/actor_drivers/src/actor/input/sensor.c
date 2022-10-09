#include "sensor.h"

static actor_signal_t sensor_validate(input_sensor_properties_t *properties) {
    return 0;
}

static actor_signal_t sensor_construct(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

static actor_signal_t sensor_start(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

static actor_signal_t sensor_stop(input_sensor_t *sensor) {
    (void)sensor;
    return 0;
}

// pass over adc value to the linked actor
static actor_signal_t sensor_value_received(input_sensor_t *sensor, actor_t *actor, void *value, void *channel) {
    (void)channel; /*unused*/
    if (sensor->adc == actor) {
        actor_send(sensor->actor, sensor->target_actor, value, sensor->target_argument);
    }
    return 0;
}

static actor_signal_t sensor_link(input_sensor_t *sensor) {
    return actor_link(sensor->actor, (void **)&sensor->adc, sensor->properties->adc_index,
                      (void *)(uint32_t)sensor->properties->adc_channel);
}

static actor_signal_t sensor_accept(input_sensor_t *sensor, actor_t *target, void *argument) {
    sensor->target_actor = target;
    sensor->target_argument = argument;
    return 0;
}

static actor_signal_t sensor_signal_received(input_sensor_t *sensor, actor_signal_t signal, actor_t *caller, void *arg) {
    switch (signal) {
    case ACTOR_SIGNAL_LINK:
        return sensor_accept(sensor, caller, arg);
    default:
        break;
    }
    return 0;
}

static actor_signal_t sensor_phase_changed(input_sensor_t *sensor, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_CONSTRUCTION:
        return sensor_construct(sensor);
    case ACTOR_PHASE_LINKAGE:
        return sensor_link(sensor);
    case ACTOR_PHASE_START:
        return sensor_start(sensor);
    case ACTOR_PHASE_STOP:
        return sensor_stop(sensor);
    default:
        break;
    }
    return 0;
}
actor_interface_t input_sensor_class = {
    .type = INPUT_SENSOR,
    .size = sizeof(input_sensor_t),
    .phase_subindex = INPUT_SENSOR_PHASE,
    .validate = (actor_method_t)sensor_validate,
    .signal_received = (actor_signal_received_t)sensor_signal_received,
    .value_received = (actor_value_received_t)sensor_value_received,
    .phase_changed = (actor_phase_changed_t)sensor_phase_changed,
};
