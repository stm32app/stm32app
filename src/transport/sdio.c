#include "sdio.h"
#include <libopencm3/stm32/sdio.h>

#ifdef __GNUC__
#define bitswap32 __builtin_bswap32
#define bitswap16 __builtin_bswap16
#else
#define bitswap32 __REV
#endif

#define SDIO_STATUS_ERROR_MASK                                                                                                             \
    (SDIO_MASK_CCRCFAILIE | SDIO_MASK_DCRCFAILIE | SDIO_MASK_CTIMEOUTIE | SDIO_MASK_DTIMEOUTIE | SDIO_MASK_TXUNDERRIE |                    \
     SDIO_MASK_RXOVERRIE | SDIO_MASK_STBITERRIE)
#define SDIO_ICR_STATIC                                                                                                                    \
    SDIO_ICR_CCRCFAILC | SDIO_ICR_DCRCFAILC | SDIO_ICR_CTIMEOUTC | SDIO_ICR_DTIMEOUTC | SDIO_ICR_TXUNDERRC | SDIO_ICR_RXOVERRC |           \
        SDIO_ICR_CMDRENDC | SDIO_ICR_CMDSENTC | SDIO_ICR_DATAENDC | SDIO_ICR_DBCKENDC

#define SDIO_STATUS_SUCCESS_MASK (SDIO_MASK_CMDRENDIE | SDIO_MASK_CMDSENTIE | SDIO_MASK_DBCKENDIE | SDIO_STATUS_ERROR_MASK)
#define SDIO_HIGH_CAPACITY ((uint32_t)0x40000000U)
#define SDIO_STANDARD_CAPACITY ((uint32_t)0x00000000U)
#define SDIO_3_0_to_3_3_V ((uint32_t)0x80100000U)

static ODR_t sdio_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    transport_sdio_t *sdio = stream->object;
    (void)sdio;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t sdio_validate(transport_sdio_properties_t *properties) {
    return 0;
}

static app_signal_t sdio_construct(transport_sdio_t *sdio) {
    actor_event_subscribe(sdio->actor, APP_EVENT_DIAGNOSE);
    sdio->address = SDIO_BASE;

#if defined(RCC_APB1ENR_SDIOEN)
    sdio->source_clock = &RCC_APB1ENR;
    sdio->reset = RCC_APB1RSTR_SDIORST;
    sdio->peripheral_clock = RCC_APB1ENR_SDIOEN;
#elif defined(RCC_APB2ENR_SDIOEN)
    sdio->source_clock = (uint32_t *)&RCC_APB2ENR;
    sdio->reset = RCC_APB2RSTR_SDIORST;
    sdio->peripheral_clock = RCC_APB2ENR_SDIOEN;
#else
    sdio->source_clock = &RCC_AHBENR;
    sdio->peripheral_clock = RCC_AHBENR_SDIOEN;
#endif
    sdio->irq = NVIC_SDIO_IRQ;
    return 0;
}

transport_sdio_t *sdio_units[SDIO_UNITS];

static void sdio_notify(uint8_t index) {
    debug_printf("> SDIO Interrupt\n");
    transport_sdio_t *sdio = sdio_units[index];
    if (sdio != NULL && sdio->job.handler) {
        app_job_execute(&sdio->job);
    } else {
        SDIO_ICR = 0xfffffff;
    }
    debug_printf("< SDIO Interrupt\n");
}

static void sdio_disable(void) {
    SDIO_POWER = SDIO_POWER_PWRCTRL_PWROFF;
    rcc_peripheral_reset(&RCC_APB2RSTR, RCC_APB2RSTR_SDIORST);
    rcc_peripheral_clear_reset(&RCC_APB2RSTR, RCC_APB2RSTR_SDIORST);
}

static void sdio_enable(void) {
    sdio_disable();
    SDIO_POWER = SDIO_POWER_PWRCTRL_PWRON;
    // 118 is 400khz
    SDIO_CLKCR = SDIO_CLKCR_WIDBUS_1 | 118 | SDIO_CLKCR_CLKEN;
    SDIO_MASK = 0xffffffff;
}

