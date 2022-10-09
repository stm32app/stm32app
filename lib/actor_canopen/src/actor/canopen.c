#include "canopen.h"
#include "OD.h"

#if ACTOR_NODE_USE_CANOPEN
//#include <actor/indicator/led.h>
//#include <actor/transport/can.h>

static void actor_thread_canopen_notify(actor_thread_t *thread);
static void actor_canopen_initialize_class(actor_canopen_t *canopen);

static ODR_t canopen_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    actor_canopen_t *canopen = stream->object;
    (void)canopen;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static actor_signal_t canopen_validate(actor_canopen_properties_t *properties) {
    return 0;
}

static actor_signal_t canopen_construct(actor_canopen_t *canopen) {

    /* Allocate memory */
    uint32_t heapMemoryUsed = 0;
    canopen->instance = CO_new(NULL, &heapMemoryUsed);
    if (canopen->instance == NULL) {
        debug_printf("Error: Can't allocate memory\n");
        return ACTOR_SIGNAL_OUT_OF_MEMORY;
    } else {
        if (heapMemoryUsed == 0) {
            debug_printf("Config - Static memory\n");
        } else {
            debug_printf("Config - On heap (%ubytes)\n", (unsigned int)heapMemoryUsed);
        }
    }

    /*
        debug_printf("Config - Storage...\n");

        CO_ReturnError_t err;
        err = CO_storageAbstract_init(&CO_storage, canopen->instance->CANmodule, NULL, OD_ENTRY_H1010_storeParameters,
       OD_ENTRY_H1011_restoreDefaultParameters, storageEntries, storageEntriesCount, storageInitError);

        if (false && err != CO_ERROR_NO && err != CO_ERROR_DATA_CORRUPT) {
            error_printf("Error: Storage %d\n", (int)*storageInitError);
            return err;
        }

        if (storageInitError != 0) {
            initError = storageInitError;
        }
        */
    return 0;
}

static actor_signal_t canopen_destruct(actor_canopen_t *canopen) {
    CO_CANsetConfigurationMode(canopen->instance);
    CO_delete(canopen->instance);
    return 0;
}

static bool actor_canopen_store_lss(actor_canopen_t *canopen, uint8_t node_id, uint16_t bitrate) {
    debug_printf("Config - Store LSS #%i @ %ikbps...\n", node_id, bitrate);
    actor_canopen_set_node_id(canopen, node_id);
    actor_canopen_set_bitrate(canopen, bitrate);
    return 0;
}

static actor_signal_t canopen_start(actor_canopen_t *canopen) {
    CO_ReturnError_t err;
    uint32_t errInfo = 0;

    /* Enter CAN propertiesuration. */
    debug_printf("Config - Communication...\n");
    canopen->instance->CANmodule->CANnormal = false;
    CO_CANsetConfigurationMode(canopen->instance);
    CO_CANmodule_disable(canopen->instance->CANmodule);

    /* Pass CAN propertiesuration to CANopen driver */
    actor_t *can_actor = actor_box(canopen);
    canopen->instance->CANmodule->port = can_actor->seq == 0 ? CAN1 : CAN2;
    canopen->instance->CANmodule->rxFifoIndex = canopen->properties->can_fifo_index;
    canopen->instance->CANmodule->sjw = transport_can_get_sjw(canopen->can);
    canopen->instance->CANmodule->prop = transport_can_get_prop(canopen->can);
    canopen->instance->CANmodule->brp = transport_can_get_brp(canopen->can);
    canopen->instance->CANmodule->ph_seg1 = transport_can_get_ph_seg1(canopen->can);
    canopen->instance->CANmodule->ph_seg2 = transport_can_get_ph_seg2(canopen->can);
    canopen->instance->CANmodule->bitrate = transport_can_get_bitrate(canopen->can);

    /* Initialize CANopen driver */
    err = CO_CANinit(canopen->instance, canopen->instance, canopen->properties->bitrate);
    if (false && err != CO_ERROR_NO) {
        error_printf("Error: CAN initialization failed: %d\n", err);
        return err;
    }

    /* Engage LSS propertiesuration */
    CO_LSS_address_t lssAddress = {.identity = {.vendorID = OD_PERSIST_COMM.x1018_identity.vendor_ID,
                                                .productCode = OD_PERSIST_COMM.x1018_identity.productCode,
                                                .revisionNumber = OD_PERSIST_COMM.x1018_identity.revisionNumber,
                                                .serialNumber = OD_PERSIST_COMM.x1018_identity.serialNumber}};

    err = CO_LSSinit(canopen->instance, &lssAddress, &canopen->properties->node_id, &canopen->properties->bitrate);
    CO_LSSslave_initCfgStoreCallback(canopen->instance->LSSslave, canopen, (bool(*)(void *, uint8_t, uint16_t))actor_canopen_store_lss);

    if (err != CO_ERROR_NO) {
        error_printf("Error: LSS slave initialization failed: %d\n", err);
        return err;
    }

    actor_canopen_initialize_class(canopen);

    /* Initialize CANopen itself */
    err = CO_CANopenInit(canopen->instance,                       /* CANopen object */
                         NULL,                                    /* alternate NMT */
                         NULL,                                    /* alternate em */
                         OD,                                      /* Object dictionary */
                         OD_STATUS_BITS,                          /* Optional OD_statusBit */
                         NMT_CONTROL,                             /* CO_NMT_control_t */
                         canopen->properties->first_hb_time,      /* firstHBTime_ms */
                         canopen->properties->sdo_client_timeout, /* SDOserverTimeoutTime_ms */
                         canopen->properties->sdo_client_timeout, /* SDOclientTimeoutTime_ms */
                         SDO_CLI_BLOCK,                           /* SDOclientBlockTransfer */
                         canopen->properties->node_id, &errInfo);

    if (err == CO_ERROR_OD_PARAMETERS) {
        debug_printf("CANopen - Error in Object Dictionary entry 0x%lX\n", errInfo);
    } else {
        debug_printf("CANopen - Initialization failed: %d  0x%lX\n", err, errInfo);
    }

    /* Emergency errors */
    if (!canopen->instance->nodeIdUnconfigured) {
        if (errInfo != 0) {
            CO_errorReport(canopen->instance->em, CO_EM_INCONSISTENT_OBJECT_DICT, CO_EMC_DATA_SET, errInfo);
        }
        // if (storageInitError != 0) {
        //    CO_errorReport(canopen->instance->em, CO_EM_NON_VOLATILE_MEMORY, CO_EMC_HARDWARE, storageInitError);
        //}
    }

    if (err == CO_ERROR_NODE_ID_UNCONFIGURED_LSS) {
        return CO_ERROR_NO;
    }

    /* start CAN */
    CO_CANsetNormalMode(canopen->instance->CANmodule);

    err = CO_CANopenInitPDO(canopen->instance, canopen->instance->em, OD, canopen->properties->node_id, &errInfo);
    return 0;
}

