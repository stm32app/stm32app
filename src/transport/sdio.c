#include "sdio.h"
#include "core/buffer.h"
#include "lib/bytes.h"
#include "lib/dma.h"
#include "storage/sdcard.h"
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

uint8_t buff[512] = {1};

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
    if (sdio != NULL && sdio->job != NULL) {
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
    // start with 1-bit width of bus
    // power saving is enabled
    SDIO_CLKCR = SDIO_CLKCR_WIDBUS_1 | SDIO_CLKCR_CLKDIV_INITIAL | SDIO_CLKCR_CLKEN | SDIO_CLKCR_PWRSAV | SDIO_CLKCR_HWFC_EN;
    SDIO_MASK = 0xffffffff & ~(SDIO_MASK_RXDAVLIE) & (~SDIO_MASK_RXACTIE);
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

static void sdio_parse_cid(storage_sdcard_t *sdcard) {
    uint8_t cid[16];
    sdio_read_r2((uint32_t *)&cid);
    storage_sdcard_set_manufacturer_id(sdcard, cid[0]);
    storage_sdcard_set_oem_id(sdcard, bitswap16(cid[1]));
    storage_sdcard_set_product_name(sdcard, (char *)&cid[5], 5);
    storage_sdcard_set_product_revision(sdcard, cid[8]);
    storage_sdcard_set_serial_number(sdcard, bitswap32(cid[9]));
    storage_sdcard_set_manufacturing_date(sdcard, bitswap16(cid[13]));
}

static void sdio_parse_csd(storage_sdcard_t *sdcard) {
    uint8_t csd[16];
    sdio_read_r2((uint32_t *)&csd);
    storage_sdcard_set_max_bus_clock_frequency(sdcard, csd[3]);
    storage_sdcard_set_csd_version(sdcard, csd[0] >> 6);
    if (storage_sdcard_get_csd_version(sdcard) == 0) {
        uint32_t block_size = ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | ((csd[8] & 0xc0) >> 6);
        uint32_t multiplier = (((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7));
        storage_sdcard_set_block_count(sdcard, 1 + block_size * multiplier);
        storage_sdcard_set_block_size(sdcard, csd[5] & 0x0f);
    } else {
        storage_sdcard_set_block_count(sdcard, 1 + (((csd[7] & 0x3f) << 16) | (csd[8] << 8) | csd[9]));
        storage_sdcard_set_block_size(sdcard, 512);
    }
    storage_sdcard_set_capacity(sdcard, storage_sdcard_get_block_count(sdcard) * storage_sdcard_get_block_size(sdcard));
}

static void sdio_set_bus_clock(uint8_t divider) {
    SDIO_CLKCR = (SDIO_CLKCR & (~SDIO_CLKCR_CLKDIV_MASK)) | (divider & SDIO_CLKCR_CLKDIV_MASK);
}

static void sdio_blocking_rx_start(transport_sdio_t *sdio, uint8_t *data, uint32_t size, bool_t force) {
    uint32_t position = 0;
    do {
        if ((SDIO_STA & SDIO_STA_RXDAVL) || force) {
            ((uint32_t *)data)[position] = SDIO_FIFO;
            if (position < size / 4) {
                ++position;
            } else {
                break;
            }
        }
    } while ((SDIO_STA & SDIO_STA_RXACT) || force);
}

static void sdio_dma_rx_start(transport_sdio_t *sdio, uint8_t *data, uint32_t size) {
    actor_register_dma(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->actor);

    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("RX started\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    actor_dma_rx_start((uint32_t)&SDIO_FIFO, sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel, data,
                       size / 4, false, 4, 1, 1);
}

static void sdio_dma_rx_stop(transport_sdio_t *sdio) {
    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("RX stopped\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    sdio->incoming_signal = 0;
    actor_dma_rx_stop(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    // Hack: DMA for some reason doesnt copy last 4 double-words (full SDIO fifo load)
    size_t position =
        actor_dma_get_buffer_position(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->target_buffer->size / 4);

    debug_printf("RX flush %lub\n", (sdio->target_buffer->size - position * 4));
    sdio_blocking_rx_start(sdio, &sdio->target_buffer->data[position * 4], (sdio->target_buffer->size - position * 4), true);

    actor_unregister_dma(sdio->properties->dma_unit, sdio->properties->dma_stream);
}

static void sdio_dma_tx_start(transport_sdio_t *sdio, uint8_t *data, uint32_t size) {
    actor_register_dma(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->actor);

    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("TX started\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    actor_dma_tx_start((uint32_t)&SDIO_FIFO, sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel,
                       sdio->source_buffer->data, sdio->source_buffer->size / 4, false, 4, 0, 0);
}

static void sdio_blocking_tx_start(transport_sdio_t *sdio, uint8_t *data, uint32_t size) {
    uint32_t position = 0;
    do {
        if (SDIO_STA & SDIO_STA_TXFIFOHE) {
            SDIO_FIFO = ((uint32_t *)data)[position];
            if (position < size / 4) {
                ++position;
            }
        }
    } while (SDIO_STA & SDIO_STA_TXACT);
}

static void sdio_dma_tx_stop(transport_sdio_t *sdio) {
    debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
    debug_printf("TX stopped\tDMA%u(%u/%u)\n", sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    actor_dma_tx_stop(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->properties->dma_channel);

    // Hack: DMA for some reason doesnt copy last 4 double-words (full SDIO fifo load)
    size_t position =
        actor_dma_get_buffer_position(sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->source_buffer->size / 4);

    debug_printf("TX flush %lub\n", (sdio->source_buffer->size - position * 4));
    sdio_blocking_tx_start(sdio, &sdio->source_buffer->data[position * 4], (sdio->source_buffer->size - position * 4));
    app_buffer_release(sdio->source_buffer, sdio->actor);

    sdio->incoming_signal = 0;
    actor_unregister_dma(sdio->properties->dma_unit, sdio->properties->dma_stream);
}

static app_job_signal_t sdio_task_erase_block(app_job_t *job, uint32_t block_id, uint32_t block_count) {
    // transport_sdio_t *sdio = job->actor->object;
    storage_sdcard_t *sdcard = job->inciting_event.producer->object;
    uint32_t start_address = storage_sdcard_get_high_capacity(sdcard) ? block_id : block_id * 512;
    uint32_t end_address = (block_id + block_count) * (storage_sdcard_get_high_capacity(sdcard) ? 1 : 512);

    switch (job->task_phase) {
    case 0:
        return sdio_send_command_and_wait(32, start_address, SDIO_CMD_WAITRESP_SHORT); // ERASE_WR_BLK_START_ADDR
    case 1:
        return sdio_send_command_and_wait(33, end_address, SDIO_CMD_WAITRESP_SHORT); // ERASE_WR_BLK_END_ADDR
    case 2:
        return sdio_send_command_and_wait(38, 0, SDIO_CMD_WAITRESP_SHORT); // SDIO_CMD_ERASE
    }
    return APP_JOB_TASK_SUCCESS;
}

static app_job_signal_t sdio_task_write_block(app_job_t *job, uint32_t block_id, uint8_t *data, uint32_t block_count) {
    transport_sdio_t *sdio = job->actor->object;
    storage_sdcard_t *sdcard = job->inciting_event.producer->object;
    uint32_t block_address = storage_sdcard_get_high_capacity(sdcard) ? block_id : block_id * 512;
    uint32_t size = 512 * block_count;

    switch (job->task_phase) {
    case 0:
        if (app_thread_is_interrupted(job->thread)) {
            return APP_JOB_TASK_QUIT_ISR;
        } else {
            // if target buffer is not provided, sdio has ability to allocate buffer aligned to 16b to use dma burst
            sdio->source_buffer = data == NULL ? app_buffer_aligned(job->actor, size, 16) : app_buffer_target(job->actor, data, size);
            sdio->source_buffer->size = size;

            if (sdio->source_buffer == NULL) {
                return APP_JOB_FAILURE;
            } else {
                sdio_dma_tx_start(sdio, sdio->source_buffer->data, size);
                return APP_JOB_TASK_CONTINUE;
            }
        }
    case 1:
        SDIO_DCTRL = 0;
        if (block_count > 1) {
            return sdio_send_command_and_wait(25, block_address, SDIO_CMD_WAITRESP_SHORT); // WRITE_MULTIPLE_BLOCK
        } else {
            return sdio_send_command_and_wait(24, block_address, SDIO_CMD_WAITRESP_SHORT); // WRITE_BLOCK
        }
    case 2:
        // Data transfer:
        //   transfer mode: block
        //   direction: to card
        //   DMA: enabled
        //   block size: 2^9 = 512 bytes
        //   DPSM: enabled
        SDIO_DTIMER = SD_DATA_READ_TIMEOUT;
        SDIO_DLEN = size;
        SDIO_DCTRL = SDIO_DCTRL_DMAEN | (9 << 4) | SDIO_DCTRL_DTEN; //| SDIO_DCTRL_DTDIR;
        return APP_JOB_TASK_CONTINUE;
        break;
    // check errors
    case 3:
        if ((SDIO_STA & SDIO_STA_TXDAVL) && (SDIO_STA & SDIO_STA_TXACT)) {
            sdio_dma_tx_stop(sdio);
            return APP_JOB_TASK_CONTINUE;
        } else if (SDIO_STA & SDIO_XFER_ERROR_FLAGS) {
            return APP_JOB_TASK_FAILURE;
            //} else if (SDIO_STA & (SDIO_STA_DBCKEND |SDIO_STA_DATAEND)) {
            //    return APP_JOB_TASK_SUCCESS;
        } else if (sdio->incoming_signal == APP_SIGNAL_DMA_TRANSFERRING) {
            sdio_dma_tx_stop(sdio);
            SDIO_ICR = SDIO_ICR_STATIC;
            return APP_JOB_TASK_CONTINUE;
        } else {
            SDIO_ICR = SDIO_ICR_STATIC;
            return APP_JOB_TASK_LOOP;
        }
    case 4:
        return APP_JOB_TASK_CONTINUE;
    // wait while tx is active
    case 5:
        if (SDIO_STA & SDIO_STA_RXACT) {
            return APP_JOB_TASK_LOOP;
        } else {
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
            error_printf("Fail\n");
        }
    }
    return APP_JOB_TASK_SUCCESS;
}

static app_job_signal_t sdio_task_read_block(app_job_t *job, uint32_t block_id, uint8_t *data, uint32_t block_count) {
    transport_sdio_t *sdio = job->actor->object;
    storage_sdcard_t *sdcard = job->inciting_event.producer->object;
    uint32_t block_address = storage_sdcard_get_high_capacity(sdcard) ? block_id : block_id * 512;
    uint32_t size = 512 * block_count;

    switch (job->task_phase) {
    case 0:
        if (app_thread_is_interrupted(job->thread)) {
            return APP_JOB_TASK_QUIT_ISR;
        } else {
            // if target buffer is not provided, sdio has ability to allocate buffer aligned to 16b to use dma burst
            sdio->target_buffer = data == NULL ? app_buffer_aligned(job->actor, size, 16) : app_buffer_target(job->actor, data, size);
            sdio->target_buffer->size = size;
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
        SDIO_DTIMER = SD_DATA_READ_TIMEOUT;
        SDIO_DLEN = size;
        SDIO_DCTRL = SDIO_DCTRL_DMAEN | 9 << 4 | SDIO_DCTRL_DTEN | SDIO_DCTRL_DTDIR;

        return APP_JOB_TASK_WAIT;
    // check errors
    case 3:
        if (SDIO_STA & SDIO_XFER_ERROR_FLAGS) {
            return APP_JOB_TASK_FAILURE;
        } else if (sdio->incoming_signal == APP_SIGNAL_DMA_TRANSFERRING || (SDIO_STA & SDIO_STA_DBCKEND)) {
            sdio_dma_rx_stop(sdio);
            SDIO_ICR = SDIO_ICR_STATIC;
            return APP_JOB_TASK_CONTINUE;
        } else {
            return APP_JOB_TASK_LOOP;
        }
    // wait while tx is active
    case 4:
        if (SDIO_STA & SDIO_STA_RXACT) {
            return APP_JOB_TASK_LOOP;
        } else {
            return APP_JOB_TASK_CONTINUE;
        }
    // stop multi-block transfer
    case 5:
        if (block_count > 1) {
            return sdio_send_command_and_wait(12, 0, SDIO_CMD_WAITRESP_SHORT); // STOP_TRANSMISSION
        } else {
            return APP_JOB_TASK_CONTINUE;
        }
    default:
        if (job->task_phase == APP_JOB_FAILURE) {
            error_printf("Fail\n");
        }
    }
    return APP_JOB_TASK_SUCCESS;
}

static app_job_signal_t sdio_task_detect_card(app_job_t *job) {
    transport_sdio_t *sdio = job->actor->object;
    storage_sdcard_t *sdcard = job->inciting_event.producer->object;
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
            storage_sdcard_set_version(sdcard, 1);
            return APP_JOB_TASK_CONTINUE;
        case APP_SIGNAL_OK:
            // sdcard echoed back correctly
            if (SDIO_RESP1 == 0x1F1) {
                storage_sdcard_set_version(sdcard, 2);
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
            41, (SDIO_3_0_to_3_3_V | (storage_sdcard_get_version(sdcard) == 2 ? SDIO_HIGH_CAPACITY : SDIO_STANDARD_CAPACITY)),
            SDIO_CMD_WAITRESP_SHORT);
    case 7:
        if (!(SDIO_RESP1 & (1 << 31))) {
            job->task_phase = 5 - 1;
            return APP_JOB_TASK_CONTINUE; // TODO: retries
        } else {
            if (SDIO_RESP1 & SDIO_HIGH_CAPACITY) {
                storage_sdcard_set_high_capacity(sdcard, 1);
            }
            return APP_JOB_TASK_CONTINUE;
        }

    // Now the CMD2 and CMD3 commands should be issued in cycle until timeout to enumerate all cards on the bus
    // Since this module suitable to work with single card, issue this commands one time only
    case 8:
        return sdio_send_command_and_wait(2, 0, SDIO_CMD_WAITRESP_LONG); // ALL_SEND_CID
    case 9:
        sdio_parse_cid(sdcard);
        error_printf("│ │ ├ Card name %.*s\n", 3, storage_sdcard_get_product_name(sdcard));
        if (storage_sdcard_get_product_name(sdcard)[0] == '\0') {
            error_printf("Bad card\n");
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
        storage_sdcard_set_relative_card_address(sdcard, SDIO_RESP1 >> 16);
        return APP_JOB_TASK_CONTINUE;

    // Retrieve CSD register:
    case 12:
        return sdio_send_command_and_wait(9, (storage_sdcard_get_relative_card_address(sdcard) << 16), SDIO_CMD_WAITRESP_LONG); // SEND_CSD
    case 13:
        sdio_parse_csd(sdcard);
        error_printf("│ │ ├ Card capacity %luMB (%ld x %ldb blocks)\n", storage_sdcard_get_capacity(sdcard) >> 10,
               storage_sdcard_get_block_count(sdcard), storage_sdcard_get_block_size(sdcard));
        return APP_JOB_TASK_CONTINUE;

    // Increase bus speed to 48mhz, set card into transfer mode
    case 14:
        sdio_set_bus_clock(SDIO_CLKCR_CLKDIV_TRANSFER);
        return sdio_send_command_and_wait(7, storage_sdcard_get_relative_card_address(sdcard) << 16,
                                          SDIO_CMD_WAITRESP_SHORT); // SEL_DESEL_CARD

    // Disable the pull-up resistor on CD/DAT3 pin of card
    case 15:
        return sdio_send_command_and_wait(55, storage_sdcard_get_relative_card_address(sdcard) << 16, SDIO_CMD_WAITRESP_SHORT); // ACMD
    case 16:
        return sdio_send_command_and_wait(42, 0, SDIO_CMD_WAITRESP_SHORT); // SET_CLR_CARD_DETECT
    // Force block size to 512
    case 17:
        if (storage_sdcard_get_block_size(sdcard) != 512) {
            return sdio_send_command_and_wait(16, 512, SDIO_CMD_WAITRESP_SHORT); // SET_BLOCKLEN
        } else {
            return APP_JOB_TASK_CONTINUE;
        }
    // Set bus width to 4
    case 18:
        return sdio_send_command_and_wait(55, storage_sdcard_get_relative_card_address(sdcard) << 16, SDIO_CMD_WAITRESP_SHORT); // ACMD
    case 19:
        return sdio_send_command_and_wait(6, 2, SDIO_CMD_WAITRESP_SHORT); // SET_BUS_WIDTH
    // increase bus speed
    case 20:
        SDIO_CLKCR = ((SDIO_CLKCR & ~SDIO_CLKCR_WIDBUS_1) | (SDIO_CLKCR_WIDBUS_4));
        return APP_JOB_TASK_SUCCESS;
    case APP_JOB_TASK_FAILURE:
    case APP_JOB_TASK_SUCCESS:
        break;
    }
    // transport_sdio_t *sdio = job->actor->object;
    return 0;
}

static app_job_signal_t sdio_task_publish_response(app_job_t *job) {
    transport_sdio_t *sdio = job->actor->object;
    switch (job->task_phase) {
    case 0:
        // if event was APP_EVENT_READ_TO_BUFFER, it is assumed that producer expects report instead of event
        if (job->inciting_event.type == APP_EVENT_READ) {
            app_publish(job->actor->app, &((app_event_t){.type = APP_EVENT_RESPONSE,
                                                         .producer = sdio->actor,
                                                         .consumer = job->inciting_event.producer,
                                                         .data = (uint8_t *)sdio->target_buffer,
                                                         .size = APP_BUFFER_DYNAMIC_SIZE}));
        } else {
            app_buffer_release(sdio->target_buffer, job->actor);
        }
        return APP_JOB_TASK_SUCCESS;
    }
    return 0;
}

static app_job_signal_t sdio_job_mount(app_job_t *job) {
    switch (job->job_phase) {
    case 0:
        return sdio_task_detect_card(job);
    default:
        return APP_JOB_SUCCESS;
    }
}

static app_job_signal_t sdio_job_unmount(app_job_t *job) {
    switch (job->job_phase) {
        //   case 0:
        //        return sdio_task_detect_card(job);
    default:
        return APP_JOB_SUCCESS;
    }
}

static app_job_signal_t sdio_job_read(app_job_t *job) {
    switch (job->job_phase) {
    case 0:
        return sdio_task_read_block(job, (uint32_t)job->inciting_event.argument, job->inciting_event.data, job->inciting_event.size);
    case 1:
        return sdio_task_publish_response(job);
    case 2:
        return APP_JOB_SUCCESS;
    case APP_JOB_FAILURE:
        error_printf("Fail!\n");
    }
    return 0;
}

static app_job_signal_t sdio_job_erase(app_job_t *job) {
    switch (job->job_phase) {
    case 0:
        return sdio_task_erase_block(job, (uint32_t)job->inciting_event.argument, job->inciting_event.size);
    case 1:
        return APP_JOB_SUCCESS;
    case APP_JOB_FAILURE:
        error_printf("Fail!\n");
    }
    return 0;
}

static app_job_signal_t sdio_job_write(app_job_t *job) {
    storage_sdcard_t *sdcard = job->inciting_event.producer->object;
    switch (job->job_phase) {
    case 0:
        if (storage_sdcard_get_capacity(sdcard) == 0) {
            return sdio_task_detect_card(job);
        } else {
            return APP_JOB_TASK_SUCCESS;
        }
    case 1:
        return sdio_task_write_block(job, (uint32_t)job->inciting_event.argument, job->inciting_event.data, job->inciting_event.size);
    case 2:
        return APP_JOB_SUCCESS;
    case APP_JOB_FAILURE:
        error_printf("Fail!\n");
    }
    return 0;
}

static app_signal_t sdio_on_high_priority(transport_sdio_t *sdio, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    return app_job_execute_if_running_in_thread(&sdio->job, thread);
}

static app_signal_t sdio_on_input(transport_sdio_t *sdio, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_READ:
    case APP_EVENT_READ_TO_BUFFER:
        return actor_event_handle_and_start_job(sdio->actor, event, &sdio->job, sdio->actor->app->high_priority, sdio_job_read);
    case APP_EVENT_WRITE:
        return actor_event_handle_and_start_job(sdio->actor, event, &sdio->job, sdio->actor->app->high_priority, sdio_job_write);
    case APP_EVENT_ERASE:
        return actor_event_handle_and_start_job(sdio->actor, event, &sdio->job, sdio->actor->app->high_priority, sdio_job_erase);
    case APP_EVENT_MOUNT:
        return actor_event_handle_and_start_job(sdio->actor, event, &sdio->job, sdio->actor->app->high_priority, sdio_job_mount);
    case APP_EVENT_UNMOUNT:
        return actor_event_handle_and_start_job(sdio->actor, event, &sdio->job, sdio->actor->app->high_priority, sdio_job_unmount);
    default:
        break;
    }
    return 0;
}

void sdio_isr(void) {
    sdio_notify(0);
}

static actor_worker_callback_t sdio_on_worker_assignment(transport_sdio_t *sdio, app_thread_t *thread) {
    if (thread == sdio->actor->app->input) {
        return (actor_worker_callback_t)sdio_on_input;
    } else if (thread == sdio->actor->app->high_priority) {
        return (actor_worker_callback_t)sdio_on_high_priority;
    }
    return NULL;
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
    .on_worker_assignment = (actor_on_worker_assignment_t)sdio_on_worker_assignment,
    .property_write = sdio_property_write,
};
