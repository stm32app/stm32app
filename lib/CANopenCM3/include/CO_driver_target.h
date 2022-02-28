/*
 * Device and application specific definitions for CANopenNode.
 *
 * @file        CO_driver_target.h
 * @author      Yaroslaff Fedin
 * @copyright   2021 Yaroslaff Fedin
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CO_DRIVER_TARGET_H
#define CO_DRIVER_TARGET_H

/* This file contains actor and application specific definitions.
 * It is included from CO_driver.h, which contains documentation
 * for common definitions below. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define bool_t bool
#include <libopencm3/stm32/can.h>

#ifdef CO_DRIVER_CUSTOM
#include "CO_driver_custom.h"
#endif


/* Print debug info from some internal parts of the stack */

/*#if (CO_CONFIG_DEBUG) & CO_CONFIG_DEBUG_COMMON
 #include <stdio.h>
 #include <syslog.h>
 #define CO_DEBUG_COMMON(msg) printf(LOG_DEBUG, DBG_CO_DEBUG, msg);
 #endif*/

#ifndef CO_CONFIG_CRC16
#define CO_CONFIG_CRC16 (CO_CONFIG_CRC16_ENABLE)
#endif

#ifndef CO_CONFIG_FIFO
#define CO_CONFIG_FIFO                                                                                                                     \
    (CO_CONFIG_FIFO_ENABLE | CO_CONFIG_FIFO_ALT_READ | CO_CONFIG_FIFO_CRC16_CCITT | CO_CONFIG_FIFO_ASCII_COMMANDS |                        \
     CO_CONFIG_FIFO_ASCII_DATATYPES)
#endif

/* Stack configuration override from CO_driver.h. Compile full stack.
 * For more information see file CO_config.h. */
#ifndef CO_CONFIG_NMT
#define CO_CONFIG_NMT (CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_NMT_CALLBACK_CHANGE | CO_CONFIG_NMT_MASTER)
#endif

#ifndef CO_CONFIG_SDO
#define CO_CONFIG_SDO (CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_SDO_SEGMENTED | CO_CONFIG_SDO_BLOCK)
#endif