static actor_signal_t canopen_stop(actor_canopen_t *canopen) {
    debug_printf("Config - Unloading...\n");
    CO_CANsetConfigurationMode((void *)&canopen->instance);
    CO_delete(canopen->instance);
    return 0;
}

static actor_signal_t canopen_link(actor_canopen_t *canopen) {
    actor_link(canopen->actor, (void **)&canopen->can, canopen->properties->can_index, NULL);
    actor_link(canopen->actor, (void **)&canopen->red_led, canopen->properties->red_led_index, NULL);
    actor_link(canopen->actor, (void **)&canopen->green_led, canopen->properties->green_led_index, NULL);
    return 0;
}

static actor_signal_t canopen_worker_high_priority(actor_canopen_t *canopen, void *argument, actor_worker_t *worker, actor_thread_t *thread) {
    (void)argument;
    worker->next_time = 0;

    uint32_t us_since_last = (thread->current_time - worker->last_time) * 1000;
    uint32_t us_until_next = -1;
    switch (CO_process(canopen->instance, false, us_since_last, &us_until_next)) {
    case CO_RESET_COMM: actor_set_phase(canopen->actor->node->actor, ACTOR_RESETTING); break;
    case CO_RESET_APP:
    case CO_RESET_QUIT: actor_set_phase(canopen->actor, ACTOR_RESETTING); break;
    default: break;
    }

    if (canopen->red_led != NULL)
        indicator_led_set_duty_cycle(canopen->red_led, CO_LED_RED(canopen->instance->LEDs, CO_LED_CANopen) ? 255 : 0);
    if (canopen->green_led != NULL)
        indicator_led_set_duty_cycle(canopen->green_led, CO_LED_GREEN(canopen->instance->LEDs, CO_LED_CANopen) ? 255 : 0);

    if (us_until_next != (uint32_t)-1) {
        actor_worker_delay(worker, us_until_next / 1000);
    }
    return 0;
}

/* CANopen accepts its input from interrupts */
static actor_signal_t canopen_worker_on_input(actor_canopen_t *canopen, void *argument, actor_worker_t *worker, actor_thread_t *thread) {
    (void)argument;

    uint32_t us_since_last = (thread->current_time - worker->last_time) * 1000;
    uint32_t us_until_next = -1;
    CO_LOCK_OD(canopen->instance->CANmodule);
    if (!canopen->instance->nodeIdUnconfigured && canopen->instance->CANmodule->CANnormal) {
        bool syncWas = false;

#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
        syncWas = CO_process_SYNC(canopen->instance, us_since_last, &us_until_next);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
        CO_process_RPDO(canopen->instance, syncWas, us_since_last, &us_until_next);
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_TPDO_ENABLE
        CO_process_TPDO(canopen->instance, syncWas, us_since_last, &us_until_next);
#endif
        (void)syncWas;
    }
    CO_UNLOCK_OD(canopen->instance->CANmodule);
    if (us_until_next != (uint32_t)-1) {
        actor_worker_delay(worker, us_until_next / 1000);
    }
    return 0;
}

static actor_signal_t canopen_phase(actor_canopen_t *canopen, actor_phase_t phase) {
    (void)canopen;
    (void)phase;
    return 0;
}

/* Publish a wakeup event to thread for immediate processing */
static void actor_thread_canopen_notify(actor_thread_t *thread) {
    actor_canopen_t *canopen = thread->actor->node->canopen;

    actor_message_t event = {.type = ACTOR_MESSAGE_RESPONSE, .producer = canopen->actor, .consumer = canopen->actor};
    // canopen messages are safe to send in any order and it is desirable to handle them asap
    actor_thread_publish(thread, &event);
}

static actor_worker_callback_t canopen_worker_assignment(actor_canopen_t *canopen, actor_thread_t *thread) {
    if (thread == canopen->actor->node->input) {
        return (actor_worker_callback_t) canopen_worker_on_input;
    } else if (thread == canopen->actor->node->high_priority) {
        return (actor_worker_callback_t) canopen_worker_high_priority;
    }
    return NULL;
}

actor_interface_t actor_canopen_class = {
    .type = ACTOR_CANOPEN,
    .size = sizeof(actor_canopen_t),
    .phase_subindex = ACTOR_CANOPEN_PHASE,

    .validate = (actor_method_t)canopen_validate,
    .construct = canopen_construct,
    .destruct = canopen_destruct,
    .link = canopen_link,
    .start = canopen_start,
    .stop = canopen_stop,

    .phase_changed = (actor_phase_changed_t)  canopen_phase,
    .property_write = canopen_property_write,
    .worker_assignment = (actor_worker_assignment_t) canopen_worker_assignment,
};

static void actor_canopen_initialize_class(actor_canopen_t *canopen) {
    actor_node_t *node = canopen->actor->node;

    /* Mainline tasks */
    if (CO_GET_CNT(EM) == 1) {
        CO_EM_initCallbackPre(canopen->instance->em, (void *)&node->input, (void (*)(void *))actor_thread_canopen_notify);
    }
    if (CO_GET_CNT(NMT) == 1) {
        CO_NMT_initCallbackPre(canopen->instance->NMT, (void *)&node->input, (void (*)(void *))actor_thread_canopen_notify);
    }
#if (CO_CONFIG_SRDO) & CO_CONFIG_SRDO_SRV_ENABLE
    for (int16_t i = 0; i < CO_GET_CNT(SRDO); i++) {
        CO_SRDO_initCallbackPre(&canopen->instance->SRDO[i], (void *)&node->input, (void (*)(void *))actor_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_HB_CONS) & CO_CONFIG_HB_CONS_ENABLE
    if (CO_GET_CNT(HB_CONS) == 1) {
        CO_HBconsumer_initCallbackPre(canopen->instance->HBCons, (void *)&node->input, (void (*)(void *))actor_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_TIME) & CO_CONFIG_TIME_ENABLE
    if (CO_GET_CNT(TIME) == 1) {
        CO_TIME_initCallbackPre(canopen->instance->TIME, (void *)&node->input, (void (*)(void *))actor_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_SDO_CLI) & CO_CONFIG_SDO_CLI_ENABLE
    for (int16_t i = 0; i < CO_GET_CNT(SDO_CLI); i++) {
        CO_SDOclient_initCallbackPre(&canopen->instance->SDOclient[i], (void *)&node->input,
                                     (void (*)(void *))actor_thread_canopen_notify);
    }
#endif
    for (int16_t i = 0; i < CO_GET_CNT(SDO_SRV); i++) {
        CO_SDOserver_initCallbackPre(&canopen->instance->SDOserver[i], (void *)&node->input,
                                     (void (*)(void *))actor_thread_canopen_notify);
    }
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_MASTER
    CO_LSSmaster_initCallbackPre(canopen->instance->LSSmaster, (void *)&node->input, (void (*)(void *))actor_thread_canopen_notify);
#endif
#if (CO_CONFIG_LSS) & CO_CONFIG_LSS_SLAVE
    CO_LSSslave_initCallbackPre(canopen->instance->LSSslave, (void *)&node->input, (void (*)(void *))actor_thread_canopen_notify);
#endif
/* Processing tasks */
#if (CO_CONFIG_SYNC) & CO_CONFIG_SYNC_ENABLE
    if (CO_GET_CNT(SYNC) == 1) {
        CO_SYNC_initCallbackPre(canopen->instance->SYNC, (void *)&node->high_priority, (void (*)(void *))actor_thread_canopen_notify);
    }
#endif
#if (CO_CONFIG_PDO) & CO_CONFIG_RPDO_ENABLE
    for (int i = 0; i < CO_GET_CNT(RPDO); i++) {
        CO_RPDO_initCallbackPre(&canopen->instance->RPDO[i], (void *)&node->high_priority, actor_thread_canopen_notify);
    }
#endif
}

void actor_node_error_report(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index) {
    if (node != NULL && node->canopen != NULL) {
        CO_errorReport(((actor_canopen_t *)(node->canopen))->instance->em, errorBit, errorCode, index);
    }
}
void actor_node_error_reset(actor_node_t *node, const uint8_t errorBit, uint16_t errorCode, uint32_t index) {
    if (node != NULL && node->canopen != NULL) {
        CO_errorReset(((actor_canopen_t *)node->canopen)->instance->em, errorBit, index);
    }
}
#endif