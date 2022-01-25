/*******************************************************************************
    CANopen Object Dictionary definition for CANopenNode V4

    This file was automatically generated by CANopenEditor v4.0-51-g2d9b1ad

    https://github.com/CANopenNode/CANopenNode
    https://github.com/CANopenNode/CANopenEditor

    DON'T EDIT THIS FILE MANUALLY, UNLESS YOU KNOW WHAT YOU ARE DOING !!!!
*******************************************************************************/

#define OD_DEFINITION
#include "301/CO_ODinterface.h"
#include "Base_F1.h"

#if CO_VERSION_MAJOR < 4
#error This Object dictionary is compatible with CANopenNode V4.0 and above!
#endif

/*******************************************************************************
    OD data initialization of all groups
*******************************************************************************/
Base_F1_ATTR_RAM Base_F1_RAM_t Base_F1_RAM = {
    .x6000_moduleMCU = {
        .highestSub_indexSupported = 0x03,
        .family = {'S', 'T', 'M', '3', '2', 'F', '1', 0},
        .board = {'S', 'T', 'M', '3', '2', 'F', '1', '0', '3', 0}
    },
    .x6100_moduleCAN_1 = {
        .highestSub_indexSupported = 0x05,
        .disabled = 0x00000000,
        .TX_Port = 0x01,
        .TX_Pin = 0x0C,
        .RX_Port = 0x01,
        .RX_Pin = 0x0B
    }
};



/*******************************************************************************
    All OD objects (constant definitions)
*******************************************************************************/
typedef struct {
    OD_obj_record_t o_6000_moduleMCU[3];
    OD_obj_record_t o_6100_moduleCAN_1[6];
} Base_F1Objs_t;

static CO_PROGMEM Base_F1Objs_t Base_F1Objs = {
    .o_6000_moduleMCU = {
        {
            .dataOrig = &Base_F1_RAM.x6000_moduleMCU.highestSub_indexSupported,
            .subIndex = 0,
            .attribute = ODA_SDO_R,
            .dataLength = 1
        },
        {
            .dataOrig = &Base_F1_RAM.x6000_moduleMCU.family[0],
            .subIndex = 2,
            .attribute = ODA_SDO_RW | ODA_STR,
            .dataLength = 7
        },
        {
            .dataOrig = &Base_F1_RAM.x6000_moduleMCU.board[0],
            .subIndex = 3,
            .attribute = ODA_SDO_RW | ODA_STR,
            .dataLength = 9
        }
    },
    .o_6100_moduleCAN_1 = {
        {
            .dataOrig = &Base_F1_RAM.x6100_moduleCAN_1.highestSub_indexSupported,
            .subIndex = 0,
            .attribute = ODA_SDO_R,
            .dataLength = 1
        },
        {
            .dataOrig = &Base_F1_RAM.x6100_moduleCAN_1.disabled,
            .subIndex = 1,
            .attribute = ODA_SDO_RW | ODA_MB,
            .dataLength = 4
        },
        {
            .dataOrig = &Base_F1_RAM.x6100_moduleCAN_1.TX_Port,
            .subIndex = 2,
            .attribute = ODA_SDO_RW,
            .dataLength = 1
        },
        {
            .dataOrig = &Base_F1_RAM.x6100_moduleCAN_1.TX_Pin,
            .subIndex = 3,
            .attribute = ODA_SDO_RW,
            .dataLength = 1
        },
        {
            .dataOrig = &Base_F1_RAM.x6100_moduleCAN_1.RX_Port,
            .subIndex = 4,
            .attribute = ODA_SDO_RW,
            .dataLength = 1
        },
        {
            .dataOrig = &Base_F1_RAM.x6100_moduleCAN_1.RX_Pin,
            .subIndex = 5,
            .attribute = ODA_SDO_RW,
            .dataLength = 1
        }
    }
};


/*******************************************************************************
    Object dictionary
*******************************************************************************/
static Base_F1_ATTR_OD OD_entry_t Base_F1List[] = {
    {0x6000, 0x03, ODT_REC, &Base_F1Objs.o_6000_moduleMCU, NULL},
    {0x6100, 0x06, ODT_REC, &Base_F1Objs.o_6100_moduleCAN_1, NULL},
    {0x0000, 0x00, 0, NULL, NULL}
};

static OD_t _Base_F1 = {
    (sizeof(Base_F1List) / sizeof(Base_F1List[0])) - 1,
    &Base_F1List[0]
};

OD_t *Base_F1 = &_Base_F1;