#ifndef CO_CONFIG_SDO_SRV
#define CO_CONFIG_SDO_SRV                                                                                                                  \
    (CO_CONFIG_SDO_SRV_SEGMENTED | CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_SDO_SRV_BLOCK |                      \
     CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_SDO_SRV_BUFFER_SIZE
#define CO_CONFIG_SDO_SRV_BUFFER_SIZE 900
#endif

#ifndef CO_CONFIG_SDO_BUFFER_SIZE
#define CO_CONFIG_SDO_BUFFER_SIZE 1800
#endif

#ifndef CO_CONFIG_EM
#define CO_CONFIG_EM                                                                                                                       \
    (CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_EM_PRODUCER | CO_CONFIG_EM_HISTORY | CO_CONFIG_EM_CONSUMER |       \
     CO_CONFIG_EM_PROD_INHIBIT)
#endif

#ifndef CO_CONFIG_HB_CONS
#define CO_CONFIG_HB_CONS                                                                                                                  \
    (CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_HB_CONS_CALLBACK_CHANGE | CO_CONFIG_HB_CONS_CALLBACK_MULTI |       \
     CO_CONFIG_HB_CONS_QUERY_FUNCT)
#endif

#ifndef CO_CONFIG_PDO
#define CO_CONFIG_PDO                                                                                                                      \
    (CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_PDO_SYNC_ENABLE | CO_CONFIG_RPDO_CALLS_EXTENSION |                 \
     CO_CONFIG_TPDO_CALLS_EXTENSION | CO_CONFIG_TPDO_TIMERS_ENABLE | CO_CONFIG_TPDO_ENABLE)
#endif

#ifndef CO_CONFIG_SYNC
#define CO_CONFIG_SYNC (CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_SYNC_ENABLE)
#endif

#ifndef CO_CONFIG_SDO_CLI
#define CO_CONFIG_SDO_CLI                                                                                                                  \
    (CO_CONFIG_SDO_CLI_ENABLE | CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_SDO_CLI_SEGMENTED |                     \
     CO_CONFIG_SDO_CLI_BLOCK | CO_CONFIG_SDO_CLI_LOCAL)
#endif

#ifndef CO_CONFIG_SDO_CLI_BUFFER_SIZE
#define CO_CONFIG_SDO_CLI_BUFFER_SIZE 1000
#endif

#ifndef CO_CONFIG_TIME
#define CO_CONFIG_TIME (CO_CONFIG_FLAG_CALLBACK_PRE)
#endif

#ifndef CO_CONFIG_LEDS
#define CO_CONFIG_LEXDS (CO_CONFIG_FLAG_TIMERNEXT | CO_CONFIG_LEDS_ENABLE)
#endif

#ifndef CO_CONFIG_LSS
#define CO_CONFIG_LSS                                                                                                                      \
    (CO_CONFIG_FLAG_CALLBACK_PRE | CO_CONFIG_LSS_SLAVE | CO_CONFIG_LSS_SLAVE_FASTSCAN_DIRECT_RESPOND | CO_CONFIG_LSS_MASTER)
#endif

/*#ifndef CO_CONFIG_GTW
#define CO_CONFIG_GTW (CO_CONFIG_GTW_ASCII |            \
                       CO_CONFIG_GTW_ASCII_SDO |        \
                       CO_CONFIG_GTW_ASCII_NMT |        \
                       CO_CONFIG_GTW_ASCII_LSS |        \
                       CO_CONFIG_GTW_ASCII_LOG |        \
                       CO_CONFIG_GTW_ASCII_ERROR_DESC | \
                       CO_CONFIG_GTW_ASCII_PRINT_HELP | \
                       CO_CONFIG_GTW_ASCII_PRINT_LEDS)

#define CO_CONFIG_GTW_BLOCK_DL_LOOP 1
#define CO_CONFIG_GTWA_COMM_BUF_SIZE 2000
#define CO_CONFIG_GTWA_LOG_BUF_SIZE 2000
#endif*/

#ifdef __cplusplus
extern "C" {
#endif

/* Basic definitions. If big endian, CO_SWAP_xx macros must swap bytes. */
#define CO_LITTLE_ENDIAN
#define CO_SWAP_16(x) x
#define CO_SWAP_32(x) x
#define CO_SWAP_64(x) x
/* NULL is defined in stddef.h */
/* true and false are defined in stdbool.h */
/* int8_t to uint64_t are defined in stdint.h */
// typedef uint_fast8_t bool_t;
typedef float float32_t;
typedef double float64_t;

typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t data[8];
} CO_CANrxMsg_t;

/* Access to received CAN message */
/* Access to received CAN message */
#define CO_CANrxMsg_readIdent(msg) ((uint16_t)((CO_CANrxMsg_t *)msg)->ident)
#define CO_CANrxMsg_readDLC(msg) ((uint8_t)((CO_CANrxMsg_t *)msg)->DLC)
#define CO_CANrxMsg_readData(msg) ((uint8_t *)((CO_CANrxMsg_t *)msg)->data)

/* Received message object */
typedef struct {
    uint16_t ident;
    uint16_t mask;
    void *object;
    void (*CANrx_callback)(void *object, void *message);
} CO_CANrx_t;

/* Transmit message object */
typedef struct {
    uint32_t ident;
    uint8_t DLC;
    uint8_t data[8];
    bool rtr;
    volatile bool_t bufferFull;
    volatile bool_t syncFlag;
} CO_CANtx_t;

/* CAN module object */
typedef struct {
    void *CANptr;
    CO_CANrx_t *rxArray;
    uint16_t rxSize;
    CO_CANtx_t *txArray;
    uint16_t txSize;
    uint16_t CANerrorStatus;
    volatile bool_t CANnormal;
    volatile bool_t useCANrxFilters;
    volatile bool_t bufferInhibitFlag;
    volatile bool_t firstCANtxMessage;
    volatile uint16_t CANtxCount;
    uint32_t errOld;
    uint32_t port;
    uint8_t rxFifoIndex;
    uint16_t bitrate;
    uint16_t brp;
    uint8_t sjw;
    uint8_t prop;
    uint8_t ph_seg1;
    uint8_t ph_seg2;
} CO_CANmodule_t;

/* Data storage object for one entry */
typedef struct {
    size_t len;
    uint8_t subIndexOD;
    uint8_t attr;
    uint8_t index; // index within entries array
    void *addr;

    // eeprom specific
    void *storageModule;
    /** CRC checksum of the data stored in eeprom, set on store, required with
     * @ref CO_storage_eeprom. */
    uint16_t crc;
    /** Address of entry signature inside eeprom, set by init, required with
     * @ref CO_storage_eeprom. */
    size_t eepromAddrSignature;
    /** Address of data inside eeprom, set by init, required with
     * @ref CO_storage_eeprom. */
    size_t eepromAddr;
    /** Offset of next byte being updated by automatic storage, required with
     * @ref CO_storage_eeprom. */
    size_t offset;
    /** Additional target specific parameters, optional. */
    void *additionalParameters;

    // flash specific
    void *addrFlash;

} CO_storage_entry_t;

/* (un)lock critical section in CO_CANsend() */
#define CO_LOCK_CAN_SEND(CAN_MODULE)
#define CO_UNLOCK_CAN_SEND(CAN_MODULE)

/* (un)lock critical section in CO_errorReport() or CO_errorReset() */
#define CO_LOCK_EMCY(CAN_MODULE)
#define CO_UNLOCK_EMCY(CAN_MODULE)

/* (un)lock critical section when accessing Object Dictionary */
#define CO_LOCK_OD(CAN_MODULE)
#define CO_UNLOCK_OD(CAN_MODULE)

/* Synchronization between CAN receive and message processing threads. */
#define CO_MemoryBarrier()
#define CO_FLAG_READ(rxNew) ((rxNew) != NULL)
#define CO_FLAG_SET(rxNew)                                                                                                                 \
    {                                                                                                                                      \
        CO_MemoryBarrier();                                                                                                                \
        rxNew = (void *)1L;                                                                                                                \
    }
#define CO_FLAG_CLEAR(rxNew)                                                                                                               \
    {                                                                                                                                      \
        CO_MemoryBarrier();                                                                                                                \
        rxNew = NULL;                                                                                                                      \
    }

/* Function called from CAN receive interrupt handler */
void CO_CANRxInterrupt(CO_CANmodule_t *CANmodule);
void CO_CANTxInterrupt(CO_CANmodule_t *CANmodule);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_DRIVER_TARGET_H */