static app_signal_t sdio_start(transport_sdio_t *sdio) {
    sdio_units[sdio->actor->seq] = sdio;
    rcc_peripheral_enable_clock(sdio->source_clock, sdio->peripheral_clock);
    gpio_configure_af_pullup(sdio->properties->d0_port, sdio->properties->d0_pin, 50, sdio->properties->af);
    gpio_configure_af_pullup(sdio->properties->d1_port, sdio->properties->d1_pin, 50, sdio->properties->af);
    gpio_configure_af_pullup(sdio->properties->d2_port, sdio->properties->d2_pin, 50, sdio->properties->af);
    gpio_configure_af_pullup(sdio->properties->d3_port, sdio->properties->d3_pin, 50, sdio->properties->af);
    gpio_configure_af_pullup(sdio->properties->ck_port, sdio->properties->ck_pin, 50, sdio->properties->af);
    gpio_configure_af_pullup(sdio->properties->cmd_port, sdio->properties->cmd_pin, 50, sdio->properties->af);

    nvic_set_priority(sdio->irq, 10 << 4);

    return 0;
}

static app_signal_t sdio_stop(transport_sdio_t *sdio) {
    return 0;
}

static app_signal_t sdio_link(transport_sdio_t *sdio) {
    return 0;
}

static app_signal_t sdio_on_phase(transport_sdio_t *sdio, actor_phase_t phase) {
    return 0;
}

static app_signal_t sdio_on_signal(transport_sdio_t *sdio, actor_t *actor, app_signal_t signal, void *source) {
    return 0;
}

static app_job_signal_t sdio_send_command(uint32_t cmd, uint32_t arg, uint32_t wait) {
    SDIO_ICR = SDIO_STA_CCRCFAIL | SDIO_ICR_CTIMEOUTC | SDIO_ICR_CMDRENDC | SDIO_ICR_CMDSENTC;
    SDIO_ARG = arg;
    SDIO_CMD = cmd | SDIO_CMD_CPSMEN | wait;
    return APP_JOB_TASK_CONTINUE;
}


static app_signal_t sdio_get_status(uint8_t cmd) {
    uint32_t value = SDIO_STA;
    if ((value & SDIO_STA_CMDACT) || (cmd == 0 && !(value & SDIO_STA_CMDSENT))) {
        return APP_SIGNAL_BUSY;
    }
    // SDIO_ICR = 0xffffffff;
    // SDIO_ICR = result;
    // if (!value) {
    //    return APP_SIGNAL_NO_RESPONSE; // no response
    //} else

    // uint32_t value = (SDIO_STA & 0x7ff);
    if (value & SDIO_STA_CCRCFAIL) {
        SDIO_ICR = SDIO_ICR_CCRCFAILC;
        if (cmd == 41) { // 41 is always corrupted, that's expected
            return APP_SIGNAL_OK;
        }
        return APP_SIGNAL_CORRUPTED; 
    } else if (value & SDIO_STA_CTIMEOUT) {
        SDIO_ICR = SDIO_ICR_CTIMEOUTC;
        return APP_SIGNAL_TIMEOUT;
    } else if (!(value & SDIO_STA_CMDREND)) {
        return APP_SIGNAL_BUSY;
    } else if (cmd != 0 && SDIO_RESPCMD != cmd) {
        return APP_SIGNAL_FAILURE;
    }

    SDIO_ICR = SDIO_ICR_STATIC;

    return APP_SIGNAL_OK;
}

static app_job_signal_t sdio_get_signal(uint8_t cmd) {
    app_signal_t signal = sdio_get_status(cmd);
    switch (signal) {
    case APP_SIGNAL_BUSY:
        return APP_JOB_TASK_WAIT;
    case APP_SIGNAL_OK:
        return 0;
    default:
        return APP_JOB_TASK_FAILURE;
    }
}
static app_job_signal_t sdio_wait(uint8_t cmd) {
    app_signal_t signal;
    if ((signal = sdio_get_signal(cmd))) {
        return signal;
    } else {
        return APP_JOB_TASK_CONTINUE;
    }
}

static app_job_signal_t sdio_send_command_blocking(uint32_t cmd, uint32_t arg, uint32_t wait) {
    sdio_send_command(cmd, arg, wait);
    for (size_t retries = 65000; retries-- > 0;) {
        app_job_signal_t signal = sdio_wait(cmd);
        printf("block %lu %u\n", cmd, retries);
        if (signal != APP_JOB_TASK_WAIT) {
            return signal;
        }
    }
    return APP_JOB_TASK_FAILURE;
}

static void sdio_read_r2(uint32_t *destination) {
    destination[0] = bitswap32(SDIO_RESP1);
    destination[1] = bitswap32(SDIO_RESP2);
    destination[2] = bitswap32(SDIO_RESP3);
    destination[3] = bitswap32(SDIO_RESP4);
}

static void sdio_parse_cid(transport_sdio_t *sdio) {
    uint8_t cid[16];
    sdio_read_r2((uint32_t *)&cid);
    transport_sdio_set_manufacturer_id(sdio, cid[0]);
    transport_sdio_set_oem_id(sdio, bitswap16(cid[1]));
    transport_sdio_set_product_name(sdio, &cid[5], 5);
    transport_sdio_set_product_revision(sdio, cid[8]);
    transport_sdio_set_serial_number(sdio, bitswap32(cid[9]));
    transport_sdio_set_manufacturing_date(sdio, bitswap16(cid[13]));
}

static void sdio_parse_csd(transport_sdio_t *sdio) {
    uint8_t csd[16];
    sdio_read_r2((uint32_t *)&csd);
    transport_sdio_set_max_bus_clock_frequency(sdio, csd[3]);
    transport_sdio_set_csd_version(sdio, csd[0] >> 6);
    if (sdio->properties->csd_version == 0) {
        uint32_t block_size = ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | ((csd[8] & 0xc0) >> 6);
        uint32_t multiplier = (((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7));
        transport_sdio_set_block_count(sdio, 1 + block_size * multiplier);
        transport_sdio_set_block_size(sdio, csd[5] & 0x0f);
    } else {
        transport_sdio_set_block_count(sdio, 1 + (((csd[7] & 0x3f) << 16) | (csd[8] << 8) | csd[9]));
        transport_sdio_set_block_size(sdio, 512);
    }
    transport_sdio_set_capacity(sdio, sdio->properties->block_count * sdio->properties->block_size);
}

static void sdio_set_bus_clock(uint8_t divider) {
    SDIO_CLKCR = (SDIO_CLKCR & (~SDIO_CLKCR_CLKDIV_MASK)) | (divider & SDIO_CLKCR_CLKDIV_MASK);
}

static app_job_signal_t sdio_task_detect_card(app_job_t *job) {
    transport_sdio_t *sdio = job->actor->object;\
    switch (job->task_phase) {
        // Power up the card
    case 0:
        nvic_enable_irq(sdio->irq);
        sdio_enable();
        return sdio_send_command(0, 0, SDIO_CMD_WAITRESP_NO_0); // GO_IDLE_STATE
    case 1:
        return sdio_wait(0);
    // Detect if card is V2 or V1
    case 2:
        return sdio_send_command(8, 0x1F1, SDIO_CMD_WAITRESP_SHORT); // SEND_IF_COND
    case 3:
        switch (sdio_get_status(8)) {
        case APP_SIGNAL_BUSY:
            return APP_JOB_TASK_LOOP;
        case APP_SIGNAL_TIMEOUT:
            transport_sdio_set_version(sdio, 1);
            return APP_JOB_TASK_CONTINUE;
        case APP_SIGNAL_OK:
            // sdcard echoed back correctly
            if (SDIO_RESP1 == 0x1F1) {
                transport_sdio_set_version(sdio, 2);
                job->task_phase += 2; // skip two next steps
                return APP_JOB_TASK_CONTINUE;
            }
            break;
        default:
            return APP_JOB_TASK_FAILURE;
        }
    // V1 cards only: Send 55 reset Illegal Command bit
    case 4:
        return sdio_send_command(55, 0, SDIO_CMD_WAITRESP_SHORT); // CMD_APP_CMD
    case 5:
        return sdio_wait(55);
    // Initialize card and wait for ready status (55 + 41)
    case 6:
        return sdio_send_command(55, 0, SDIO_CMD_WAITRESP_SHORT); // CMD_APP_CMD
    case 7:
        return sdio_wait(55);
    case 8:
        // MMC not supported: Need to send CMD1 instead of ACMD41
        return sdio_send_command(41, (SDIO_3_0_to_3_3_V | (sdio->properties->version == 2 ? SDIO_HIGH_CAPACITY : SDIO_STANDARD_CAPACITY)),
                                 SDIO_CMD_WAITRESP_SHORT);
    case 9:
        return sdio_wait(41);
    case 10:
        if (!(SDIO_RESP1 & (1 << 31))) {
            job->task_phase = 6;
            return APP_JOB_TASK_LOOP; // TODO: retries
        } else {
            if (SDIO_RESP1 & SDIO_HIGH_CAPACITY) {
                transport_sdio_set_high_capacity(sdio, 1);
            }
            return APP_JOB_TASK_CONTINUE;
        }
    // Now the CMD2 and CMD3 commands should be issued in cycle until timeout to enumerate all cards on the bus
    // Since this module suitable to work with single card, issue this commands one time only
    case 11:
        return sdio_send_command(2, 0, SDIO_CMD_WAITRESP_LONG); // ALL_SEND_CID
    case 12:
        return sdio_wait(2);
    case 13:
        sdio_parse_cid(sdio);
        printf("│ │ ├ Card name %.*s\n", 3, sdio->properties->product_name);
        if (sdio->properties->product_name[0] == '\0') {
            printf("Bad card\n");
        }
        return APP_JOB_TASK_CONTINUE;
    // Get relative card address
    // MMC not supported:  host should set a RCA value to the card by SET_REL_ADD
    case 14:
        return sdio_send_command(3, 0, SDIO_CMD_WAITRESP_SHORT); // SEND_REL_ADDR
    case 15:
        return sdio_wait(3);
    case 16:
        // r6 errors
        if (SDIO_RESP1 & (0x00002000U | 0x00004000U | 0x00008000U)) {
            return APP_JOB_TASK_FAILURE;
        }
        transport_sdio_set_relative_card_address(sdio, SDIO_RESP1 >> 16);
        return APP_JOB_TASK_CONTINUE;
    // Retrieve CSD register:
    case 17:
        return sdio_send_command(9, (sdio->properties->relative_card_address << 16), SDIO_CMD_WAITRESP_LONG); // SEND_CSD
    case 18:
        return sdio_wait(9);
    case 19:
        sdio_parse_csd(sdio);
        printf("│ │ ├ Card capacity %luMB\n", sdio->properties->capacity >> 10);
        return APP_JOB_TASK_CONTINUE;
    // Increase bus speed to 48mhz, set card into transfer mode
    case 20:
        sdio_set_bus_clock(0);
        return sdio_send_command(9, sdio->properties->relative_card_address << 16, SDIO_CMD_WAITRESP_SHORT); // SEL_DESEL_CARD
    case 21:
        return sdio_wait(9);
    case 22:
        return APP_JOB_TASK_SUCCESS;
    case APP_JOB_TASK_FAILURE:
        printf("Fail!\n");
        break;
    case APP_JOB_TASK_SUCCESS:
        printf("Success!\n");
        break;
    }
    // transport_sdio_t *sdio = job->actor->object;
    return 0;
}

static app_job_signal_t sdio_job_diagnose(app_job_t *job) {
    switch (job->job_phase) {
    case 0:
        return sdio_task_detect_card(job);
    case 1:
        return APP_JOB_SUCCESS;
    }
    return 0;
}
static app_signal_t sdio_on_high_priority(transport_sdio_t *sdio, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    return app_job_execute_if_running_in_thread(&sdio->job, thread);
}

static app_signal_t sdio_on_input(transport_sdio_t *sdio, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    debug_log_inhibited = false;
    switch (event->type) {
    case APP_EVENT_DIAGNOSE:
        return actor_event_handle_and_start_job(sdio->actor, event, &sdio->job, sdio->actor->app->threads->high_priority,
                                                sdio_job_diagnose);
        break;
    default:
        break;
    }
    return 0;
}

void sdio_isr(void) {
    sdio_notify(0);
}

actor_class_t transport_sdio_class = {
    .type = TRANSPORT_SDIO,
    .size = sizeof(transport_sdio_t),
    .phase_subindex = TRANSPORT_SDIO_PHASE,
    .validate = (app_method_t)sdio_validate,
    .construct = (app_method_t)sdio_construct,
    .link = (app_method_t)sdio_link,
    .start = (app_method_t)sdio_start,
    .stop = (app_method_t)sdio_stop,
    .on_phase = (actor_on_phase_t)sdio_on_phase,
    .on_signal = (actor_on_signal_t)sdio_on_signal,
    .worker_input = (actor_on_worker_t)sdio_on_input,
    .worker_high_priority = (actor_on_worker_t)sdio_on_high_priority,
    .property_write = sdio_property_write,
};
