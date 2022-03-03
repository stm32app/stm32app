#ifndef INC_SPI
#define INC_SPI

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/actor.h>
#include <actor/lib/dma.h>
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
    uint16_t dma_rx_circular_buffer_size;
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
    actor_event_t processed_event;      // current reading job
    uint8_t *dma_rx_circular_buffer;        // circular buffer for DMA
    uint16_t dma_rx_circular_buffer_cursor; // current ingested position in rx buffer
    uint32_t rx_bytes_target;
    uint32_t tx_bytes_target;
    uint32_t tx_bytes_sent;
    actor_buffer_t *ring_buffer;      
    actor_buffer_t *output_buffer;      // pool that allocates growing memory chunk for recieved messages
};

extern actor_class_t transport_spi_class;

/* Initiate Tx transmission */
int transport_spi_write(transport_spi_t *spi, actor_t *writer, void *argument, uint8_t *tx_buffer, uint16_t tx_size);

/* Initiate Rx transmission */
int transport_spi_read(transport_spi_t *spi, actor_t *reader, void *argument);





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_SPI_H */