#include "enums.h"
char* get_transport_modbus_cmd_name (uint32_t v) {
switch (v) {
case 1: return "MODBUS_READ_COIL_STATUS";
case 2: return "MODBUS_READ_DISCRETE_INPUTS";
case 3: return "MODBUS_READ_HOLDING_REGISTERS";
case 4: return "MODBUS_READ_INPUT_REGISTERS";
case 5: return "MODBUS_WRITE_SINGLE_COIL";
case 6: return "MODBUS_WRITE_SINGLE_REGISTER";
case 15: return "MODBUS_WRITE_MULTIPLE_COILS";
case 16: return "MODBUS_WRITE_MULTIPLE_REGISTERS";
default: return "Unknown";
}
};

char* get_screen_ili9341_cmd_name (uint32_t v) {
switch (v) {
case 207: return "ILI9341_CMD_POWER_CONTROL_B";
case 237: return "ILI9341_CMD_POWER_SEQUENCE_CONTROL";
case 232: return "ILI9341_CMD_DRIVER_TIMING_CONTROL_A";
case 234: return "ILI9341_CMD_DRIVER_TIMING_CONTROL_B";
case 203: return "ILI9341_CMD_POWER_CONTROL_A";
case 192: return "ILI9341_CMD_POWER_CONTROL_1";
case 193: return "ILI9341_CMD_POWER_CONTROL_2";
case 247: return "ILI9341_CMD_PUMP_RATIO_CONTROL";
case 197: return "ILI9341_CMD_VCOM_CONTROL_1";
case 199: return "ILI9341_CMD_VCOM_CONTROL_2";
case 54: return "ILI9341_CMD_MEMORY_CONTROL";
case 58: return "ILI9341_CMD_PIXEL_FORMAT_SET";
case 177: return "ILI9341_CMD_FRAME_RATE_CONTROL_NORMAL";
case 182: return "ILI9341_CMD_DISPLAY_FUNCTION_CONTROL";
case 242: return "ILI9341_CMD_ENABLE_3g";
case 38: return "ILI9341_CMD_GAMMA_SET";
case 224: return "ILI9341_CMD_POSITIVE_GAMMA_CORRECTION";
case 225: return "ILI9341_CMD_NEGATIVE_GAMMA_CORRECTION";
case 43: return "ILI9341_CMD_PAGE_ADDRESS_SET";
case 42: return "ILI9341_CMD_COLUMN_ADDRESS_SET";
case 17: return "ILI9341_CMD_SLEEP_OUT";
case 16: return "ILI9341_CMD_SLEEP_IN";
case 40: return "ILI9341_CMD_DISPLAY_OFF";
case 41: return "ILI9341_CMD_DISPLAY_ON";
case 32: return "ILI9341_CMD_INVERT_COLORS_OFF";
case 33: return "ILI9341_CMD_INVERT_COLORS_ON";
default: return "Unknown";
}
};

char* get_w25_commands_name (uint32_t v) {
switch (v) {
case 144: return "W25_CMD_MANUF_ACTOR";
case 159: return "W25_CMD_JEDEC_ID";
case 6: return "W25_CMD_UNLOCK";
case 4: return "W25_CMD_LOCK";
case 5: return "W25_CMD_READ_SR1";
case 53: return "W25_CMD_READ_SR2";
case 199: return "W25_CMD_CHIP_ERASE";
case 3: return "W25_CMD_READ_DATA";
case 11: return "W25_CMD_FAST_READ";
case 2: return "W25_CMD_WRITE_DATA";
case 75: return "W25_CMD_READ_UID";
case 171: return "W25_CMD_PWR_ON";
case 185: return "W25_CMD_PWR_OFF";
case 32: return "W25_CMD_ERA_SECTOR";
case 82: return "W25_CMD_ERA_32K";
case 216: return "W25_CMD_ERA_64K";
default: return "Unknown";
}
};

char* get_actor_phase_name (uint32_t v) {
switch (v) {
case 0: return "ACTOR_PHASE_ENABLED";
case 1: return "ACTOR_PHASE_DISABLED";
case 2: return "ACTOR_PHASE_CONSTRUCTION";
case 3: return "ACTOR_PHASE_DESTRUCTION";
case 4: return "ACTOR_PHASE_LINKAGE";
case 5: return "ACTOR_PHASE_START";
case 6: return "ACTOR_PHASE_CALIBRATION";
case 7: return "ACTOR_PHASE_PREPARATION";
case 8: return "ACTOR_PHASE_OPERATION";
case 9: return "ACTOR_PHASE_WORK";
case 10: return "ACTOR_PHASE_IDLE";
case 11: return "ACTOR_PHASE_BUSY";
case 12: return "ACTOR_PHASE_RESET";
case 13: return "ACTOR_PHASE_PAUSE";
case 14: return "ACTOR_PHASE_STOP";
default: return "Unknown";
}
};

char* get_actor_buffer_flag_name (uint32_t v) {
switch (v) {
case 1: return "ACTOR_BUFFER_UNMANAGED";
case 2: return "ACTOR_BUFFER_ALIGNED";
case 4: return "ACTOR_BUFFER_DMA";
case 8: return "ACTOR_BUFFER_EXT";
case 16: return "ACTOR_BUFFER_CHUNKED";
default: return "Unknown";
}
};

char* get_actor_message_type_name (uint32_t v) {
switch (v) {
case 32: return "ACTOR_MESSAGE_THREAD_START";
case 33: return "ACTOR_MESSAGE_THREAD_STOP";
case 34: return "ACTOR_MESSAGE_THREAD_ALARM";
case 0: return "ACTOR_MESSAGE_IDLE";
case 1: return "ACTOR_MESSAGE_READ";
case 2: return "ACTOR_MESSAGE_READ_TO_BUFFER";
case 3: return "ACTOR_MESSAGE_WRITE";
case 4: return "ACTOR_MESSAGE_TRANSFER";
case 5: return "ACTOR_MESSAGE_ERASE";
case 6: return "ACTOR_MESSAGE_MOUNT";
case 7: return "ACTOR_MESSAGE_UNMOUNT";
case 8: return "ACTOR_MESSAGE_RESPONSE";
case 9: return "ACTOR_MESSAGE_LOCK";
case 10: return "ACTOR_MESSAGE_UNLOCK";
case 11: return "ACTOR_MESSAGE_INTROSPECTION";
case 12: return "ACTOR_MESSAGE_DIAGNOSE";
case 13: return "ACTOR_MESSAGE_OPEN";
case 14: return "ACTOR_MESSAGE_QUERY";
case 15: return "ACTOR_MESSAGE_DELETE";
case 16: return "ACTOR_MESSAGE_CLOSE";
case 17: return "ACTOR_MESSAGE_SYNC";
case 18: return "ACTOR_MESSAGE_STATS";
case 19: return "ACTOR_MESSAGE_START";
case 20: return "ACTOR_MESSAGE_STOP";
case 21: return "ACTOR_MESSAGE_ENABLE";
case 22: return "ACTOR_MESSAGE_DISABLE";
default: return "Unknown";
}
};

char* get_actor_message_status_name (uint32_t v) {
switch (v) {
case 0: return "ACTOR_MESSAGE_WAITING";
case 1: return "ACTOR_MESSAGE_RECEIVED";
case 2: return "ACTOR_MESSAGE_ADDRESSED";
case 3: return "ACTOR_MESSAGE_HANDLED";
case 4: return "ACTOR_MESSAGE_DEFERRED";
case 5: return "ACTOR_MESSAGE_FINALIZED";
default: return "Unknown";
}
};

char* get_actor_signal_name (uint32_t v) {
switch (v) {
case 0: return "ACTOR_SIGNAL_OK";
case 1: return "ACTOR_SIGNAL_REPEAT";
case 2: return "ACTOR_SIGNAL_ACCEPTED";
case 3: return "ACTOR_SIGNAL_UNAFFECTED";
case 4: return "ACTOR_SIGNAL_TIMER";
case 5: return "ACTOR_SIGNAL_DMA_TRANSFERRING";
case 6: return "ACTOR_SIGNAL_DMA_IDLE";
case 7: return "ACTOR_SIGNAL_CANCEL";
case 8: return "ACTOR_SIGNAL_RX_COMPLETE";
case 9: return "ACTOR_SIGNAL_TX_COMPLETE";
case 10: return "ACTOR_SIGNAL_CATCHUP";
case 11: return "ACTOR_SIGNAL_RESCHEDULE";
case 12: return "ACTOR_SIGNAL_INCOMING";
case 13: return "ACTOR_SIGNAL_LINK";
case 14: return "ACTOR_SIGNAL_WAIT";
case 15: return "ACTOR_SIGNAL_LOOP";
case 16: return "ACTOR_SIGNAL_TASK_COMPLETE";
case 17: return "ACTOR_SIGNAL_JOB_COMPLETE";
case -1: return "ACTOR_SIGNAL_FAILURE";
case -2: return "ACTOR_SIGNAL_TIMEOUT";
case -3: return "ACTOR_SIGNAL_DMA_ERROR";
case -4: return "ACTOR_SIGNAL_BUSY";
case -5: return "ACTOR_SIGNAL_NOT_FOUND";
case -6: return "ACTOR_SIGNAL_UNCONFIGURED";
case -7: return "ACTOR_SIGNAL_INCOMPLETE";
case -8: return "ACTOR_SIGNAL_CORRUPTED";
case -9: return "ACTOR_SIGNAL_NO_RESPONSE";
case -10: return "ACTOR_SIGNAL_OUT_OF_MEMORY";
case -11: return "ACTOR_SIGNAL_INVALID_ARGUMENT";
case -12: return "ACTOR_SIGNAL_NOT_IMPLEMENTED";
case -13: return "ACTOR_SIGNAL_TRANSFER_FAILED";
case -14: return "ACTOR_SIGNAL_UNSUPPORTED";
default: return "Unknown";
}
};

char* get_actor_file_mode_name (uint32_t v) {
switch (v) {
case 1: return "ACTOR_FILE_READ";
case 2: return "ACTOR_FILE_WRITE";
case 3: return "ACTOR_FILE_READ_WRITE";
case 256: return "ACTOR_FILE_CREATE";
case 512: return "ACTOR_FILE_EXCLUSIVE";
case 1024: return "ACTOR_FILE_TRUNCATE";
case 2048: return "ACTOR_FILE_APPEND";
case 524288: return "ACTOR_FILE_ERROR";
default: return "Unknown";
}
};

char* get_actor_thread_flags_name (uint32_t v) {
switch (v) {
case 1: return "ACTOR_THREAD_SHARED";
case 2: return "ACTOR_THREAD_BLOCKABLE";
default: return "Unknown";
}
};

char* get_device_circuit_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "DEVICE_CIRCUIT_PORT";
case 2: return "DEVICE_CIRCUIT_PIN";
case 3: return "DEVICE_CIRCUIT_LIMIT_CURRENT";
case 4: return "DEVICE_CIRCUIT_LIMIT_VOLTAGE";
case 5: return "DEVICE_CIRCUIT_PSU_INDEX";
case 6: return "DEVICE_CIRCUIT_SENSOR_INDEX";
case 7: return "DEVICE_CIRCUIT_PHASE";
case 8: return "DEVICE_CIRCUIT_DUTY_CYCLE";
case 9: return "DEVICE_CIRCUIT_CURRENT";
case 10: return "DEVICE_CIRCUIT_VOLTAGE";
case 11: return "DEVICE_CIRCUIT_CONSUMERS";
default: return "Unknown";
}
};

char* get_module_mcu_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "MODULE_MCU_FAMILY";
case 2: return "MODULE_MCU_BOARD_TYPE";
case 3: return "MODULE_MCU_STORAGE_INDEX";
case 4: return "MODULE_MCU_PHASE";
case 5: return "MODULE_MCU_CPU_TEMPERATURE";
case 6: return "MODULE_MCU_STARTUP_TIME";
default: return "Unknown";
}
};

char* get_actor_canopen_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "ACTOR_CANOPEN_CAN_INDEX";
case 2: return "ACTOR_CANOPEN_CAN_FIFO_INDEX";
case 3: return "ACTOR_CANOPEN_GREEN_LED_INDEX";
case 4: return "ACTOR_CANOPEN_RED_LED_INDEX";
case 5: return "ACTOR_CANOPEN_FIRST_HB_TIME";
case 6: return "ACTOR_CANOPEN_SDO_SERVER_TIMEOUT";
case 7: return "ACTOR_CANOPEN_SDO_CLIENT_TIMEOUT";
case 8: return "ACTOR_CANOPEN_PHASE";
case 9: return "ACTOR_CANOPEN_NODE_ID";
case 10: return "ACTOR_CANOPEN_BITRATE";
default: return "Unknown";
}
};

char* get_actor_database_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "ACTOR_DATABASE_STORAGE_INDEX";
case 2: return "ACTOR_DATABASE_JOURNAL_BUFFER_SIZE";
case 3: return "ACTOR_DATABASE_PATH";
case 4: return "ACTOR_DATABASE_PHASE";
default: return "Unknown";
}
};

char* get_module_timer_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "MODULE_TIMER_PRESCALER";
case 2: return "MODULE_TIMER_INITIAL_SUBSCRIPTIONS_COUNT";
case 3: return "MODULE_TIMER_PERIOD";
case 4: return "MODULE_TIMER_FREQUENCY";
case 5: return "MODULE_TIMER_PHASE";
default: return "Unknown";
}
};

char* get_transport_can_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_CAN_TX_PORT";
case 2: return "TRANSPORT_CAN_TX_PIN";
case 3: return "TRANSPORT_CAN_RX_PORT";
case 4: return "TRANSPORT_CAN_RX_PIN";
case 5: return "TRANSPORT_CAN_BITRATE";
case 6: return "TRANSPORT_CAN_BRP";
case 7: return "TRANSPORT_CAN_SJW";
case 8: return "TRANSPORT_CAN_PROP";
case 9: return "TRANSPORT_CAN_PH_SEG1";
case 10: return "TRANSPORT_CAN_PH_SEG2";
case 11: return "TRANSPORT_CAN_PHASE";
default: return "Unknown";
}
};

char* get_transport_spi_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_SPI_IS_SLAVE";
case 2: return "TRANSPORT_SPI_SOFTWARE_SS_CONTROL";
case 3: return "TRANSPORT_SPI_MODE";
case 4: return "TRANSPORT_SPI_DMA_RX_UNIT";
case 5: return "TRANSPORT_SPI_DMA_RX_STREAM";
case 6: return "TRANSPORT_SPI_DMA_RX_CHANNEL";
case 7: return "TRANSPORT_SPI_DMA_RX_IDLE_TIMEOUT";
case 8: return "TRANSPORT_SPI_DMA_RX_CIRCULAR_BUFFER_SIZE";
case 9: return "TRANSPORT_SPI_RX_POOL_MAX_SIZE";
case 10: return "TRANSPORT_SPI_RX_POOL_INITIAL_SIZE";
case 11: return "TRANSPORT_SPI_RX_POOL_BLOCK_SIZE";
case 12: return "TRANSPORT_SPI_DMA_TX_UNIT";
case 13: return "TRANSPORT_SPI_DMA_TX_STREAM";
case 14: return "TRANSPORT_SPI_DMA_TX_CHANNEL";
case 15: return "TRANSPORT_SPI_AF_INDEX";
case 16: return "TRANSPORT_SPI_SS_PORT";
case 17: return "TRANSPORT_SPI_SS_PIN";
case 18: return "TRANSPORT_SPI_SCK_PORT";
case 19: return "TRANSPORT_SPI_SCK_PIN";
case 20: return "TRANSPORT_SPI_MISO_PORT";
case 21: return "TRANSPORT_SPI_MISO_PIN";
case 22: return "TRANSPORT_SPI_MOSI_PORT";
case 23: return "TRANSPORT_SPI_MOSI_PIN";
case 24: return "TRANSPORT_SPI_PHASE";
default: return "Unknown";
}
};

char* get_transport_usart_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_USART_DMA_RX_UNIT";
case 2: return "TRANSPORT_USART_DMA_RX_STREAM";
case 3: return "TRANSPORT_USART_DMA_RX_CHANNEL";
case 4: return "TRANSPORT_USART_DMA_RX_CIRCULAR_BUFFER_SIZE";
case 5: return "TRANSPORT_USART_DMA_TX_UNIT";
case 6: return "TRANSPORT_USART_DMA_TX_STREAM";
case 7: return "TRANSPORT_USART_DMA_TX_CHANNEL";
case 8: return "TRANSPORT_USART_BAUDRATE";
case 9: return "TRANSPORT_USART_DATABITS";
case 10: return "TRANSPORT_USART_PHASE";
default: return "Unknown";
}
};

char* get_transport_i2c_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_I2C_DMA_RX_UNIT";
case 2: return "TRANSPORT_I2C_DMA_RX_STREAM";
case 3: return "TRANSPORT_I2C_DMA_RX_CHANNEL";
case 4: return "TRANSPORT_I2C_DMA_RX_CIRCULAR_BUFFER_SIZE";
case 5: return "TRANSPORT_I2C_RX_POOL_MAX_SIZE";
case 6: return "TRANSPORT_I2C_RX_POOL_INITIAL_SIZE";
case 7: return "TRANSPORT_I2C_RX_POOL_BLOCK_SIZE";
case 8: return "TRANSPORT_I2C_DMA_TX_UNIT";
case 9: return "TRANSPORT_I2C_DMA_TX_STREAM";
case 10: return "TRANSPORT_I2C_DMA_TX_CHANNEL";
case 11: return "TRANSPORT_I2C_AF";
case 12: return "TRANSPORT_I2C_SMBA_PIN";
case 13: return "TRANSPORT_I2C_SMBA_PORT";
case 14: return "TRANSPORT_I2C_SDA_PORT";
case 15: return "TRANSPORT_I2C_SDA_PIN";
case 16: return "TRANSPORT_I2C_SCL_PORT";
case 17: return "TRANSPORT_I2C_SCL_PIN";
case 18: return "TRANSPORT_I2C_FREQUENCY";
case 19: return "TRANSPORT_I2C_DATABITS";
case 20: return "TRANSPORT_I2C_PHASE";
case 21: return "TRANSPORT_I2C_SLAVE_ADDRESS";
default: return "Unknown";
}
};

char* get_transport_modbus_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_MODBUS_USART_INDEX";
case 2: return "TRANSPORT_MODBUS_RTS_PORT";
case 3: return "TRANSPORT_MODBUS_RTS_PIN";
case 4: return "TRANSPORT_MODBUS_SLAVE_ADDRESS";
case 5: return "TRANSPORT_MODBUS_TIMEOUT";
case 6: return "TRANSPORT_MODBUS_PHASE";
default: return "Unknown";
}
};

char* get_transport_sdio_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "TRANSPORT_SDIO_DMA_UNIT";
case 2: return "TRANSPORT_SDIO_DMA_STREAM";
case 3: return "TRANSPORT_SDIO_DMA_CHANNEL";
case 4: return "TRANSPORT_SDIO_AF";
case 5: return "TRANSPORT_SDIO_D0_PORT";
case 6: return "TRANSPORT_SDIO_D0_PIN";
case 7: return "TRANSPORT_SDIO_D1_PORT";
case 8: return "TRANSPORT_SDIO_D1_PIN";
case 9: return "TRANSPORT_SDIO_D2_PORT";
case 10: return "TRANSPORT_SDIO_D2_PIN";
case 11: return "TRANSPORT_SDIO_D3_PORT";
case 12: return "TRANSPORT_SDIO_D3_PIN";
case 13: return "TRANSPORT_SDIO_CK_PORT";
case 14: return "TRANSPORT_SDIO_CK_PIN";
case 15: return "TRANSPORT_SDIO_CMD_PORT";
case 16: return "TRANSPORT_SDIO_CMD_PIN";
case 17: return "TRANSPORT_SDIO_PHASE";
default: return "Unknown";
}
};

char* get_module_adc_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "MODULE_ADC_INTERVAL";
case 2: return "MODULE_ADC_SAMPLE_COUNT_PER_CHANNEL";
case 3: return "MODULE_ADC_DMA_UNIT";
case 4: return "MODULE_ADC_DMA_STREAM";
case 5: return "MODULE_ADC_DMA_CHANNEL";
case 6: return "MODULE_ADC_PHASE";
default: return "Unknown";
}
};

char* get_storage_eeprom_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "STORAGE_EEPROM_TRANSPORT_INDEX";
case 2: return "STORAGE_EEPROM_TRANSPORT_ADDRESS";
case 3: return "STORAGE_EEPROM_PAGE_SIZE";
case 4: return "STORAGE_EEPROM_SIZE";
case 5: return "STORAGE_EEPROM_PHASE";
default: return "Unknown";
}
};

char* get_storage_w25_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "STORAGE_W25_SPI_INDEX";
case 2: return "STORAGE_W25_PAGE_SIZE";
case 3: return "STORAGE_W25_SIZE";
case 4: return "STORAGE_W25_PHASE";
default: return "Unknown";
}
};

char* get_storage_flash_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "STORAGE_FLASH_START_ADDRESS";
case 2: return "STORAGE_FLASH_PAGE_SIZE";
case 3: return "STORAGE_FLASH_SIZE";
case 4: return "STORAGE_FLASH_PHASE";
default: return "Unknown";
}
};

char* get_memory_sram_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "MEMORY_SRAM_DISABLED";
case 2: return "MEMORY_SRAM_TRANSPORT_INDEX";
case 3: return "MEMORY_SRAM_TRANSPORT_ADDRESS";
case 4: return "MEMORY_SRAM_PAGE_SIZE";
case 5: return "MEMORY_SRAM_SIZE";
case 6: return "MEMORY_SRAM_PHASE";
default: return "Unknown";
}
};

char* get_storage_at24c_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "STORAGE_AT24C_I2C_INDEX";
case 2: return "STORAGE_AT24C_I2C_ADDRESS";
case 3: return "STORAGE_AT24C_START_ADDRESS";
case 4: return "STORAGE_AT24C_PAGE_SIZE";
case 5: return "STORAGE_AT24C_SIZE";
case 6: return "STORAGE_AT24C_PHASE";
default: return "Unknown";
}
};

char* get_storage_sdcard_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "STORAGE_SDCARD_SDIO_INDEX";
case 2: return "STORAGE_SDCARD_FS_READ_SIZE";
case 3: return "STORAGE_SDCARD_FS_PROGRAM_SIZE";
case 4: return "STORAGE_SDCARD_FS_BLOCK_CYCLES";
case 5: return "STORAGE_SDCARD_FS_CACHE_SIZE";
case 6: return "STORAGE_SDCARD_FS_LOOKAHEAD_SIZE";
case 7: return "STORAGE_SDCARD_FS_NAME_MAX_SIZE";
case 8: return "STORAGE_SDCARD_FS_FILE_MAX_SIZE";
case 9: return "STORAGE_SDCARD_FS_ATTR_MAX_SIZE";
case 10: return "STORAGE_SDCARD_FS_METADATA_MAX_SIZE";
case 11: return "STORAGE_SDCARD_FS_VOLUME_NAME";
case 12: return "STORAGE_SDCARD_PHASE";
case 13: return "STORAGE_SDCARD_CAPACITY";
case 14: return "STORAGE_SDCARD_BLOCK_SIZE";
case 15: return "STORAGE_SDCARD_BLOCK_COUNT";
case 16: return "STORAGE_SDCARD_MAX_BUS_CLOCK_FREQUENCY";
case 17: return "STORAGE_SDCARD_CSD_VERSION";
case 18: return "STORAGE_SDCARD_RELATIVE_CARD_ADDRESS";
case 19: return "STORAGE_SDCARD_MANUFACTURER_ID";
case 20: return "STORAGE_SDCARD_OEM_ID";
case 21: return "STORAGE_SDCARD_PRODUCT_NAME";
case 22: return "STORAGE_SDCARD_PRODUCT_REVISION";
case 23: return "STORAGE_SDCARD_SERIAL_NUMBER";
case 24: return "STORAGE_SDCARD_MANUFACTURING_DATE";
case 25: return "STORAGE_SDCARD_VERSION";
case 26: return "STORAGE_SDCARD_HIGH_CAPACITY";
default: return "Unknown";
}
};

char* get_input_sensor_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "INPUT_SENSOR_DISABLED";
case 2: return "INPUT_SENSOR_PORT";
case 3: return "INPUT_SENSOR_PIN";
case 4: return "INPUT_SENSOR_ADC_INDEX";
case 5: return "INPUT_SENSOR_ADC_CHANNEL";
case 6: return "INPUT_SENSOR_PHASE";
default: return "Unknown";
}
};

char* get_control_touchscreen_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "CONTROL_TOUCHSCREEN_SPI_INDEX";
case 2: return "CONTROL_TOUCHSCREEN_DC_PORT";
case 3: return "CONTROL_TOUCHSCREEN_DC_PIN";
case 4: return "CONTROL_TOUCHSCREEN_CS_PORT";
case 5: return "CONTROL_TOUCHSCREEN_CS_PIN";
case 6: return "CONTROL_TOUCHSCREEN_BUSY_PORT";
case 7: return "CONTROL_TOUCHSCREEN_BUSY_PIN";
case 8: return "CONTROL_TOUCHSCREEN_RESET_PORT";
case 9: return "CONTROL_TOUCHSCREEN_RESET_PIN";
case 10: return "CONTROL_TOUCHSCREEN_WIDTH";
case 11: return "CONTROL_TOUCHSCREEN_HEIGHT";
case 12: return "CONTROL_TOUCHSCREEN_MODE";
case 13: return "CONTROL_TOUCHSCREEN_PHASE";
default: return "Unknown";
}
};

char* get_screen_epaper_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "SCREEN_EPAPER_SPI_INDEX";
case 2: return "SCREEN_EPAPER_DC_PORT";
case 3: return "SCREEN_EPAPER_DC_PIN";
case 4: return "SCREEN_EPAPER_CS_PORT";
case 5: return "SCREEN_EPAPER_CS_PIN";
case 6: return "SCREEN_EPAPER_BUSY_PORT";
case 7: return "SCREEN_EPAPER_BUSY_PIN";
case 8: return "SCREEN_EPAPER_RESET_PORT";
case 9: return "SCREEN_EPAPER_RESET_PIN";
case 10: return "SCREEN_EPAPER_WIDTH";
case 11: return "SCREEN_EPAPER_HEIGHT";
case 12: return "SCREEN_EPAPER_MODE";
case 13: return "SCREEN_EPAPER_PHASE";
default: return "Unknown";
}
};

char* get_actor_mothership_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "ACTOR_MOTHERSHIP_TIMER_INDEX";
case 2: return "ACTOR_MOTHERSHIP_STORAGE_INDEX";
case 3: return "ACTOR_MOTHERSHIP_MCU_INDEX";
case 4: return "ACTOR_MOTHERSHIP_CANOPEN_INDEX";
case 5: return "ACTOR_MOTHERSHIP_PHASE";
default: return "Unknown";
}
};

char* get_screen_ili9341_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "SCREEN_ILI9341_WIDTH";
case 2: return "SCREEN_ILI9341_HEIGHT";
case 3: return "SCREEN_ILI9341_PHASE";
case 4: return "SCREEN_ILI9341_ORIENTATION";
case 5: return "SCREEN_ILI9341_INVERT_COLORS";
case 6: return "SCREEN_ILI9341_BACKLIGHT";
default: return "Unknown";
}
};

char* get_indicator_led_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "INDICATOR_LED_PORT";
case 2: return "INDICATOR_LED_PIN";
case 3: return "INDICATOR_LED_PHASE";
case 4: return "INDICATOR_LED_DUTY_CYCLE";
default: return "Unknown";
}
};

char* get_signal_beeper_properties_indecies_name (uint32_t v) {
switch (v) {
case 1: return "SIGNAL_BEEPER_PORT";
case 2: return "SIGNAL_BEEPER_PIN";
case 3: return "SIGNAL_BEEPER_PHASE";
case 4: return "SIGNAL_BEEPER_DUTY_CYCLE";
case 5: return "SIGNAL_BEEPER_TEST_Z";
default: return "Unknown";
}
};

char* get_actor_type_name (uint32_t v) {
switch (v) {
case 25248: return "TRANSPORT_SDIO";
case 12288: return "ACTOR_MOTHERSHIP";
case 16384: return "DEVICE_CIRCUIT";
case 24576: return "MODULE_MCU";
case 24608: return "ACTOR_CANOPEN";
case 24704: return "ACTOR_DATABASE";
case 24832: return "MODULE_TIMER";
case 25088: return "TRANSPORT_CAN";
case 25120: return "TRANSPORT_SPI";
case 25152: return "TRANSPORT_USART";
case 25184: return "TRANSPORT_I2C";
case 25216: return "TRANSPORT_MODBUS";
case 25344: return "MODULE_ADC";
case 28672: return "STORAGE_EEPROM";
case 28928: return "STORAGE_W25";
case 29184: return "STORAGE_FLASH";
case 29440: return "MEMORY_SRAM";
case 29696: return "STORAGE_AT24C";
case 29952: return "STORAGE_SDCARD";
case 32768: return "INPUT_SENSOR";
case 33024: return "CONTROL_TOUCHSCREEN";
case 36864: return "SCREEN_EPAPER";
case 37120: return "SCREEN_ILI9341";
case 38912: return "INDICATOR_LED";
case 39168: return "SIGNAL_BEEPER";
default: return "Unknown";
}
};

