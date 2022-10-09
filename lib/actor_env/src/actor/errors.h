#ifndef INC_ACTOR_ERRORS
#define INC_ACTOR_ERRORS

/**
 * CANopen Error code
 *
 * Standard error codes according to CiA DS-301 and DS-401.
 */
typedef enum {
    /** 0x10xx, Generic Error */
    ACTOR_ERROR_GENERIC = 0x1000U,
    /** 0x20xx, Current */
    ACTOR_ERROR_CURRENT = 0x2000U,
    /** 0x21xx, Current, device input side */
    ACTOR_ERROR_CURRENT_INPUT = 0x2100U,
    /** 0x22xx, Current inside the device */
    ACTOR_ERROR_CURRENT_INSIDE = 0x2200U,
    /** 0x23xx, Current, device output side */
    ACTOR_ERROR_CURRENT_OUTPUT = 0x2300U,
    /** 0x30xx, Voltage */
    ACTOR_ERROR_VOLTAGE = 0x3000U,
    /** 0x31xx, Mains Voltage */
    ACTOR_ERROR_VOLTAGE_MAINS = 0x3100U,
    /** 0x32xx, Voltage inside the device */
    ACTOR_ERROR_VOLTAGE_INSIDE = 0x3200U,
    /** 0x33xx, Output Voltage */
    ACTOR_ERROR_VOLTAGE_OUTPUT = 0x3300U,
    /** 0x40xx, Temperature */
    ACTOR_ERROR_TEMPERATURE = 0x4000U,
    /** 0x41xx, Ambient Temperature */
    ACTOR_ERROR_TEMP_AMBIENT = 0x4100U,
    /** 0x42xx, Device Temperature */
    ACTOR_ERROR_TEMP_DEVICE = 0x4200U,
    /** 0x50xx, Device Hardware */
    ACTOR_ERROR_HARDWARE = 0x5000U,
    /** 0x60xx, Device Software */
    ACTOR_ERROR_SOFTWARE_DEVICE = 0x6000U,
    /** 0x61xx, Internal Software */
    ACTOR_ERROR_SOFTWARE_INTERNAL = 0x6100U,
    /** 0x62xx, User Software */
    ACTOR_ERROR_SOFTWARE_USER = 0x6200U,
    /** 0x63xx, Data Set */
    ACTOR_ERROR_DATA_SET = 0x6300U,
    /** 0x70xx, Additional Modules */
    ACTOR_ERROR_ADDITIONAL_MODUL = 0x7000U,
    /** 0x80xx, Monitoring */
    ACTOR_ERROR_MONITORING = 0x8000U,
    /** 0x81xx, Communication */
    ACTOR_ERROR_COMMUNICATION = 0x8100U,
    /** 0x8110, CAN Overrun (Objects lost) */
    ACTOR_ERROR_CAN_OVERRUN = 0x8110U,
    /** 0x8120, CAN in Error Passive Mode */
    ACTOR_ERROR_CAN_PASSIVE = 0x8120U,
    /** 0x8130, Life Guard Error or Heartbeat Error */
    ACTOR_ERROR_HEARTBEAT = 0x8130U,
    /** 0x8140, recovered from bus off */
    ACTOR_ERROR_BUS_OFF_RECOVERED = 0x8140U,
    /** 0x8150, CAN-ID collision */
    ACTOR_ERROR_CAN_ID_COLLISION = 0x8150U,
    /** 0x82xx, Protocol Error */
    ACTOR_ERROR_PROTOCOL_ERROR = 0x8200U,
    /** 0x8210, PDO not processed due to length error */
    ACTOR_ERROR_PDO_LENGTH = 0x8210U,
    /** 0x8220, PDO length exceeded */
    ACTOR_ERROR_PDO_LENGTH_EXC = 0x8220U,
    /** 0x8230, DAM MPDO not processed, destination object not available */
    ACTOR_ERROR_DAM_MPDO = 0x8230U,
    /** 0x8240, Unexpected SYNC data length */
    ACTOR_ERROR_SYNC_DATA_LENGTH = 0x8240U,
    /** 0x8250, RPDO timeout */
    ACTOR_ERROR_RPDO_TIMEOUT = 0x8250U,
    /** 0x90xx, External Error */
    ACTOR_ERROR_EXTERNAL_ERROR = 0x9000U,
    /** 0xF0xx, Additional Functions */
    ACTOR_ERROR_ADDITIONAL_FUNC = 0xF000U,
    /** 0xFFxx, Device specific */
    ACTOR_ERROR_DEVICE_SPECIFIC = 0xFF00U,

    /** 0x2310, DS401, Current at outputs too high (overload) */
    ACTOR_ERROR401_OUT_CUR_HI = 0x2310U,
    /** 0x2320, DS401, Short circuit at outputs */
    ACTOR_ERROR401_OUT_SHORTED = 0x2320U,
    /** 0x2330, DS401, Load dump at outputs */
    ACTOR_ERROR401_OUT_LOAD_DUMP = 0x2330U,
    /** 0x3110, DS401, Input voltage too high */
    ACTOR_ERROR401_IN_VOLT_HI = 0x3110U,
    /** 0x3120, DS401, Input voltage too low */
    ACTOR_ERROR401_IN_VOLT_LOW = 0x3120U,
    /** 0x3210, DS401, Internal voltage too high */
    ACTOR_ERROR401_INTERN_VOLT_HI = 0x3210U,
    /** 0x3220, DS401, Internal voltage too low */
    ACTOR_ERROR401_INTERN_VOLT_LO = 0x3220U,
    /** 0x3310, DS401, Output voltage too high */
    ACTOR_ERROR401_OUT_VOLT_HIGH = 0x3310U,
    /** 0x3320, DS401, Output voltage too low */
    ACTOR_ERROR401_OUT_VOLT_LOW = 0x3320U,
} ACTOR_ERROR_errorCode_t;


/**
 * Error status bits
 *
 * Bits for internal indication of the error condition. Each error condition is
 * specified by unique index from 0x00 up to 0xFF.
 *
 * If specific error occurs in the stack or in the application, CO_errorReport()
 * sets specific bit in the _errorStatusBit_ variable from @ref ACTOR_ERROR_t. If bit
 * was already set, function returns without any action. Otherwise it prepares
 * emergency message.
 *
 * Maximum size (in bits) of the _errorStatusBit_ variable is specified by
 * @ref CO_CONFIG_EM_ERR_STATUS_BITS_COUNT (set to 10*8 bits by default). Stack
 * uses first 6 bytes. Additional 4 bytes are pre-defined for manufacturer
 * or device specific error indications, by default.
 */
typedef enum {
    /** 0x00, Error Reset or No Error */
    ACTOR_ERROR_NO_ERROR                  = 0x00U,
    /** 0x01, communication, info, CAN bus warning limit reached */
    ACTOR_ERROR_CAN_BUS_WARNING           = 0x01U,
    /** 0x02, communication, info, Wrong data length of the received CAN
     * message */
    ACTOR_ERROR_RXMSG_WRONG_LENGTH        = 0x02U,
    /** 0x03, communication, info, Previous received CAN message wasn't
     * processed yet */
    ACTOR_ERROR_RXMSG_OVERFLOW            = 0x03U,
    /** 0x04, communication, info, Wrong data length of received PDO */
    ACTOR_ERROR_RPDO_WRONG_LENGTH         = 0x04U,
    /** 0x05, communication, info, Previous received PDO wasn't processed yet */
    ACTOR_ERROR_RPDO_OVERFLOW             = 0x05U,
    /** 0x06, communication, info, CAN receive bus is passive */
    ACTOR_ERROR_CAN_RX_BUS_PASSIVE        = 0x06U,
    /** 0x07, communication, info, CAN transmit bus is passive */
    ACTOR_ERROR_CAN_TX_BUS_PASSIVE        = 0x07U,
    /** 0x08, communication, info, Wrong NMT command received */
    ACTOR_ERROR_NMT_WRONG_COMMAND         = 0x08U,
    /** 0x09, communication, info, TIME message timeout */
    ACTOR_ERROR_TIME_TIMEOUT              = 0x09U,

    /** 0x12, communication, critical, CAN transmit bus is off */
    ACTOR_ERROR_CAN_TX_BUS_OFF            = 0x12U,
    /** 0x13, communication, critical, CAN module receive buffer has
     * overflowed */
    ACTOR_ERROR_CAN_RXB_OVERFLOW          = 0x13U,
    /** 0x14, communication, critical, CAN transmit buffer has overflowed */
    ACTOR_ERROR_CAN_TX_OVERFLOW           = 0x14U,
    /** 0x15, communication, critical, TPDO is outside SYNC window */
    ACTOR_ERROR_TPDO_OUTSIDE_WINDOW       = 0x15U,
    /** 0x16, communication, critical, (unused) */
    ACTOR_ERROR_16_unused                 = 0x16U,
    /** 0x17, communication, critical, RPDO message timeout */
    ACTOR_ERROR_RPDO_TIME_OUT             = 0x17U,
    /** 0x18, communication, critical, SYNC message timeout */
    ACTOR_ERROR_SYNC_TIME_OUT             = 0x18U,
    /** 0x19, communication, critical, Unexpected SYNC data length */
    ACTOR_ERROR_SYNC_LENGTH               = 0x19U,
    /** 0x1A, communication, critical, Error with PDO mapping */
    ACTOR_ERROR_PDO_WRONG_MAPPING         = 0x1AU,
    /** 0x1B, communication, critical, Heartbeat consumer timeout */
    ACTOR_ERROR_HEARTBEAT_CONSUMER        = 0x1BU,
    /** 0x1C, communication, critical, Heartbeat consumer detected remote node
     * reset */
    ACTOR_ERROR_HB_CONSUMER_REMOTE_RESET  = 0x1CU,

    /** 0x20, generic, info, Emergency buffer is full, Emergency message wasn't
     * sent */
    ACTOR_ERROR_EMERGENCY_BUFFER_FULL     = 0x20U,
    /** 0x21, generic, info, (unused) */
    ACTOR_ERROR_21_unused                 = 0x21U,
    /** 0x22, generic, info, Microcontroller has just started */
    ACTOR_ERROR_MICROCONTROLLER_RESET     = 0x22U,
    /** 0x27, generic, info, Automatic store to non-volatile memory failed */
    ACTOR_ERROR_NON_VOLATILE_AUTO_SAVE    = 0x27U,

    /** 0x28, generic, critical, Wrong parameters to CO_errorReport() function*/
    ACTOR_ERROR_WRONG_ERROR_REPORT        = 0x28U,
    /** 0x29, generic, critical, Timer task has overflowed */
    ACTOR_ERROR_ISR_TIMER_OVERFLOW        = 0x29U,
    /** 0x2A, generic, critical, Unable to allocate memory for objects */
    ACTOR_ERROR_MEMORY_ALLOCATION_ERROR   = 0x2AU,
    /** 0x2B, generic, critical, Generic error, test usage */
    ACTOR_ERROR_GENERIC_ERROR             = 0x2BU,
    /** 0x2C, generic, critical, Software error */
    ACTOR_ERROR_GENERIC_SOFTWARE_ERROR    = 0x2CU,
    /** 0x2D, generic, critical, Object dictionary does not match the software*/
    ACTOR_ERROR_INCONSISTENT_OBJECT_DICT  = 0x2DU,
    /** 0x2E, generic, critical, Error in calculation of device parameters */
    ACTOR_ERROR_CALCULATION_OF_PARAMETERS = 0x2EU,
    /** 0x2F, generic, critical, Error with access to non volatile device memory
     */
    ACTOR_ERROR_NON_VOLATILE_MEMORY       = 0x2FU,

    /** 0x30+, manufacturer, info or critical, Error status buts, free to use by
     * manufacturer. By default bits 0x30..0x3F are set as informational and
     * bits 0x40..0x4F are set as critical. Manufacturer critical bits sets the
     * error register, as specified by @ref CO_CONFIG_ERR_CONDITION_MANUFACTURER
     */
    ACTOR_ERROR_MANUFACTURER_START        = 0x30U,
    /** (@ref CO_CONFIG_EM_ERR_STATUS_BITS_COUNT - 1), largest value of the
     * Error status bit. */
    //ACTOR_ERROR_MANUFACTURER_END          = CO_CONFIG_EM_ERR_STATUS_BITS_COUNT - 1
} ACTOR_ERROR_errorStatusBits_t;

#endif