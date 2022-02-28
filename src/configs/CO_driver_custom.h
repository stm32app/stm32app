#ifndef CO_CUSTOM_CONFIG
#define CO_CUSTOM_CONFIG

#include <app_env.h>
#include "core/types.h"

#define NMT_CONTROL                                                                                                                        \
    CO_NMT_STARTUP_TO_OPERATIONAL                                                                                                          \
    | CO_NMT_ERR_ON_ERR_REG | CO_ERR_REG_GENERIC_ERR | CO_ERR_REG_COMMUNICATION
#define FIRST_HB_TIME 501
#define SDO_SRV_TIMEOUT_TIME 1000
#define SDO_CLI_TIMEOUT_TIME 500
#define SDO_CLI_BLOCK true
#define OD_STATUS_BITS NULL

#define CO_GET_CNT(obj) OD_CNT_##obj
#define OD_GET(entry, index) OD_ENTRY_##entry

#endif