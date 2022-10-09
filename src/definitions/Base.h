/*******************************************************************************
    CANopen Object Dictionary definition for CANopenNode V4

    This file was automatically generated by CANopenEditor v4.0-51-g2d9b1ad

    https://github.com/CANopenNode/CANopenNode
    https://github.com/CANopenNode/CANopenEditor

    DON'T EDIT THIS FILE MANUALLY !!!!
********************************************************************************

    File info:
        File Names:   OD.h; OD.c
        Project File: Base.xdd
        File Version: 1

        Created:      9/25/2021 2:03:07 AM
        Created By:   
        Modified:     3/12/2022 8:18:58 PM
        Modified By:  

    Device Info:
        Vendor Name:  2
        Vendor ID:    3
        Product Name: Base
        Product ID:   1

        Description:  
*******************************************************************************/

#ifndef OD_H
#define OD_H
/*******************************************************************************
    Counters of OD objects
*******************************************************************************/
#define OD_CNT_NMT 1
#define OD_CNT_EM 1
#define OD_CNT_SYNC 1
#define OD_CNT_SYNC_PROD 1
#define OD_CNT_STORAGE 1
#define OD_CNT_TIME 1
#define OD_CNT_EM_PROD 1
#define OD_CNT_HB_CONS 1
#define OD_CNT_HB_PROD 1
#define OD_CNT_SDO_SRV 1
#define OD_CNT_SDO_CLI 1
#define OD_CNT_RPDO 4
#define OD_CNT_TPDO 4
#define OD_CNT_DEVICE_CIRCUIT 1
#define OD_CNT_MODULE_TIMER 4
#define OD_CNT_TRANSPORT_CAN 1
#define OD_CNT_TRANSPORT_SPI 1
#define OD_CNT_TRANSPORT_USART 1
#define OD_CNT_TRANSPORT_I2C 3
#define OD_CNT_TRANSPORT_MODBUS 1
#define OD_CNT_MODULE_ADC 1
#define OD_CNT_STORAGE_EEPROM 1
#define OD_CNT_STORAGE_WINBOND 1
#define OD_CNT_STORAGE_FLASH 1
#define OD_CNT_MEMORY_SRAM 1
#define OD_CNT_STORAGE_AT24C 1
#define OD_CNT_INPUT_SENSOR 1
#define OD_CNT_CONTROL_TOUCHSCREEN 1
#define OD_CNT_SCREEN_EPAPER 1
#define OD_CNT_INDICATOR_LED 3


/*******************************************************************************
    Sizes of OD arrays
*******************************************************************************/
#define OD_CNT_ARR_1003 16
#define OD_CNT_ARR_1010 4
#define OD_CNT_ARR_1011 4
#define OD_CNT_ARR_1016 8


/*******************************************************************************
    OD data declaration of all groups
*******************************************************************************/
typedef struct {
    uint32_t x1000_deviceType;
    uint32_t x1005_COB_ID_SYNCMessage;
    uint32_t x1006_communicationCyclePeriod;
    uint32_t x1007_synchronousWindowLength;
    uint32_t x1012_COB_IDTimeStampObject;
    uint32_t x1014_COB_ID_EMCY;
    uint16_t x1015_inhibitTimeEMCY;
    uint8_t x1016_consumerHeartbeatTime_sub0;
    uint32_t x1016_consumerHeartbeatTime[OD_CNT_ARR_1016];
    uint16_t x1017_producerHeartbeatTime;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t vendor_ID;
        uint32_t productCode;
        uint32_t revisionNumber;
        uint32_t serialNumber;
    } x1018_identity;
    uint8_t x1019_synchronousCounterOverflowValue;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDClientToServerTx;
        uint32_t COB_IDServerToClientRx;
        uint8_t node_IDOfTheSDOServer;
    } x1280_SDOClientParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByRPDO;
        uint8_t transmissionType;
        uint16_t eventTimer;
    } x1400_RPDOCommunicationParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByRPDO;
        uint8_t transmissionType;
        uint16_t eventTimer;
    } x1401_RPDOCommunicationParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByRPDO;
        uint8_t transmissionType;
        uint16_t eventTimer;
    } x1402_RPDOCommunicationParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByRPDO;
        uint8_t transmissionType;
        uint16_t eventTimer;
    } x1403_RPDOCommunicationParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1600_RPDOMappingParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1601_RPDOMappingParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1602_RPDOMappingParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1603_RPDOMappingParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByTPDO;
        uint8_t transmissionType;
        uint16_t inhibitTime;
        uint16_t eventTimer;
        uint8_t SYNCStartValue;
    } x1800_TPDOCommunicationParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByTPDO;
        uint8_t transmissionType;
        uint16_t inhibitTime;
        uint16_t eventTimer;
        uint8_t SYNCStartValue;
    } x1801_TPDOCommunicationParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByTPDO;
        uint8_t transmissionType;
        uint16_t inhibitTime;
        uint16_t eventTimer;
        uint8_t SYNCStartValue;
    } x1802_TPDOCommunicationParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDUsedByTPDO;
        uint8_t transmissionType;
        uint16_t inhibitTime;
        uint16_t eventTimer;
        uint8_t SYNCStartValue;
    } x1803_TPDOCommunicationParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1A00_TPDOMappingParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1A01_TPDOMappingParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1A02_TPDOMappingParameter;
    struct {
        uint8_t numberOfMappedApplicationObjectsInPDO;
        uint32_t applicationObject_1;
        uint32_t applicationObject_2;
        uint32_t applicationObject_3;
        uint32_t applicationObject_4;
        uint32_t applicationObject_5;
        uint32_t applicationObject_6;
        uint32_t applicationObject_7;
        uint32_t applicationObject_8;
    } x1A03_TPDOMappingParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t port;
        uint8_t pin;
        uint16_t limitCurrent;
        uint16_t limitVoltage;
        uint16_t PSU_Index;
        uint16_t sensorIndex;
        uint8_t phase;
        uint16_t dutyCycle;
        uint16_t current;
        uint16_t voltage;
        uint8_t consumers;
    } x4000_deviceCircuit_1;
    struct {
        uint8_t highestSub_indexSupported;
        char family[8];
        char boardType[10];
        uint32_t storageIndex;
        uint8_t phase;
        int16_t CPU_Temperature;
        uint32_t startupTime;
    } x6000_moduleMCU;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t CAN_Index;
        uint8_t CAN_FIFO_Index;
        uint16_t greenLedIndex;
        uint16_t redLedIndex;
        uint16_t firstHB_Time;
        uint16_t SDO_ServerTimeout;
        uint16_t SDO_ClientTimeout;
        uint8_t phase;
        uint8_t nodeID;
        uint16_t bitrate;
    } x6020_actorCANopen;
    struct {
        uint8_t highestSub_indexSupported;
        int16_t storageIndex;
        uint32_t journalBufferSize;
        char path[11];
        uint8_t phase;
    } x6080_actorDatabase;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t prescaler;
        uint8_t initialSubscriptionsCount;
        uint32_t period;
        uint32_t frequency;
        uint8_t phase;
    } x6100_moduleTimer_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t prescaler;
        uint8_t initialSubscriptionsCount;
        uint32_t period;
        uint32_t frequency;
        uint8_t phase;
    } x6101_moduleTimer_2;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t prescaler;
        uint8_t initialSubscriptionsCount;
        uint32_t period;
        uint32_t frequency;
        uint8_t phase;
    } x6102_moduleTimer_3;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t prescaler;
        uint8_t initialSubscriptionsCount;
        uint32_t period;
        uint32_t frequency;
        uint8_t phase;
    } x6106_moduleTimer_7;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t TX_Port;
        uint8_t TX_Pin;
        uint8_t RX_Port;
        uint8_t RX_Pin;
        int16_t bitrate;
        uint16_t BRP;
        uint8_t SJW;
        uint8_t prop;
        uint8_t phSeg1;
        uint8_t phSeg2;
        uint8_t phase;
    } x6200_transportCAN_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t isSlave;
        uint8_t softwareSS_Control;
        uint8_t mode;
        uint8_t DMA_RxUnit;
        uint8_t DMA_RxStream;
        uint8_t DMA_RxChannel;
        uint32_t DMA_RxIdleTimeout;
        uint16_t DMA_RxCircularBufferSize;
        uint16_t rxPoolMaxSize;
        uint16_t rxPoolInitialSize;
        uint16_t rxPoolBlockSize;
        uint8_t DMA_TxUnit;
        uint8_t DMA_TxStream;
        uint8_t DMA_TxChannel;
        uint8_t AF_Index;
        uint8_t SS_Port;
        uint8_t SS_Pin;
        uint8_t SCK_Port;
        uint8_t SCK_Pin;
        uint8_t MISO_Port;
        uint8_t MISO_Pin;
        uint8_t MOSI_Port;
        uint8_t MOSI_Pin;
        uint8_t phase;
    } x6220_transportSPI_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t DMA_RxUnit;
        uint8_t DMA_RxStream;
        uint8_t DMA_RxChannel;
        uint8_t DMA_RxCircularBufferSize;
        uint8_t DMA_TxUnit;
        uint8_t DMA_TxStream;
        uint8_t DMA_TxChannel;
        uint32_t baudrate;
        uint8_t databits;
        uint8_t phase;
    } x6240_transportUSART_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t DMA_RxUnit;
        uint8_t DMA_RxStream;
        uint8_t DMA_RxChannel;
        uint8_t DMA_RxCircularBufferSize;
        uint16_t rxPoolMaxSize;
        uint16_t rxPoolInitialSize;
        uint16_t rxPoolBlockSize;
        uint8_t DMA_TxUnit;
        uint8_t DMA_TxStream;
        uint8_t DMA_TxChannel;
        uint8_t AF;
        uint8_t SMBA_Pin;
        uint8_t SMBA_Port;
        uint8_t SDA_Port;
        uint8_t SDA_Pin;
        uint8_t SCL_Port;
        uint8_t SCL_Pin;
        int8_t frequency;
        uint8_t databits;
        uint8_t phase;
        uint8_t slaveAddress;
    } x6260_transportI2C_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t USART_Index;
        uint8_t RTS_Port;
        uint8_t RTS_Pin;
        uint8_t slaveAddress;
        uint16_t timeout;
        uint8_t phase;
    } x6280_transportModbus_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t DMA_Unit;
        uint8_t DMA_Stream;
        uint8_t DMA_Channel;
        uint8_t AF;
        uint8_t D0_Port;
        uint8_t D0_Pin;
        uint8_t D1_Port;
        uint8_t D1_Pin;
        uint8_t D2_Port;
        uint8_t D2_Pin;
        uint8_t D3_Port;
        uint8_t D3_Pin;
        uint8_t CK_Port;
        uint8_t CK_Pin;
        uint8_t CMD_Port;
        uint8_t CMD_Pin;
        uint8_t phase;
    } x62A0_transportSDIO;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t interval;
        uint16_t sampleCountPerChannel;
        uint8_t DMA_Unit;
        uint8_t DMA_Stream;
        uint8_t DMA_Channel;
        uint8_t phase;
    } x6300_moduleADC_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t transportIndex;
        uint16_t transportAddress;
        uint16_t pageSize;
        uint16_t size;
        uint8_t phase;
    } x7000_storageEeprom_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t SPI_Index;
        uint16_t pageSize;
        uint16_t size;
        uint8_t phase;
    } x7100_storageW25;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t startAddress;
        uint16_t pageSize;
        uint16_t size;
        uint8_t phase;
    } x7200_storageFlash;
    struct {
        uint8_t highestSub_indexSupported;
        int16_t disabled;
        uint16_t transportIndex;
        uint16_t transportAddress;
        uint16_t pageSize;
        uint16_t size;
        uint8_t phase;
    } x7300_memorySRAM_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t I2C_Index;
        uint8_t I2C_Address;
        uint16_t startAddress;
        uint16_t pageSize;
        uint16_t size;
        uint8_t phase;
    } x7400_storageAT24C;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t SDIO_Index;
        uint32_t FS_ReadSize;
        uint32_t FS_ProgramSize;
        int32_t FS_BlockCycles;
        uint16_t FS_CacheSize;
        uint32_t FS_LookaheadSize;
        uint32_t FS_NameMaxSize;
        uint32_t FS_FileMaxSize;
        uint32_t FS_AttrMaxSize;
        uint32_t FS_MetadataMaxSize;
        char FS_VolumeName[17];
        uint8_t phase;
        uint32_t capacity;
        uint32_t blockSize;
        uint32_t blockCount;
        uint32_t maxBusClockFrequency;
        uint8_t CSD_Version;
        uint16_t relativeCardAddress;
        uint8_t manufacturerID;
        uint16_t OEM_ID;
        char productName[6];
        uint8_t productRevision;
        uint32_t serialNumber;
        uint16_t manufacturingDate;
        uint8_t version;
        bool_t highCapacity;
    } x7500_storageSDCard;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t disabled;
        uint8_t port;
        uint8_t pin;
        uint16_t ADC_Index;
        uint8_t ADC_Channel;
        uint8_t phase;
    } x8000_inputSensor_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t SPI_Index;
        uint8_t DC_Port;
        uint8_t DC_Pin;
        uint8_t CS_Port;
        uint8_t CS_Pin;
        uint8_t busyPort;
        uint8_t busyPin;
        uint8_t resetPort;
        uint8_t resetPin;
        uint16_t width;
        uint16_t height;
        uint16_t mode;
        uint8_t phase;
    } x8100_controlTouchscreen_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint16_t SPI_Index;
        uint8_t DC_Port;
        uint8_t DC_Pin;
        uint8_t CS_Port;
        uint8_t CS_Pin;
        uint8_t busyPort;
        uint8_t busyPin;
        uint8_t resetPort;
        uint8_t resetPin;
        uint16_t width;
        uint16_t height;
        uint16_t mode;
        uint8_t phase;
    } x9000_screenEpaper_1;
} OD_PERSIST_COMM_t;

