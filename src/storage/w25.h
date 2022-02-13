#ifndef INC_DEV_WINBOND
#define INC_DEV_WINBOND

#ifdef __cplusplus
extern "C" {
#endif

#include "core/actor.h"
#include "lib/bytes.h"

#define W25_SR1_BUSY 0x01

enum w25_commands {
    W25_CMD_MANUF_ACTOR = 0x90,
    W25_CMD_JEDEC_ID = 0x9F,
    W25_CMD_UNLOCK = 0x06,
    W25_CMD_LOCK = 0x04,
    W25_CMD_READ_SR1 = 0x05,
    W25_CMD_READ_SR2 = 0x35,
    W25_CMD_CHIP_ERASE = 0xC7,
    W25_CMD_READ_DATA = 0x03,
    W25_CMD_FAST_READ = 0x0B,
    W25_CMD_WRITE_DATA = 0x02,
    W25_CMD_READ_UID = 0x4B,
    W25_CMD_PWR_ON = 0xAB,
    W25_CMD_PWR_OFF = 0xB9,
    W25_CMD_ERA_SECTOR = 0x20,
    W25_CMD_ERA_32K = 0x52,
    W25_CMD_ERA_64K = 0xD8
};

/* Start of autogenerated OD types */
/* 0x7100: Storage W25
   Winbond flash storage device over SPI */
typedef struct storage_w25_properties {
    uint8_t parameter_count;
    uint16_t spi_index;
    uint16_t page_size;
    uint16_t size;
    uint8_t phase;
} storage_w25_properties_t;
/* End of autogenerated OD types */

struct storage_w25 {
    actor_t *actor;
    storage_w25_properties_t *properties;
    transport_spi_t *spi;
    app_job_t job;
} ;

extern actor_class_t storage_w25_class;

/* Start of autogenerated OD accessors */
typedef enum storage_w25_properties_properties {
  STORAGE_W25_SPI_INDEX = 0x01,
  STORAGE_W25_PAGE_SIZE = 0x02,
  STORAGE_W25_SIZE = 0x03,
  STORAGE_W25_PHASE = 0x04
} storage_w25_properties_properties_t;

/* 0x71XX04: null */
#define storage_w25_set_phase(w25, value) actor_set_property_numeric(w25->actor, STORAGE_W25_PHASE, (uint32_t) (value), sizeof(uint8_t))
#define storage_w25_get_phase(w25) *((uint8_t *) actor_get_property_pointer(w25->actor, STORAGE_W25_PHASE)
/* End of autogenerated OD accessors */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif