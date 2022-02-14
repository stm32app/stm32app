#include "sdio.h"
#include "core/buffer.h"
#include "lib/bytes.h"
#include "lib/dma.h"
#include <libopencm3/stm32/sdio.h>

// heavily inspired by code at
// https://github.com/LonelyWolf/stm32/blob/master/stm32l4-sdio/src/sdcard.c

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
    // 118 is 400khz
    // start with 1-bit width of bus
    // power saving is enabled
    SDIO_CLKCR = SDIO_CLKCR_WIDBUS_1 | SDIO_CLKCR_CLKDIV_INITIAL | SDIO_CLKCR_CLKEN; /*| SDIO_CLKCR_PWRSAV*///| SDIO_CLKCR_HWFC_EN;
    SDIO_MASK = 0xffffffff & ~(SDIO_MASK_RXDAVLIE) & (~SDIO_MASK_RXACTIE);// & (~SDIO_MASK_RXFIFOEIE);
    SDIO_POWER = SDIO_POWER_PWRCTRL_PWRON;
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
    switch (signal) {
    // DMA interrupts on buffer filling up half-way or all the way up
    case APP_SIGNAL_DMA_TRANSFERRING:
        sdio->incoming_signal = APP_SIGNAL_DMA_TRANSFERRING;
        app_job_execute(&sdio->job);
        break;
    default:
        break;
    }
    return 0;
}
static app_job_signal_t sdio_send_command(uint32_t cmd, uint32_t arg, uint32_t wait) {
    SDIO_ICR = SDIO_ICR_CMD;
    SDIO_ARG = arg;
    SDIO_CMD = cmd | SDIO_CMD_CPSMEN | wait;
    return APP_JOB_TASK_CONTINUE;
}

static app_signal_t sdio_get_status(uint8_t cmd) {
    uint32_t value = SDIO_STA;
    if (value & SDIO_STA_CCRCFAIL) {
        SDIO_ICR = SDIO_ICR_CCRCFAILC;
        if (cmd == 41) { // 41 is always corrupted, that's expected
            return APP_SIGNAL_OK;
        }
        return APP_SIGNAL_CORRUPTED;
    } else if (value & SDIO_STA_CTIMEOUT) {
        SDIO_ICR = SDIO_ICR_CTIMEOUTC;
        return APP_SIGNAL_TIMEOUT;
    }

    if (cmd == 0) {
        if (!(value & SDIO_STA_CMDSENT)) {
            return APP_SIGNAL_BUSY;
        }
    } else {
        if ((value & SDIO_STA_CMDACT) || !(value & SDIO_STA_CMDREND)) {
            return APP_SIGNAL_BUSY;
            // only short-wait commands have RESPCMD
        } else if (cmd != 0 && cmd != 2 && cmd != 9 && SDIO_RESPCMD != cmd) {
            return APP_SIGNAL_FAILURE;
        }
    }

    SDIO_ICR = SDIO_ICR_STATIC;

    return APP_SIGNAL_OK;
}

static app_job_signal_t sdio_get_signal(uint8_t cmd) {
    app_signal_t signal = sdio_get_status(cmd);
    switch (signal) {
    case APP_SIGNAL_BUSY:
        return APP_JOB_TASK_LOOP;
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

static app_job_signal_t sdio_send_command_and_wait(uint32_t cmd, uint32_t arg, uint32_t wait) {
    if (SDIO_STA == 0) {
        sdio_send_command(cmd, arg, wait);
        return APP_JOB_TASK_LOOP;
    } else {
        return sdio_wait(cmd);
    }
}

static void sdio_read_r2(uint32_t *destination) {
    *destination++ = bitswap32(SDIO_RESP1);
    *destination++ = bitswap32(SDIO_RESP2);
    *destination++ = bitswap32(SDIO_RESP3);
    *destination++ = bitswap32(SDIO_RESP4);
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

static void sdio_dma_rx_start(transport_sdio_t *sdio, uint8_t *data, uint32_t size) {
    actor_register_dma(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->actor);

    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("RX started\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    actor_dma_rx_start((uint32_t)&SDIO_FIFO, sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel, data,
                       size / 4, false, 4, 0, 0);

}

static void sdio_dma_rx_stop(transport_sdio_t *sdio) {
    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("RX stopped\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    sdio->incoming_signal = 0;
    actor_dma_rx_stop(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);
    actor_unregister_dma(sdio->properties->dma_unit, sdio->properties->dma_stream);
}

static void sdio_dma_tx_start(transport_sdio_t *sdio, uint8_t *data, uint32_t size) {
    actor_register_dma(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->actor);

    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("TX started\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    // Data transfer:
    //   transfer mode: block
    //   direction: to card
    //   DMA: enabled
    //   block size: 2^9 = 512 bytes
    //   DPSM: enabled
    SDIO_DCTRL = 0;
    SDIO_DTIMER = 0;
    SDIO_DLEN = size;
    SDIO_DCTRL = (9 << 4) | SDIO_DCTRL_DTEN;

    actor_dma_tx_start((uint32_t)&SDIO_FIFO, sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel,
                       sdio->target_buffer->data, sdio->target_buffer->size / 4, false, 32, 4, 1);
}

static void sdio_dma_tx_stop(transport_sdio_t *sdio) {
    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("TX stopped\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    sdio->incoming_signal = 0;
    app_buffer_release(sdio->target_buffer, sdio->actor);
    actor_dma_tx_stop(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);
    actor_unregister_dma(sdio->properties->dma_unit, sdio->properties->dma_stream);
}

static app_job_signal_t sdio_task_write_block(app_job_t *job, uint32_t address, uint8_t *data, uint32_t size) {
    transport_sdio_t *sdio = job->actor->object;
    uint32_t block_count = size >> 9;
    uint32_t block_address = sdio->properties->high_capacity ? address >> 9 : address;

    switch (job->task_phase) {
    case 0:
        // SDSC card uses byte unit address and
        // SDHC/SDXC cards use block unit address (1 unit = 512 bytes)
        // For SDHC card addr must be converted to block unit address
        if (block_count > 1) {
            SDIO_MASK = SDIO_TX_MB_FLAGS;
            return sdio_send_command_and_wait(25, block_address, SDIO_CMD_WAITRESP_SHORT); // WRITE_MULTIPLE_BLOCK
        } else {
            SDIO_MASK = SDIO_TX_SB_FLAGS;
            return sdio_send_command_and_wait(24, block_address, SDIO_CMD_WAITRESP_SHORT); // WRITE_BLOCK
        }
    case 1:
        sdio_dma_tx_start(sdio, data, size);
        return APP_JOB_TASK_WAIT;
    // wait while tx is active
    case 2:
        if (SDIO_STA & SDIO_STA_TXACT) {
            return APP_JOB_TASK_LOOP;
        } else {
            sdio_dma_tx_stop(sdio);
            return APP_JOB_TASK_CONTINUE;
        }
    // stop multi-block transfer
    case 3:
        if (block_count > 1) {
            return sdio_send_command_and_wait(12, 0, SDIO_CMD_WAITRESP_SHORT); // STOP_TRANSMISSION
        } else {
            return APP_JOB_TASK_CONTINUE;
        }
    // check errors
    case 4:
        if (SDIO_STA & SDIO_XFER_ERROR_FLAGS) {
            return APP_JOB_TASK_FAILURE;
        } else {
            return APP_JOB_TASK_CONTINUE;
        }
    // Wait while the card is in programming state
    case 5:
        return sdio_send_command_and_wait(12, 0, SDIO_CMD_WAITRESP_SHORT);
    case 6:
        switch ((SDIO_RESP1 & 0x1e00) >> 9) {
        case 0x07U: // Programming
        case 0x06U: // Receiving
            break;
            job->task_phase = 5 - 1;
            return APP_JOB_TASK_CONTINUE;
        default:
            return APP_JOB_TASK_CONTINUE;
        }
    default:
        SDIO_ICR = SDIO_ICR_STATIC;
    }
    return APP_JOB_TASK_SUCCESS;
}

static app_job_signal_t sdio_task_read_block(app_job_t *job, uint32_t address, uint8_t *data, uint32_t size) {
    transport_sdio_t *sdio = job->actor->object;
    uint32_t block_count = size >> 9;
    uint32_t block_address = sdio->properties->high_capacity ? address >> 9 : address;

    switch (job->task_phase) {
    //case 0:
    //    return sdio_send_command_and_wait(16, 512, SDIO_CMD_WAITRESP_SHORT); // SET_BLOCKLEN
    case 0:
        SDIO_DCTRL = 0;
        SDIO_ICR = SDIO_ICR_STATIC;

        if (SCB_ICSR & SCB_ICSR_VECTACTIVE) {
            return APP_JOB_TASK_QUIT_ISR;
        } else {
            sdio->target_buffer = app_buffer_target(job->actor, data, size);
            for (size_t i = 0; i < 512; i++)
                sdio->target_buffer->data[i] = (uint8_t) i;

            if (sdio->target_buffer == NULL) {
                return APP_JOB_FAILURE;
            } else {
                sdio_dma_rx_start(sdio, sdio->target_buffer->data, size);
                return APP_JOB_TASK_CONTINUE;
            }
        }
    case 1:

        // SDSC card uses byte unit address and
        // SDHC/SDXC cards use block unit address (1 unit = 512 bytes)
        // For SDHC card addr must be converted to block unit address
        if (block_count > 1) {
            return sdio_send_command_and_wait(18, block_address, SDIO_CMD_WAITRESP_SHORT); // READ_MULTIPLE_BLOCK
        } else {
            return sdio_send_command_and_wait(17, block_address, SDIO_CMD_WAITRESP_SHORT); // READ_BLOCK
        }
    case 2:
        // Data transfer:
        //   transfer mode: block
        //   direction: to card
        //   DMA: enabled
        //   block size: 2^9 = 512 bytes
        //   DPSM: enabled
        //if (SCB_ICSR & SCB_ICSR_VECTACTIVE) {
        //    return APP_JOB_TASK_QUIT_ISR;
        //} else {
            SDIO_DTIMER = SD_DATA_READ_TIMEOUT;
            SDIO_DLEN = size;
            SDIO_DCTRL = SDIO_DCTRL_DMAEN | (9 << 4) | SDIO_DCTRL_DTEN | SDIO_DCTRL_DTDIR;
        //}
        return APP_JOB_TASK_CONTINUE;
    case 3:
        return APP_JOB_TASK_WAIT;
    // check errors
    case 4:
        if (SDIO_STA & SDIO_XFER_ERROR_FLAGS) {
            return APP_JOB_TASK_FAILURE;
        } else if (sdio->incoming_signal == APP_SIGNAL_DMA_TRANSFERRING) {
            printf("DMA BUF, ~%s~\n", &sdio->target_buffer->data[480]);
            return APP_JOB_TASK_CONTINUE;
        } else {
            SDIO_ICR = SDIO_ICR_STATIC;

            return APP_JOB_TASK_LOOP;
        }
    // wait while tx is active
    case 5:
        if (SDIO_STA & SDIO_STA_RXACT) {
            return APP_JOB_TASK_LOOP;
        } else {
            sdio_dma_rx_stop(sdio);
            return APP_JOB_TASK_CONTINUE;
        }
    // stop multi-block transfer
    case 6:
        if (block_count > 1) {
            return sdio_send_command_and_wait(12, 0, SDIO_CMD_WAITRESP_SHORT); // STOP_TRANSMISSION
        } else {
            return APP_JOB_TASK_CONTINUE;
        }
    default:
        if (job->task_phase == APP_JOB_FAILURE) {
            printf("Fail\n");
        }
    }
    return APP_JOB_TASK_SUCCESS;
}

static app_job_signal_t sdio_task_detect_card(app_job_t *job) {
    transport_sdio_t *sdio = job->actor->object;
    // Power up the card
    switch (job->task_phase) {
    case 0:
        nvic_enable_irq(sdio->irq);
        sdio_enable();
        return APP_JOB_TASK_CONTINUE;
    case 1:
        return sdio_send_command_and_wait(0, 0, SDIO_CMD_WAITRESP_NO_0); // GO_IDLE_STATE

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
                job->task_phase += 1; // skip next step
                return APP_JOB_TASK_CONTINUE;
            }
            break;
        default:
            return APP_JOB_TASK_FAILURE;
        }

    // V1 cards only: Send 55 reset Illegal Command bit
    case 4:
        return sdio_send_command_and_wait(55, 0, SDIO_CMD_WAITRESP_SHORT); // CMD_APP_CMD

    // Initialize card and wait for ready status (55 + 41)
    case 5:
        return sdio_send_command_and_wait(55, 0, SDIO_CMD_WAITRESP_SHORT); // CMD_APP_CMD
    case 6:
        // MMC not supported: Need to send CMD1 instead of ACMD41
        return sdio_send_command_and_wait(
            41, (SDIO_3_0_to_3_3_V | (sdio->properties->version == 2 ? SDIO_HIGH_CAPACITY : SDIO_STANDARD_CAPACITY)),
            SDIO_CMD_WAITRESP_SHORT);
    case 7:
        if (!(SDIO_RESP1 & (1 << 31))) {
            job->task_phase = 5 - 1;
            return APP_JOB_TASK_CONTINUE; // TODO: retries
        } else {
            if (SDIO_RESP1 & SDIO_HIGH_CAPACITY) {
                transport_sdio_set_high_capacity(sdio, 1);
            }
            return APP_JOB_TASK_CONTINUE;
        }

    // Now the CMD2 and CMD3 commands should be issued in cycle until timeout to enumerate all cards on the bus
    // Since this module suitable to work with single card, issue this commands one time only
    case 8:
        return sdio_send_command_and_wait(2, 0, SDIO_CMD_WAITRESP_LONG); // ALL_SEND_CID
    case 9:
        sdio_parse_cid(sdio);
        printf("│ │ ├ Card name %.*s\n", 3, sdio->properties->product_name);
        if (sdio->properties->product_name[0] == '\0') {
            printf("Bad card\n");
        }
        return APP_JOB_TASK_CONTINUE;

    // Get relative card address
    // MMC not supported:  host should set a RCA value to the card by SET_REL_ADD
    case 10:
        return sdio_send_command_and_wait(3, 0, SDIO_CMD_WAITRESP_SHORT); // SEND_REL_ADDR
    case 11:
        // r6 errors
        if (SDIO_RESP1 & (0x00002000U | 0x00004000U | 0x00008000U)) {
            return APP_JOB_TASK_FAILURE;
        }
        transport_sdio_set_relative_card_address(sdio, SDIO_RESP1 >> 16);
        return APP_JOB_TASK_CONTINUE;

    // Retrieve CSD register:
    case 12:
        return sdio_send_command_and_wait(9, (sdio->properties->relative_card_address << 16), SDIO_CMD_WAITRESP_LONG); // SEND_CSD
    case 13:
        sdio_parse_csd(sdio);
        printf("│ │ ├ Card capacity %luMB\n", sdio->properties->capacity >> 10);
        return APP_JOB_TASK_CONTINUE;

    // Increase bus speed to 48mhz, set card into transfer mode
    case 14:
        sdio_set_bus_clock(SDIO_CLKCR_CLKDIV_TRANSFER);
        return sdio_send_command_and_wait(7, sdio->properties->relative_card_address << 16, SDIO_CMD_WAITRESP_SHORT); // SEL_DESEL_CARD

    // Disable the pull-up resistor on CD/DAT3 pin of card
    case 15:
        return sdio_send_command_and_wait(55, sdio->properties->relative_card_address << 16, SDIO_CMD_WAITRESP_SHORT); // ACMD
    case 16:
        return sdio_send_command_and_wait(42, 0, SDIO_CMD_WAITRESP_SHORT); // SET_CLR_CARD_DETECT
    // Set block size to 512
    case 17:
        return sdio_send_command_and_wait(16, 512, SDIO_CMD_WAITRESP_SHORT); // SET_BLOCKLEN
    case 18:
        return APP_JOB_TASK_SUCCESS;
    // Set bus width to 4
    case 19:
        return sdio_send_command_and_wait(55, sdio->properties->relative_card_address << 16, SDIO_CMD_WAITRESP_SHORT); // ACMD
    case 20:
        return sdio_send_command_and_wait(6, 4, SDIO_CMD_WAITRESP_SHORT); // SET_BUS_WIDTH
    case 21:
        SDIO_CLKCR = ((SDIO_CLKCR & ~SDIO_CLKCR_WIDBUS_1) | (SDIO_CLKCR_WIDBUS_4));
        return APP_JOB_TASK_SUCCESS;
    case APP_JOB_TASK_FAILURE:
    case APP_JOB_TASK_SUCCESS:
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
        return sdio_task_read_block(job, 0, NULL, 512);
        ;
    case 2:
        return APP_JOB_SUCCESS;
    case APP_JOB_FAILURE:
        printf("Fail!\n");
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