typedef struct {
    uint8_t x1001_errorRegister;
    uint8_t x1010_storeParameters_sub0;
    uint32_t x1010_storeParameters[OD_CNT_ARR_1010];
    uint8_t x1011_restoreDefaultParameters_sub0;
    uint32_t x1011_restoreDefaultParameters[OD_CNT_ARR_1011];
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t COB_IDClientToServerRx;
        uint32_t COB_IDServerToClientTx;
    } x1200_SDOServerParameter;
    struct {
        uint8_t highestSub_indexSupported;
        uint32_t timerIndex;
        uint32_t storageIndex;
        uint32_t MCU_Index;
        uint32_t CANopenIndex;
        uint8_t phase;
    } x3000_actorMothership;
    struct {
        uint8_t highestSub_indexSupported;
        int16_t width;
        uint16_t height;
        uint8_t phase;
        uint8_t orientation;
        bool_t invertColors;
        bool_t backlight;
    } x9100_screenILI9341;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t port;
        uint8_t pin;
        uint8_t phase;
        uint8_t dutyCycle;
    } x9800_indicatorLED_1;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t port;
        uint8_t pin;
        uint8_t phase;
        uint8_t dutyCycle;
    } x9801_indicatorLED_2;
    struct {
        uint8_t highestSub_indexSupported;
        uint8_t port;
        uint8_t pin;
        uint8_t phase;
        uint8_t dutyCycle;
        uint8_t testZ;
    } x9900_signalBeeper_1;
} OD_RAM_t;

#ifndef OD_ATTR_PERSIST_COMM
#define OD_ATTR_PERSIST_COMM
#endif
extern OD_ATTR_PERSIST_COMM OD_PERSIST_COMM_t OD_PERSIST_COMM;

#ifndef OD_ATTR_RAM
#define OD_ATTR_RAM
#endif
extern OD_ATTR_RAM OD_RAM_t OD_RAM;

#ifndef OD_ATTR_OD
#define OD_ATTR_OD
#endif
extern OD_ATTR_OD OD_t *OD;


/*******************************************************************************
    Object dictionary entries - shortcuts
*******************************************************************************/
#define OD_ENTRY_H1000 &OD->list[0]
#define OD_ENTRY_H1001 &OD->list[1]
#define OD_ENTRY_H1003 &OD->list[2]
#define OD_ENTRY_H1005 &OD->list[3]
#define OD_ENTRY_H1006 &OD->list[4]
#define OD_ENTRY_H1007 &OD->list[5]
#define OD_ENTRY_H1010 &OD->list[6]
#define OD_ENTRY_H1011 &OD->list[7]
#define OD_ENTRY_H1012 &OD->list[8]
#define OD_ENTRY_H1014 &OD->list[9]
#define OD_ENTRY_H1015 &OD->list[10]
#define OD_ENTRY_H1016 &OD->list[11]
#define OD_ENTRY_H1017 &OD->list[12]
#define OD_ENTRY_H1018 &OD->list[13]
#define OD_ENTRY_H1019 &OD->list[14]
#define OD_ENTRY_H1200 &OD->list[15]
#define OD_ENTRY_H1280 &OD->list[16]
#define OD_ENTRY_H1400 &OD->list[17]
#define OD_ENTRY_H1401 &OD->list[18]
#define OD_ENTRY_H1402 &OD->list[19]
#define OD_ENTRY_H1403 &OD->list[20]
#define OD_ENTRY_H1600 &OD->list[21]
#define OD_ENTRY_H1601 &OD->list[22]
#define OD_ENTRY_H1602 &OD->list[23]
#define OD_ENTRY_H1603 &OD->list[24]
#define OD_ENTRY_H1800 &OD->list[25]
#define OD_ENTRY_H1801 &OD->list[26]
#define OD_ENTRY_H1802 &OD->list[27]
#define OD_ENTRY_H1803 &OD->list[28]
#define OD_ENTRY_H1A00 &OD->list[29]
#define OD_ENTRY_H1A01 &OD->list[30]
#define OD_ENTRY_H1A02 &OD->list[31]
#define OD_ENTRY_H1A03 &OD->list[32]
#define OD_ENTRY_H3000 &OD->list[33]
#define OD_ENTRY_H4000 &OD->list[34]
#define OD_ENTRY_H6000 &OD->list[35]
#define OD_ENTRY_H6020 &OD->list[36]
#define OD_ENTRY_H6080 &OD->list[37]
#define OD_ENTRY_H6100 &OD->list[38]
#define OD_ENTRY_H6101 &OD->list[39]
#define OD_ENTRY_H6102 &OD->list[40]
#define OD_ENTRY_H6106 &OD->list[41]
#define OD_ENTRY_H6200 &OD->list[42]
#define OD_ENTRY_H6220 &OD->list[43]
#define OD_ENTRY_H6240 &OD->list[44]
#define OD_ENTRY_H6260 &OD->list[45]
#define OD_ENTRY_H6280 &OD->list[46]
#define OD_ENTRY_H62A0 &OD->list[47]
#define OD_ENTRY_H6300 &OD->list[48]
#define OD_ENTRY_H7000 &OD->list[49]
#define OD_ENTRY_H7100 &OD->list[50]
#define OD_ENTRY_H7200 &OD->list[51]
#define OD_ENTRY_H7300 &OD->list[52]
#define OD_ENTRY_H7400 &OD->list[53]
#define OD_ENTRY_H7500 &OD->list[54]
#define OD_ENTRY_H8000 &OD->list[55]
#define OD_ENTRY_H8100 &OD->list[56]
#define OD_ENTRY_H9000 &OD->list[57]
#define OD_ENTRY_H9100 &OD->list[58]
#define OD_ENTRY_H9800 &OD->list[59]
#define OD_ENTRY_H9801 &OD->list[60]
#define OD_ENTRY_H9900 &OD->list[61]


/*******************************************************************************
    Object dictionary entries - shortcuts with names
*******************************************************************************/
#define OD_ENTRY_H1000_deviceType &OD->list[0]
#define OD_ENTRY_H1001_errorRegister &OD->list[1]
#define OD_ENTRY_H1003_pre_definedErrorField &OD->list[2]
#define OD_ENTRY_H1005_COB_ID_SYNCMessage &OD->list[3]
#define OD_ENTRY_H1006_communicationCyclePeriod &OD->list[4]
#define OD_ENTRY_H1007_synchronousWindowLength &OD->list[5]
#define OD_ENTRY_H1010_storeParameters &OD->list[6]
#define OD_ENTRY_H1011_restoreDefaultParameters &OD->list[7]
#define OD_ENTRY_H1012_COB_IDTimeStampObject &OD->list[8]
#define OD_ENTRY_H1014_COB_ID_EMCY &OD->list[9]
#define OD_ENTRY_H1015_inhibitTimeEMCY &OD->list[10]
#define OD_ENTRY_H1016_consumerHeartbeatTime &OD->list[11]
#define OD_ENTRY_H1017_producerHeartbeatTime &OD->list[12]
#define OD_ENTRY_H1018_identity &OD->list[13]
#define OD_ENTRY_H1019_synchronousCounterOverflowValue &OD->list[14]
#define OD_ENTRY_H1200_SDOServerParameter &OD->list[15]
#define OD_ENTRY_H1280_SDOClientParameter &OD->list[16]
#define OD_ENTRY_H1400_RPDOCommunicationParameter &OD->list[17]
#define OD_ENTRY_H1401_RPDOCommunicationParameter &OD->list[18]
#define OD_ENTRY_H1402_RPDOCommunicationParameter &OD->list[19]
#define OD_ENTRY_H1403_RPDOCommunicationParameter &OD->list[20]
#define OD_ENTRY_H1600_RPDOMappingParameter &OD->list[21]
#define OD_ENTRY_H1601_RPDOMappingParameter &OD->list[22]
#define OD_ENTRY_H1602_RPDOMappingParameter &OD->list[23]
#define OD_ENTRY_H1603_RPDOMappingParameter &OD->list[24]
#define OD_ENTRY_H1800_TPDOCommunicationParameter &OD->list[25]
#define OD_ENTRY_H1801_TPDOCommunicationParameter &OD->list[26]
#define OD_ENTRY_H1802_TPDOCommunicationParameter &OD->list[27]
#define OD_ENTRY_H1803_TPDOCommunicationParameter &OD->list[28]
#define OD_ENTRY_H1A00_TPDOMappingParameter &OD->list[29]
#define OD_ENTRY_H1A01_TPDOMappingParameter &OD->list[30]
#define OD_ENTRY_H1A02_TPDOMappingParameter &OD->list[31]
#define OD_ENTRY_H1A03_TPDOMappingParameter &OD->list[32]
#define OD_ENTRY_H3000_actorMothership &OD->list[33]
#define OD_ENTRY_H4000_deviceCircuit_1 &OD->list[34]
#define OD_ENTRY_H6000_moduleMCU &OD->list[35]
#define OD_ENTRY_H6020_actorCANopen &OD->list[36]
#define OD_ENTRY_H6080_actorDatabase &OD->list[37]
#define OD_ENTRY_H6100_moduleTimer_1 &OD->list[38]
#define OD_ENTRY_H6101_moduleTimer_2 &OD->list[39]
#define OD_ENTRY_H6102_moduleTimer_3 &OD->list[40]
#define OD_ENTRY_H6106_moduleTimer_7 &OD->list[41]
#define OD_ENTRY_H6200_transportCAN_1 &OD->list[42]
#define OD_ENTRY_H6220_transportSPI_1 &OD->list[43]
#define OD_ENTRY_H6240_transportUSART_1 &OD->list[44]
#define OD_ENTRY_H6260_transportI2C_1 &OD->list[45]
#define OD_ENTRY_H6280_transportModbus_1 &OD->list[46]
#define OD_ENTRY_H62A0_transportSDIO &OD->list[47]
#define OD_ENTRY_H6300_moduleADC_1 &OD->list[48]
#define OD_ENTRY_H7000_storageEeprom_1 &OD->list[49]
#define OD_ENTRY_H7100_storageW25 &OD->list[50]
#define OD_ENTRY_H7200_storageFlash &OD->list[51]
#define OD_ENTRY_H7300_memorySRAM_1 &OD->list[52]
#define OD_ENTRY_H7400_storageAT24C &OD->list[53]
#define OD_ENTRY_H7500_storageSDCard &OD->list[54]
#define OD_ENTRY_H8000_inputSensor_1 &OD->list[55]
#define OD_ENTRY_H8100_controlTouchscreen_1 &OD->list[56]
#define OD_ENTRY_H9000_screenEpaper_1 &OD->list[57]
#define OD_ENTRY_H9100_screenILI9341 &OD->list[58]
#define OD_ENTRY_H9800_indicatorLED_1 &OD->list[59]
#define OD_ENTRY_H9801_indicatorLED_2 &OD->list[60]
#define OD_ENTRY_H9900_signalBeeper_1 &OD->list[61]


/*******************************************************************************
    OD config structure
*******************************************************************************/
#ifdef CO_MULTIPLE_OD
#define OD_INIT_CONFIG(config) {\
    (config).CNT_NMT = OD_CNT_NMT;\
    (config).ENTRY_H1017 = OD_ENTRY_H1017;\
    (config).CNT_HB_CONS = OD_CNT_HB_CONS;\
    (config).CNT_ARR_1016 = OD_CNT_ARR_1016;\
    (config).ENTRY_H1016 = OD_ENTRY_H1016;\
    (config).CNT_EM = OD_CNT_EM;\
    (config).ENTRY_H1001 = OD_ENTRY_H1001;\
    (config).ENTRY_H1014 = OD_ENTRY_H1014;\
    (config).ENTRY_H1015 = OD_ENTRY_H1015;\
    (config).CNT_ARR_1003 = OD_CNT_ARR_1003;\
    (config).ENTRY_H1003 = OD_ENTRY_H1003;\
    (config).CNT_SDO_SRV = OD_CNT_SDO_SRV;\
    (config).ENTRY_H1200 = OD_ENTRY_H1200;\
    (config).CNT_SDO_CLI = OD_CNT_SDO_CLI;\
    (config).ENTRY_H1280 = OD_ENTRY_H1280;\
    (config).CNT_TIME = OD_CNT_TIME;\
    (config).ENTRY_H1012 = OD_ENTRY_H1012;\
    (config).CNT_SYNC = OD_CNT_SYNC;\
    (config).ENTRY_H1005 = OD_ENTRY_H1005;\
    (config).ENTRY_H1006 = OD_ENTRY_H1006;\
    (config).ENTRY_H1007 = OD_ENTRY_H1007;\
    (config).ENTRY_H1019 = OD_ENTRY_H1019;\
    (config).CNT_RPDO = OD_CNT_RPDO;\
    (config).ENTRY_H1400 = OD_ENTRY_H1400;\
    (config).ENTRY_H1600 = OD_ENTRY_H1600;\
    (config).CNT_TPDO = OD_CNT_TPDO;\
    (config).ENTRY_H1800 = OD_ENTRY_H1800;\
    (config).ENTRY_H1A00 = OD_ENTRY_H1A00;\
    (config).CNT_LEDS = 0;\
    (config).CNT_GFC = 0;\
    (config).ENTRY_H1300 = NULL;\
    (config).CNT_SRDO = 0;\
    (config).ENTRY_H1301 = NULL;\
    (config).ENTRY_H1381 = NULL;\
    (config).ENTRY_H13FE = NULL;\
    (config).ENTRY_H13FF = NULL;\
    (config).CNT_LSS_SLV = 0;\
    (config).CNT_LSS_MST = 0;\
    (config).CNT_GTWA = 0;\
    (config).CNT_TRACE = 0;\
}
#endif

#endif /* OD_H */
