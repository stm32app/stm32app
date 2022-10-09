#ifndef INC_STORAGE_SDCARD
#define INC_STORAGE_SDCARD

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/actor.h>
//#include "coru.h"
#include "lfs.h"

/* Start of autogenerated OD types */
/* 0x7500: Storage SDCard
   SDIO-based SDcard */
typedef struct storage_sdcard_properties {
    uint8_t parameter_count;
    uint16_t sdio_index;
    uint32_t fs_read_size; // Minimum size of a block read. All read operations will be a  multiple of this value. 
    uint32_t fs_program_size; // Minimum size of a block program. All program operations will be a multiple of this value. 
    int32_t fs_block_cycles; // Number of erase cycles before littlefs evicts metadata logs and moves the metadata to another block.  
    uint16_t fs_cache_size; // Size of block caches. Each cache buffers a portion of a block in RAM. 
    uint32_t fs_lookahead_size; // Size of the lookahead buffer in bytes. A larger lookahead buffer increases the number of blocks found during an allocation pass.  
    uint32_t fs_name_max_size; // Optional upper limit on length of file names in bytes.  
    uint32_t fs_file_max_size; // Optional upper limit on files in bytes.  
    uint32_t fs_attr_max_size; // Optional upper limit on custom attributes in bytes.  
    uint32_t fs_metadata_max_size; // Optional upper limit on total space given to metadata pairs in bytes.  
    char fs_volume_name[17];
    uint8_t phase;
    uint32_t capacity;
    uint32_t block_size;
    uint32_t block_count;
    uint32_t max_bus_clock_frequency;
    uint8_t csd_version;
    uint16_t relative_card_address;
    uint8_t manufacturer_id;
    uint16_t oem_id;
    char product_name[6];
    uint8_t product_revision;
    uint32_t serial_number;
    uint16_t manufacturing_date;
    uint8_t version; // 2 or 1 
    bool_t high_capacity; // 1 for SDHC/SDXC card 
} storage_sdcard_properties_t;
/* End of autogenerated OD types */

struct storage_sdcard {
    actor_t *actor;
    storage_sdcard_properties_t *properties;
    actor_t *sdio;
    actor_job_t *job;
    actor_thread_t *thread;
    actor_buffer_t *coroutine_stack_buffer;
    actor_buffer_t *lookahead_buffer;
    actor_buffer_t *read_buffer;
    actor_buffer_t *prog_buffer;
    struct lfs_config *fs_config;
    lfs_file_t *fs_file;
    lfs_t *fs;
};


extern actor_interface_t storage_sdcard_class;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif