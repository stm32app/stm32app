#ifndef INC_SPI
#define INC_SPI

#ifdef __cplusplus
extern "C" {
#endif

#include "core/actor.h"
#include "lib/dma.h"
#include "lib/vpool.h"
#include "lib/vpool.h"
#include <libopencm3/stm32/spi.h>


#define TRANSPORT_SPI_MODE_FULL_DUPLEX 0
/* Start of autogenerated OD types */
/* 0x6220: Transport SPI 1
   ADC Unit used for high-volume sampling of analog signals */
typedef struct transport_spi_properties {
    uint8_t parameter_count;
    uint8_t is_slave;
    uint8_t software_ss_control;
    uint8_t mode;
    uint8_t dma_rx_unit;
    uint8_t dma_rx_stream;
    uint8_t dma_rx_channel;
    uint32_t dma_rx_idle_timeout; // In microseconds 
    uint16_t rx_buffer_size;
    uint16_t rx_pool_max_size;
    uint16_t rx_pool_initial_size;
    uint16_t rx_pool_block_size;
    uint8_t dma_tx_unit;
    uint8_t dma_tx_stream;
    uint8_t dma_tx_channel;
    uint8_t af_index; // Index of the corresponding AF function for gpio pins 
    uint8_t ss_port;
    uint8_t ss_pin;
    uint8_t sck_port;
    uint8_t sck_pin;
    uint8_t miso_port;
    uint8_t miso_pin;
    uint8_t mosi_port;
    uint8_t mosi_pin;
    uint8_t phase;
} transport_spi_properties_t;
/* End of autogenerated OD types */

struct transport_spi {
    actor_t *actor;
    transport_spi_properties_t *properties;
    uint32_t clock;
    uint32_t address;
    app_event_t processed_event;      // current reading job
    uint8_t *rx_buffer;        // circular buffer for DMA
    uint16_t rx_buffer_cursor; // current ingested position in rx buffer
    struct vpool rx_pool;      // pool that allocates growing memory chunk for recieved messages
};

extern actor_class_t transport_spi_class;

/* Initiate Tx transmission */
int transport_spi_write(transport_spi_t *spi, actor_t *writer, void *argument, uint8_t *tx_buffer, uint16_t tx_size);

/* Initiate Rx transmission */
int transport_spi_read(transport_spi_t *spi, actor_t *reader, void *argument);



/* Start of autogenerated OD accessors */
typedef enum transport_spi_properties_properties {
  TRANSPORT_SPI_IS_SLAVE = 0x01,
  TRANSPORT_SPI_SOFTWARE_SS_CONTROL = 0x02,
  TRANSPORT_SPI_MODE = 0x03,
  TRANSPORT_SPI_DMA_RX_UNIT = 0x04,
  TRANSPORT_SPI_DMA_RX_STREAM = 0x05,
  TRANSPORT_SPI_DMA_RX_CHANNEL = 0x06,
  TRANSPORT_SPI_DMA_RX_IDLE_TIMEOUT = 0x07,
  TRANSPORT_SPI_RX_BUFFER_SIZE = 0x08,
  TRANSPORT_SPI_RX_POOL_MAX_SIZE = 0x09,
  TRANSPORT_SPI_RX_POOL_INITIAL_SIZE = 0x0A,
  TRANSPORT_SPI_RX_POOL_BLOCK_SIZE = 0x0B,
  TRANSPORT_SPI_DMA_TX_UNIT = 0x0C,
  TRANSPORT_SPI_DMA_TX_STREAM = 0x0D,
  TRANSPORT_SPI_DMA_TX_CHANNEL = 0x0E,
  TRANSPORT_SPI_AF_INDEX = 0x0F,
  TRANSPORT_SPI_SS_PORT = 0x10,
  TRANSPORT_SPI_SS_PIN = 0x11,
  TRANSPORT_SPI_SCK_PORT = 0x12,
  TRANSPORT_SPI_SCK_PIN = 0x13,
  TRANSPORT_SPI_MISO_PORT = 0x14,
  TRANSPORT_SPI_MISO_PIN = 0x15,
  TRANSPORT_SPI_MOSI_PORT = 0x16,
  TRANSPORT_SPI_MOSI_PIN = 0x17,
  TRANSPORT_SPI_PHASE = 0x18
} transport_spi_properties_properties_t;

/* 0x62XX18: null */
#define transport_spi_set_phase(spi, value) actor_set_property_numeric(spi->actor, (uint32_t) value, sizeof(uint8_t), TRANSPORT_SPI_PHASE)
#define transport_spi_get_phase(spi) *((uint8_t *) actor_get_property_pointer(spi->actor, &(uint8_t[sizeof(uint8_t)]{}), sizeof(uint8_t), TRANSPORT_SPI_PHASE)
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_SPI_H */