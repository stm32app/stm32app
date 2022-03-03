#ifndef INC_ACTOR_CANOPEN
#define INC_ACTOR_CANOPEN

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/actor.h>
#include <CANopen.h>



/* Start of autogenerated OD types */
/* 0x6020: Actor CANopen
   CANOpen framework */
typedef struct actor_canopen_properties {
    uint8_t parameter_count;
    uint16_t can_index; // Values other than zero will prevent device from initializing 
    uint8_t can_fifo_index; // Values other than zero will prevent device from initializing 
    uint16_t green_led_index;
    uint16_t red_led_index;
    uint16_t first_hb_time;
    uint16_t sdo_server_timeout; // in MS 
    uint16_t sdo_client_timeout; // in MS 
    uint8_t phase;
    uint8_t node_id; // Node ID 
    uint16_t bitrate; // Negotiated bitrate 
} actor_canopen_properties_t;
/* End of autogenerated OD types */



struct actor_canopen {
    actor_t *actor;
    actor_canopen_properties_t *properties;
    indicator_led_t *green_led;
    indicator_led_t *red_led;
    transport_can_t *can;
    CO_t *instance;
};

extern actor_class_t actor_canopen_class;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif