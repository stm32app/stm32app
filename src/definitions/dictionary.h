typedef struct transport_sdio transport_sdio_t /* Secure input/output protocol*/;
typedef struct actor_mothership actor_mothership_t /* Configuration of global object*/;
typedef struct device_circuit device_circuit_t /* A relay that turns circuit on and off (pwm optional */;
typedef struct module_mcu module_mcu_t /* */;
typedef struct actor_canopen actor_canopen_t /* CANOpen framework*/;
typedef struct actor_database actor_database_t /* SQLite instance*/;
typedef struct module_timer module_timer_t /* Basic 16-bit timer with low power consumption.*/;
typedef struct transport_can transport_can_t /* */;
typedef struct transport_spi transport_spi_t /* ADC Unit used for high-volume sampling of analog signals*/;
typedef struct transport_usart transport_usart_t /* Serial protocol*/;
typedef struct transport_i2c transport_i2c_t /* Serial protocol*/;
typedef struct transport_modbus transport_modbus_t /* Modbus over USART powered by DMA*/;
typedef struct module_adc module_adc_t /* ADC Unit used for high-volume sampling of analog signals*/;
typedef struct storage_eeprom storage_eeprom_t /* */;
typedef struct storage_w25 storage_w25_t /* Winbond flash storage device over SPI*/;
typedef struct storage_flash storage_flash_t /* Internal flash storage*/;
typedef struct memory_sram memory_sram_t /* */;
typedef struct storage_at24c storage_at24c_t /* I2C-based EEPROM*/;
typedef struct storage_sdcard storage_sdcard_t /* SDIO-based SDcard*/;
typedef struct input_sensor input_sensor_t /* A sensor that measures a single analog value (i.e. current meter, tank level meter)*/;
typedef struct control_touchscreen control_touchscreen_t /* */;
typedef struct screen_epaper screen_epaper_t /* E-ink screen with low power consumption and low update frequency*/;
typedef struct indicator_led indicator_led_t /* */;
typedef struct signal_beeper signal_beeper_t /* */;

enum actor_type {
    TRANSPORT_SDIO = 0x62A0, /* Secure input/output protocol*/
    ACTOR_MOTHERSHIP = 0x3000, /* Configuration of global object*/
    DEVICE_CIRCUIT = 0x4000, /* A relay that turns circuit on and off (pwm optional */
    MODULE_MCU = 0x6000, /* */
    ACTOR_CANOPEN = 0x6020, /* CANOpen framework*/
    ACTOR_DATABASE = 0x6080, /* SQLite instance*/
    MODULE_TIMER = 0x6100, /* Basic 16-bit timer with low power consumption.*/
    TRANSPORT_CAN = 0x6200, /* */
    TRANSPORT_SPI = 0x6220, /* ADC Unit used for high-volume sampling of analog signals*/
    TRANSPORT_USART = 0x6240, /* Serial protocol*/
    TRANSPORT_I2C = 0x6260, /* Serial protocol*/
    TRANSPORT_MODBUS = 0x6280, /* Modbus over USART powered by DMA*/
    MODULE_ADC = 0x6300, /* ADC Unit used for high-volume sampling of analog signals*/
    STORAGE_EEPROM = 0x7000, /* */
    STORAGE_W25 = 0x7100, /* Winbond flash storage device over SPI*/
    STORAGE_FLASH = 0x7200, /* Internal flash storage*/
    MEMORY_SRAM = 0x7300, /* */
    STORAGE_AT24C = 0x7400, /* I2C-based EEPROM*/
    STORAGE_SDCARD = 0x7500, /* SDIO-based SDcard*/
    INPUT_SENSOR = 0x8000, /* A sensor that measures a single analog value (i.e. current meter, tank level meter)*/
    CONTROL_TOUCHSCREEN = 0x8100, /* */
    SCREEN_EPAPER = 0x9000, /* E-ink screen with low power consumption and low update frequency*/
    INDICATOR_LED = 0x9800, /* */
    SIGNAL_BEEPER = 0x9900, /* */
};

