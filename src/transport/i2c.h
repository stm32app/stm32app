#ifndef INC_DEV_I2C
#define INC_DEV_I2C

#ifdef __cplusplus
extern "C" {
#endif

#include "core/actor.h"
#include <libopencm3/stm32/i2c.h>
#include "lib/dma.h"
#include "core/buffer.h"

#define I2C_UNITS 3

/* Start of autogenerated OD types */
/* 0x6260: Transport I2C 1
   Serial protocol */
typedef struct transport_i2c_properties {
    uint8_t parameter_count;
    uint8_t dma_rx_unit;
    uint8_t dma_rx_stream;
    uint8_t dma_rx_channel;
    uint8_t dma_rx_circular_buffer_size;
    uint16_t rx_pool_max_size;
    uint16_t rx_pool_initial_size;
    uint16_t rx_pool_block_size;
    uint8_t dma_tx_unit;
    uint8_t dma_tx_stream;
    uint8_t dma_tx_channel;
    uint8_t af;
    uint8_t smba_pin;
    uint8_t smba_port;
    uint8_t sda_port;
    uint8_t sda_pin;
    uint8_t scl_port;
    uint8_t scl_pin;
    int8_t frequency; // in mhz 
    uint8_t databits;
    uint8_t phase;
    uint8_t slave_address;
} transport_i2c_properties_t;
/* End of autogenerated OD types */

struct transport_i2c{
    actor_t *actor;
    transport_i2c_properties_t *properties;
    uint32_t clock;
    uint32_t address;
    uint32_t ev_irq;
    uint32_t er_irq;


    app_signal_t incoming_signal;
    app_task_t task;
    uint16_t task_retries;
    
    app_double_buffer_t read;
    uint8_t ready;
} ;


typedef struct {
  uint8_t slave_address;
  uint16_t memory_address;
} transport_i2c_event_argument_t;


void *i2c_pack_event_argument(uint8_t device_address, uint16_t memory_address);
transport_i2c_event_argument_t *i2c_unpack_event_argument(void **argument);

extern actor_class_t transport_i2c_class;

/* Start of autogenerated OD accessors */
typedef enum transport_i2c_properties_properties {
  TRANSPORT_I2C_DMA_RX_UNIT = 0x01,
  TRANSPORT_I2C_DMA_RX_STREAM = 0x02,
  TRANSPORT_I2C_DMA_RX_CHANNEL = 0x03,
  TRANSPORT_I2C_DMA_RX_CIRCULAR_BUFFER_SIZE = 0x04,
  TRANSPORT_I2C_RX_POOL_MAX_SIZE = 0x05,
  TRANSPORT_I2C_RX_POOL_INITIAL_SIZE = 0x06,
  TRANSPORT_I2C_RX_POOL_BLOCK_SIZE = 0x07,
  TRANSPORT_I2C_DMA_TX_UNIT = 0x08,
  TRANSPORT_I2C_DMA_TX_STREAM = 0x09,
  TRANSPORT_I2C_DMA_TX_CHANNEL = 0x0A,
  TRANSPORT_I2C_AF = 0x0B,
  TRANSPORT_I2C_SMBA_PIN = 0x0C,
  TRANSPORT_I2C_SMBA_PORT = 0x0D,
  TRANSPORT_I2C_SDA_PORT = 0x0E,
  TRANSPORT_I2C_SDA_PIN = 0x0F,
  TRANSPORT_I2C_SCL_PORT = 0x10,
  TRANSPORT_I2C_SCL_PIN = 0x11,
  TRANSPORT_I2C_FREQUENCY = 0x12,
  TRANSPORT_I2C_DATABITS = 0x13,
  TRANSPORT_I2C_PHASE = 0x14,
  TRANSPORT_I2C_SLAVE_ADDRESS = 0x15
} transport_i2c_properties_properties_t;

/* 0x62XX14: null */
#define transport_i2c_set_phase(i2c, value) actor_set_property_numeric(i2c->actor, (uint32_t) value, sizeof(uint8_t), TRANSPORT_I2C_PHASE)
#define transport_i2c_get_phase(i2c) *((uint8_t *) actor_get_property_pointer(i2c->actor, &(uint8_t[sizeof(uint8_t)]{}), sizeof(uint8_t), TRANSPORT_I2C_PHASE)
/* 0x62XX15: null */
#define transport_i2c_set_slave_address(i2c, value) actor_set_property_numeric(i2c->actor, (uint32_t) value, sizeof(uint8_t), TRANSPORT_I2C_SLAVE_ADDRESS)
#define transport_i2c_get_slave_address(i2c) *((uint8_t *) actor_get_property_pointer(i2c->actor, &(uint8_t[sizeof(uint8_t)]{}), sizeof(uint8_t), TRANSPORT_I2C_SLAVE_ADDRESS)
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif