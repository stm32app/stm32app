#include <actor/buffer.h>
#include <actor/lib/bytes.h>
#include <actor/lib/dma.h>
#include <actor/lib/gpio.h>
#include <actor/transport/sdio.h>
#include <libopencm3/stm32/sdio.h>

// heavily inspired by code at
// https://github.com/LonelyWolf/stm32/blob/master/stm32l4-sdio/src/sdcard.c

static actor_signal_t sdio_validate(transport_sdio_properties_t* properties) {
  return 0;
}

uint8_t buff[512] = {1};

static actor_signal_t sdio_construct(transport_sdio_t* sdio) {
  actor_message_subscribe(sdio->actor, ACTOR_MESSAGE_DIAGNOSE);
  sdio->address = SDIO_BASE;

#if defined(RCC_APB1ENR_SDIOEN)
  sdio->source_clock = &RCC_APB1ENR;
  sdio->reset = RCC_APB1RSTR_SDIORST;
  sdio->peripheral_clock = RCC_APB1ENR_SDIOEN;
#elif defined(RCC_APB2ENR_SDIOEN)
  sdio->source_clock = (uint32_t*)&RCC_APB2ENR;
  sdio->reset = RCC_APB2RSTR_SDIORST;
  sdio->peripheral_clock = RCC_APB2ENR_SDIOEN;
#else
  sdio->source_clock = &RCC_AHBENR;
  sdio->peripheral_clock = RCC_AHBENR_SDIOEN;
#endif
  sdio->irq = NVIC_SDIO_IRQ;
  return 0;
}

transport_sdio_t* sdio_units[SDIO_UNITS];

static void sdio_notify(uint8_t index) {
  debug_printf("> SDIO Interrupt\n");
  transport_sdio_t* sdio = sdio_units[index];
  if (sdio != NULL && sdio->job != NULL) {
    actor_job_execute(sdio->job, ACTOR_SIGNAL_OK, sdio->job->actor);
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
  SDIO_CLKCR = SDIO_CLKCR_WIDBUS_1 | SDIO_CLKCR_CLKDIV_INITIAL | SDIO_CLKCR_CLKEN |
               SDIO_CLKCR_PWRSAV | SDIO_CLKCR_HWFC_EN;
  SDIO_MASK = 0xffffffff & ~(SDIO_MASK_RXDAVLIE) & (~SDIO_MASK_RXACTIE);
}

static actor_signal_t sdio_start(transport_sdio_t* sdio) {
  sdio_units[sdio->actor->seq] = sdio;
  rcc_peripheral_enable_clock(sdio->source_clock, sdio->peripheral_clock);
  gpio_configure_af_pullup(sdio->properties->d0_port, sdio->properties->d0_pin, 50,
                           sdio->properties->af);
  gpio_configure_af_pullup(sdio->properties->d1_port, sdio->properties->d1_pin, 50,
                           sdio->properties->af);
  gpio_configure_af_pullup(sdio->properties->d2_port, sdio->properties->d2_pin, 50,
                           sdio->properties->af);
  gpio_configure_af_pullup(sdio->properties->d3_port, sdio->properties->d3_pin, 50,
                           sdio->properties->af);
  gpio_configure_af_pullup(sdio->properties->ck_port, sdio->properties->ck_pin, 50,
                           sdio->properties->af);
  gpio_configure_af_pullup(sdio->properties->cmd_port, sdio->properties->cmd_pin, 50,
                           sdio->properties->af);

  nvic_set_priority(sdio->irq, 10 << 4);

  return 0;
}

static actor_signal_t sdio_stop(transport_sdio_t* sdio) {
  return 0;
}

static actor_signal_t sdio_signal_received(transport_sdio_t* sdio,
                                           actor_signal_t signal,
                                           actor_t* caller,
                                           void* source) {
  switch (signal) {
    // DMA interrupts on buffer filling up half-way or all the way up
    case ACTOR_SIGNAL_DMA_TRANSFERRING:
      actor_job_execute(sdio->job, ACTOR_SIGNAL_DMA_TRANSFERRING, sdio->actor);
      break;
    default:
      break;
  }
  return 0;
}

static actor_signal_t sdio_get_status(uint8_t cmd) {
  uint32_t value = SDIO_STA;
  if (value & SDIO_STA_CCRCFAIL) {
    SDIO_ICR = SDIO_ICR_CCRCFAILC;
    if (cmd == 41) {  // 41 is always corrupted, that's expected
      return ACTOR_SIGNAL_OK;
    }
    return ACTOR_SIGNAL_CORRUPTED;
  } else if (value & SDIO_STA_CTIMEOUT) {
    SDIO_ICR = SDIO_ICR_CTIMEOUTC;
    return ACTOR_SIGNAL_TIMEOUT;
  }

  if (cmd == 0) {
    if (!(value & SDIO_STA_CMDSENT)) {
      return ACTOR_SIGNAL_WAIT;
    }
  } else {
    if ((value & SDIO_STA_CMDACT) || !(value & SDIO_STA_CMDREND)) {
      return ACTOR_SIGNAL_WAIT;
      // only short-wait commands have RESPCMD
    } else if (cmd != 0 && cmd != 2 && cmd != 9 && SDIO_RESPCMD != cmd) {
      return ACTOR_SIGNAL_FAILURE;
    }
  }

  SDIO_ICR = SDIO_ICR_STATIC;

  return ACTOR_SIGNAL_OK;
}

static actor_signal_t sdio_send_command(uint32_t cmd, uint32_t arg, uint32_t wait) {
  SDIO_ICR = SDIO_ICR_CMD;
  SDIO_ARG = arg;
  SDIO_CMD = cmd | SDIO_CMD_CPSMEN | wait;
  return ACTOR_SIGNAL_OK;
}

static actor_signal_t sdio_command(uint32_t cmd, uint32_t arg, uint32_t wait) {
  if (SDIO_STA == 0) {
    sdio_send_command(cmd, arg, wait);
  }
  return sdio_get_status(cmd);
}

static void sdio_read_r2(uint32_t* destination) {
  *destination++ = bitswap32(SDIO_RESP1);
  *destination++ = bitswap32(SDIO_RESP2);
  *destination++ = bitswap32(SDIO_RESP3);
  *destination++ = bitswap32(SDIO_RESP4);
}

static void sdio_parse_cid(actor_t* sdcard) {
  uint8_t cid[16];
  sdio_read_r2((uint32_t*)&cid);
  storage_sdcard_set_manufacturer_id(sdcard, cid[0]);
  storage_sdcard_set_oem_id(sdcard, bitswap16(cid[1]));
  storage_sdcard_set_product_name(sdcard, (char*)&cid[5], 5);
  storage_sdcard_set_product_revision(sdcard, cid[8]);
  storage_sdcard_set_serial_number(sdcard, bitswap32(cid[9]));
  storage_sdcard_set_manufacturing_date(sdcard, bitswap16(cid[13]));
}

static void sdio_parse_csd(actor_t* sdcard) {
  uint8_t csd[16];
  sdio_read_r2((uint32_t*)&csd);
  storage_sdcard_set_max_bus_clock_frequency(sdcard, csd[3]);
  storage_sdcard_set_csd_version(sdcard, csd[0] >> 6);
  if (storage_sdcard_get_csd_version(sdcard) == 0) {
    uint32_t block_size =
        ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | ((csd[8] & 0xc0) >> 6);
    uint32_t multiplier = (((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7));
    storage_sdcard_set_block_count(sdcard, 1 + block_size * multiplier);
    storage_sdcard_set_block_size(sdcard, csd[5] & 0x0f);
  } else {
    storage_sdcard_set_block_count(sdcard, 1 + (((csd[7] & 0x3f) << 16) | (csd[8] << 8) | csd[9]));
    storage_sdcard_set_block_size(sdcard, 512);
  }
  storage_sdcard_set_capacity(
      sdcard, storage_sdcard_get_block_count(sdcard) * storage_sdcard_get_block_size(sdcard));
}

static void sdio_set_bus_clock(uint8_t divider) {
  SDIO_CLKCR = (SDIO_CLKCR & (~SDIO_CLKCR_CLKDIV_MASK)) | (divider & SDIO_CLKCR_CLKDIV_MASK);
}

static void sdio_blocking_rx_start(transport_sdio_t* sdio,
                                   uint8_t* data,
                                   uint32_t size,
                                   bool force) {
  uint32_t position = 0;
  do {
    if ((SDIO_STA & SDIO_STA_RXDAVL) || force) {
      ((uint32_t*)data)[position] = SDIO_FIFO;
      if (position < size / 4) {
        ++position;
      } else {
        break;
      }
    }
  } while ((SDIO_STA & SDIO_STA_RXACT) || force);
}

static actor_signal_t sdio_dma_rx_start(transport_sdio_t* sdio, uint8_t* data, uint32_t size) {
  actor_signal_t status =
      actor_dma_acquire(sdio->actor, sdio->properties->dma_unit, sdio->properties->dma_stream);
  if (status == ACTOR_SIGNAL_UNAFFECTED || status < 0) {
    return status;
  }

  debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
  debug_printf("RX started\tDMA%u(%u/%u)\n", sdio->properties->dma_unit,
               sdio->properties->dma_stream, sdio->properties->dma_channel);

  actor_dma_rx_start((uint32_t)&SDIO_FIFO, sdio->properties->dma_unit, sdio->properties->dma_stream,
                     sdio->properties->dma_channel, data, size / 4, false, 4, 1, 1);
  return ACTOR_SIGNAL_OK;
}

static actor_signal_t sdio_dma_rx_stop(transport_sdio_t* sdio) {
  actor_signal_t status =
      actor_dma_release(sdio->actor, sdio->properties->dma_unit, sdio->properties->dma_stream);
  if (status == ACTOR_SIGNAL_UNAFFECTED || status < 0) {
    return status;
  }

  debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
  debug_printf("RX stopped\tDMA%u(%u/%u)\n", sdio->properties->dma_unit,
               sdio->properties->dma_stream, sdio->properties->dma_channel);

  actor_dma_rx_stop(sdio->properties->dma_unit, sdio->properties->dma_stream,
                    sdio->properties->dma_channel);

  // Hack: DMA for some reason doesnt copy last 4 double-words (full SDIO fifo load)
  size_t position = actor_dma_get_buffer_position(
      sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->target_buffer->size / 4);

  debug_printf("RX flush %lub\n", (sdio->target_buffer->size - position * 4));
  sdio_blocking_rx_start(sdio, &sdio->target_buffer->data[position * 4],
                         (sdio->target_buffer->size - position * 4), true);
  return ACTOR_SIGNAL_OK;
}

static actor_signal_t sdio_dma_tx_start(transport_sdio_t* sdio, uint8_t* data, uint32_t size) {
  actor_signal_t status =
      actor_dma_acquire(sdio->actor, sdio->properties->dma_unit, sdio->properties->dma_stream);
  if (status == ACTOR_SIGNAL_UNAFFECTED || status < 0) {
    return status;
  }

  debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
  debug_printf("TX started\tDMA%u(%u/%u)\n", sdio->properties->dma_unit,
               sdio->properties->dma_stream, sdio->properties->dma_channel);

  actor_dma_tx_start((uint32_t)&SDIO_FIFO, sdio->properties->dma_unit, sdio->properties->dma_stream,
                     sdio->properties->dma_channel, sdio->source_buffer->data,
                     sdio->source_buffer->size / 4, false, 4, 0, 0);
  return ACTOR_SIGNAL_OK;
}

static void sdio_blocking_tx_start(transport_sdio_t* sdio, uint8_t* data, uint32_t size) {
  uint32_t position = 0;
  do {
    if (SDIO_STA & SDIO_STA_TXFIFOHE) {
      SDIO_FIFO = ((uint32_t*)data)[position];
      if (position < size / 4) {
        ++position;
      }
    }
  } while (SDIO_STA & SDIO_STA_TXACT);
}

static actor_signal_t sdio_dma_tx_stop(transport_sdio_t* sdio) {
  actor_signal_t status =
      actor_dma_release(sdio->actor, sdio->properties->dma_unit, sdio->properties->dma_stream);
  if (status == ACTOR_SIGNAL_UNAFFECTED || status < 0) {
    return status;
  }
  debug_printf("│ │ ├ SDIO%u\t\t", sdio->actor->seq + 1);
  debug_printf("TX stopped\tDMA%u(%u/%u)\n", sdio->properties->dma_unit,
               sdio->properties->dma_stream, sdio->properties->dma_channel);

  actor_dma_tx_stop(sdio->properties->dma_unit, sdio->properties->dma_stream,
                    sdio->properties->dma_channel);

  // Hack: DMA for some reason doesnt copy last 4 double-words (full SDIO fifo load)
  size_t position = actor_dma_get_buffer_position(
      sdio->properties->dma_unit, sdio->properties->dma_stream, sdio->source_buffer->size / 4);

  debug_printf("TX flush %lub\n", (sdio->source_buffer->size - position * 4));
  sdio_blocking_tx_start(sdio, &sdio->source_buffer->data[position * 4],
                         (sdio->source_buffer->size - position * 4));

  return ACTOR_SIGNAL_OK;
}

static actor_signal_t sdio_task_erase_block(actor_job_t* job,
                                            actor_signal_t signal,
                                            actor_t* caller,
                                            uint32_t block_id,
                                            uint32_t block_count) {
  actor_t* sdcard = job->inciting_message.producer;
  uint32_t start_address = storage_sdcard_get_high_capacity(sdcard) ? block_id : block_id * 512;
  uint32_t end_address =
      (block_id + block_count) * (storage_sdcard_get_high_capacity(sdcard) ? 1 : 512);

  actor_async_task_begin();
  actor_async_await(
      sdio_command(/*ERASE_WR_BLK_START_ADDR*/ 32, start_address, SDIO_CMD_WAITRESP_SHORT));
  actor_async_await(
      sdio_command(/*ERASE_WR_BLK_END_ADDR*/ 33, end_address, SDIO_CMD_WAITRESP_SHORT));
  actor_async_await(sdio_command(/*SDIO_CMD_ERASE*/ 38, 0, SDIO_CMD_WAITRESP_SHORT));
  actor_async_task_end();
}

static actor_signal_t sdio_task_write_block(actor_job_t* job,
                                            actor_signal_t signal,
                                            actor_t* caller,
                                            uint32_t block_id,
                                            uint8_t* data,
                                            uint32_t block_count) {
  transport_sdio_t* sdio = job->actor->object;
  actor_t* sdcard = job->inciting_message.producer;
  uint32_t block_address = storage_sdcard_get_high_capacity(sdcard) ? block_id : block_id * 512;
  uint32_t size = 512 * block_count;

  actor_async_task_begin();

  // Allocate source buffer (requires switching to thread). If buffer was not provided explicitly,
  // the allocated buffer will be aligned to 16b so DMA burst can work
  actor_async_await(actor_job_switch_thread(job, job->thread));
  sdio->source_buffer = data == NULL ? actor_buffer_aligned(job->actor, size, 16)
                                     : actor_buffer_target(job->actor, data, size);
  actor_async_assert(sdio->source_buffer, ACTOR_SIGNAL_OUT_OF_MEMORY);
  sdio->source_buffer->size = size;
                                 actor_async_defer(buffer, {
    actor_buffer_release(sdio->source_buffer, sdio->actor);
                                 });
  // Initialize DMA transmission
  sdio_dma_tx_start(sdio, sdio->source_buffer->data, size);
  //SDIO_DCTRL = 0;
  actor_async_await(sdio_command(block_count > 1 ? /*WRITE_MULTIPLE_BLOCK*/ 25 : /*WRITE_BLOCK*/ 24,
                                 block_address, SDIO_CMD_WAITRESP_SHORT));
                                 actor_async_defer(dma, {

    sdio_dma_tx_stop(sdio);
    SDIO_DCTRL = 0;
    SDIO_ICR = SDIO_ICR_STATIC;
                                 })

  // Write data
  SDIO_DTIMER = SD_DATA_READ_TIMEOUT;
  SDIO_DLEN = size;
  SDIO_DCTRL = SDIO_DCTRL_DMAEN    // using dma
               | (9 << 4)          // 512 bytes block size
               | SDIO_DCTRL_DTEN;  // enabled

  // wait for transmission completion, and handle failures
  while (!((SDIO_STA & SDIO_STA_TXDAVL) && (SDIO_STA & SDIO_STA_TXACT)) &&
         signal != ACTOR_SIGNAL_DMA_TRANSFERRING) {
    if (SDIO_STA & SDIO_XFER_ERROR_FLAGS) {
      actor_async_exit(ACTOR_SIGNAL_TRANSFER_FAILED);
    } else {
      SDIO_ICR = SDIO_ICR_STATIC;
      actor_async_sleep();
    }
  }

  // release DMA
  actor_async_while(SDIO_STA & SDIO_STA_TXACT);
  sdio_dma_tx_stop(sdio);
  SDIO_ICR = SDIO_ICR_STATIC;

  // stop multi-block transfer
  if (block_count > 1) {
    actor_async_await(sdio_command(/*STOP_TRANSMISSION*/ 12, 0, SDIO_CMD_WAITRESP_SHORT));
  }

  actor_async_task_end({
    actor_async_cleanup(dma);
    actor_async_cleanup(buffer);
  });
}

static actor_signal_t sdio_task_read_block(actor_job_t* job,
                                           actor_signal_t signal,
                                           actor_t* caller,
                                           uint32_t block_id,
                                           uint8_t* data,
                                           uint32_t block_count) {
  transport_sdio_t* sdio = job->actor->object;
  actor_t* sdcard = job->inciting_message.producer;
  uint32_t block_address = storage_sdcard_get_high_capacity(sdcard) ? block_id : block_id * 512;
  uint32_t size = 512 * block_count;

  actor_async_task_begin();

  // Allocate buffer wrapper, hint to get
  actor_async_await(actor_job_switch_thread(job, job->thread), .timeout=100);
  sdio->target_buffer = data == NULL ? actor_buffer_aligned(job->actor, size, 16)
                                     : actor_buffer_target(job->actor, data, size);
  sdio->target_buffer->size = size;
  actor_async_assert(sdio->target_buffer, ACTOR_SIGNAL_OUT_OF_MEMORY);
  actor_async_defer(buffer, {
    actor_buffer_release(sdio->source_buffer, sdio->actor);
  });

  // Set up read operation 
  sdio_dma_rx_start(sdio, sdio->target_buffer->data, size);
  actor_async_defer(dma, {
    sdio_dma_rx_stop(sdio);
    SDIO_ICR = SDIO_ICR_STATIC;
    SDIO_DCTRL = 0;
  });
  
  actor_async_await(sdio_command(block_count > 1 ? /*READ_MULTIPLE_BLOCK*/ 18 : /*READ_BLOCK*/ 17,
                                 block_address, SDIO_CMD_WAITRESP_SHORT));
  actor_async_defer(cmd, {
    if (block_count > 1) {
      actor_async_await(sdio_command(/*STOP_TRANSMISSION*/ 12, 0, SDIO_CMD_WAITRESP_SHORT));
    }
  });


  // Kick off data read
  SDIO_DTIMER = SD_DATA_READ_TIMEOUT;
  SDIO_DLEN = size;
  SDIO_DCTRL = SDIO_DCTRL_DMAEN     // using dma
               | 9 << 4             // 512 bytes
               | SDIO_DCTRL_DTEN    // enable transfer
               | SDIO_DCTRL_DTDIR;  // from card

  actor_async_cleanup(dma);
  
  // Listen for end of transmission and handle errors
  while (signal != ACTOR_SIGNAL_DMA_TRANSFERRING && !(SDIO_STA & SDIO_STA_DBCKEND)) {
    if (SDIO_STA & SDIO_XFER_ERROR_FLAGS) {
      actor_async_exit(ACTOR_SIGNAL_TRANSFER_FAILED);
    } else {
      actor_async_sleep();
    }
  }
  actor_async_task_end({
    actor_async_cleanup(dma);
    actor_async_cleanup(cmd);
    actor_async_cleanup(buffer);
  });
}

static actor_signal_t sdio_task_detect_card(actor_job_t* job,
                                            actor_signal_t signal,
                                            actor_t* caller) {
  transport_sdio_t* sdio = job->actor->object;
  actor_t* sdcard = job->inciting_message.producer;

  actor_async_task_begin();
  // Skip initialization if card is already initialized
  if (storage_sdcard_get_capacity(sdcard) > 0) {
    actor_async_exit(ACTOR_SIGNAL_UNAFFECTED);
  }

  nvic_enable_irq(sdio->irq);
  sdio_enable();


  actor_async_await(sdio_command(/*GO_IDLE_STATE*/ 0, 0, SDIO_CMD_WAITRESP_NO_0));

  actor_async_await(sdio_command(/*SEND_IF_COND*/ 8, 0x1F1, SDIO_CMD_WAITRESP_SHORT), .catch=true);
  if (actor_async_error == ACTOR_SIGNAL_TIMEOUT) {
    // V1: cards dont recognize command 8, need command 55 to reset illegal command bit
    storage_sdcard_set_version(sdcard, 1);
    actor_async_await(sdio_command(/*CMD_ACTOR_CMD*/ 55, 0, SDIO_CMD_WAITRESP_SHORT));
  } else if (!actor_async_error) {
    // V2: sdcard echoed back correctly
    if (SDIO_RESP1 == 0x1F1) {
      storage_sdcard_set_version(sdcard, 2);
    } else {
      actor_async_exit(ACTOR_SIGNAL_UNSUPPORTED);
    }
  } else {
    actor_async_exit(actor_async_error);
  }

  do {
    // Initialize card and wait for ready status (55 + 41)
    actor_async_await(sdio_command(/*CMD_ACTOR_CMD*/ 55, 0, SDIO_CMD_WAITRESP_SHORT));

    actor_async_await(sdio_command(
        /*CMD_SD_SEND_OP_COND*/ 41,
        (SDIO_3_0_to_3_3_V |
         (storage_sdcard_get_version(sdcard) == 2 ? SDIO_HIGH_CAPACITY : SDIO_STANDARD_CAPACITY)),
        SDIO_CMD_WAITRESP_SHORT), .catch=true);

    // MMC Cards will time out and fail this:
    if (actor_async_error == ACTOR_SIGNAL_TIMEOUT) {
      return actor_async_exit(ACTOR_SIGNAL_UNSUPPORTED);
    } else if (actor_async_error) {
      return actor_async_error;
    }

    if (SDIO_RESP1 & SDIO_HIGH_CAPACITY) {
      storage_sdcard_set_high_capacity(sdcard, 1);
    }
  } while (!(SDIO_RESP1 & (1 << 31)));

  // Now the CMD2 and CMD3 commands should be issued in cycle until timeout to enumerate all
  // cards on the bus. Since this module suitable to work with single card, issue this commands
  // one time only
  actor_async_await(sdio_command(/*ALL_SEND_CID*/ 2, 0, SDIO_CMD_WAITRESP_LONG));

  // Parse card information
  sdio_parse_cid(sdcard);
  error_printf("│ │ ├ Card name %.*s\n", 3, storage_sdcard_get_product_name(sdcard));
  if (storage_sdcard_get_product_name(sdcard)[0] == 0) {
    error_printf("Bad card\n");
  }

  // Get relative card address
  actor_async_await(sdio_command(/*SEND_REL_ADDR*/ 3, 0, SDIO_CMD_WAITRESP_SHORT));

  // r6 errors
  if (SDIO_RESP1 & (0x00002000U | 0x00004000U | 0x00008000U)) {
    actor_async_exit(ACTOR_SIGNAL_UNSUPPORTED);
  }
  storage_sdcard_set_relative_card_address(sdcard, SDIO_RESP1 >> 16);

  // Retrieve CSD register:
  actor_async_await(sdio_command(
      /*SEND_CSD*/ 9, (storage_sdcard_get_relative_card_address(sdcard) << 16),
      SDIO_CMD_WAITRESP_LONG));
  sdio_parse_csd(sdcard);
  error_printf("│ │ ├ Card capacity %luMB (%ld x %ldb blocks)\n",
               storage_sdcard_get_capacity(sdcard) >> 10, storage_sdcard_get_block_count(sdcard),
               storage_sdcard_get_block_size(sdcard));

  // Increase bus speed to 48mhz, set card into transfer mode
  sdio_set_bus_clock(SDIO_CLKCR_CLKDIV_TRANSFER);
  actor_async_await(sdio_command(
      /*SEL_DESEL_CARD*/ 7, storage_sdcard_get_relative_card_address(sdcard) << 16,
      SDIO_CMD_WAITRESP_SHORT));

  // Disable the pull-up resistor on CD/DAT3 pin of card
  actor_async_await(sdio_command(
      /*ACMD*/ 55, storage_sdcard_get_relative_card_address(sdcard) << 16,
      SDIO_CMD_WAITRESP_SHORT));
  actor_async_await(sdio_command(/*SET_CLR_CARD_DETECT*/ 42, 0, SDIO_CMD_WAITRESP_SHORT));

  // Force block size to 512
  if (storage_sdcard_get_block_size(sdcard) != 512) {
    actor_async_await(sdio_command(/*SET_BLOCKLEN*/ 16, 512, SDIO_CMD_WAITRESP_SHORT))
  }

  // Set bus width to 4
  actor_async_await(sdio_command(
      /*ACMD*/ 55, storage_sdcard_get_relative_card_address(sdcard) << 16,
      SDIO_CMD_WAITRESP_SHORT));
  actor_async_await(sdio_command(/*SET_BUS_WIDTH*/ 6, 2, SDIO_CMD_WAITRESP_SHORT));

  // Increase bus speed
  SDIO_CLKCR = ((SDIO_CLKCR & ~SDIO_CLKCR_WIDBUS_1) | (SDIO_CLKCR_WIDBUS_4));
  actor_async_exit(ACTOR_SIGNAL_TASK_COMPLETE);
  actor_async_task_end();
}

static actor_signal_t sdio_task_publish_response(actor_job_t* job,
                                                 actor_signal_t signal,
                                                 actor_t* caller) {
  transport_sdio_t* sdio = job->actor->object;
  actor_async_task_begin();
  // Actors issuing ACTOR_MESSAGE_READ_TO_BUFFER recieve acknowledgement via message finalization
  if (job->inciting_message.type == ACTOR_MESSAGE_READ_TO_BUFFER) {
    actor_buffer_release(sdio->target_buffer, job->actor);
  } else {
    actor_publish(job->actor, &((actor_message_t){.type = ACTOR_MESSAGE_RESPONSE,
                                                  .producer = sdio->actor,
                                                  .consumer = job->inciting_message.producer,
                                                  .data = (uint8_t*)sdio->target_buffer,
                                                  .size = ACTOR_BUFFER_DYNAMIC_SIZE}));
  }
  actor_async_task_end();
}

static actor_signal_t sdio_job_mount(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(sdio_task_detect_card(job, signal, caller));
  actor_async_job_end();
}

static actor_signal_t sdio_job_unmount(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_t* sdcard = job->inciting_message.producer;
  actor_async_job_begin();
  storage_sdcard_set_manufacturer_id(sdcard, 0);
  storage_sdcard_set_oem_id(sdcard, 0);
  storage_sdcard_set_product_name(sdcard, "", 0);
  storage_sdcard_set_product_revision(sdcard, 0);
  storage_sdcard_set_serial_number(sdcard, 0);
  storage_sdcard_set_manufacturing_date(sdcard, 0);
  storage_sdcard_set_max_bus_clock_frequency(sdcard, 0);
  storage_sdcard_set_csd_version(sdcard, 0);
  storage_sdcard_set_block_count(sdcard, 0);
  storage_sdcard_set_block_size(sdcard, 0);
  storage_sdcard_set_capacity(sdcard, 0);
  actor_async_job_end();
}

static actor_signal_t sdio_job_read(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(sdio_task_detect_card(job, signal, caller));
  actor_async_await(sdio_task_read_block(job, signal, caller,
                                         (uint32_t)job->inciting_message.argument,
                                         job->inciting_message.data, job->inciting_message.size));
  actor_async_await(sdio_task_publish_response(job, signal, caller));
  actor_async_job_end();
}

static actor_signal_t sdio_job_erase(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(sdio_task_detect_card(job, signal, caller));
  actor_async_await(sdio_task_erase_block(
      job, signal, caller, (uint32_t)job->inciting_message.argument, job->inciting_message.size));
  actor_async_await(sdio_task_publish_response(job, signal, caller));
  actor_async_job_end();
}

static actor_signal_t sdio_job_write(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(sdio_task_detect_card(job, signal, caller));
  actor_async_await(sdio_task_write_block(job, signal, caller,
                                          (uint32_t)job->inciting_message.argument,
                                          job->inciting_message.data, job->inciting_message.size));
  actor_async_job_end();
}

static actor_signal_t sdio_on_high_priority(transport_sdio_t* sdio,
                                            actor_message_t* event,
                                            actor_worker_t* tick,
                                            actor_thread_t* thread) {
  return actor_job_execute_if_running_in_thread(sdio->job, thread);
}

static actor_signal_t sdio_on_input(transport_sdio_t* sdio,
                                    actor_message_t* event,
                                    actor_worker_t* tick,
                                    actor_thread_t* thread) {
  switch (event->type) {
    case ACTOR_MESSAGE_READ:
    case ACTOR_MESSAGE_READ_TO_BUFFER:
      return actor_message_handle_and_start_job(sdio->actor, event, &sdio->job,
                                                sdio->actor->node->high_priority, sdio_job_read);
    case ACTOR_MESSAGE_WRITE:
      return actor_message_handle_and_start_job(sdio->actor, event, &sdio->job,
                                                sdio->actor->node->high_priority, sdio_job_write);
    case ACTOR_MESSAGE_ERASE:
      return actor_message_handle_and_start_job(sdio->actor, event, &sdio->job,
                                                sdio->actor->node->high_priority, sdio_job_erase);
    case ACTOR_MESSAGE_MOUNT:
      return actor_message_handle_and_start_job(sdio->actor, event, &sdio->job,
                                                sdio->actor->node->high_priority, sdio_job_mount);
    case ACTOR_MESSAGE_UNMOUNT:
      return actor_message_handle_and_start_job(sdio->actor, event, &sdio->job,
                                                sdio->actor->node->high_priority, sdio_job_unmount);
    default:
      break;
  }
  return 0;
}

void sdio_isr(void) {
  sdio_notify(0);
}

static actor_worker_callback_t sdio_worker_assignment(transport_sdio_t* sdio,
                                                      actor_thread_t* thread) {
  if (thread == sdio->actor->node->input) {
    return (actor_worker_callback_t)sdio_on_input;
  } else if (thread == sdio->actor->node->high_priority) {
    return (actor_worker_callback_t)sdio_on_high_priority;
  }
  return NULL;
}

static actor_signal_t sdio_phase_changed(transport_sdio_t* sdio, actor_phase_t phase) {
  switch (phase) {
    case ACTOR_PHASE_CONSTRUCTION:
      return sdio_construct(sdio);
    case ACTOR_PHASE_START:
      return sdio_start(sdio);
    case ACTOR_PHASE_STOP:
      return sdio_stop(sdio);
    default:
      break;
  }
  return 0;
}

actor_interface_t transport_sdio_class = {
    .type = TRANSPORT_SDIO,
    .size = sizeof(transport_sdio_t),
    .phase_subindex = TRANSPORT_SDIO_PHASE,
    .validate = (actor_method_t)sdio_validate,
    .phase_changed = (actor_phase_changed_t)sdio_phase_changed,
    .signal_received = (actor_signal_received_t)sdio_signal_received,
    .worker_assignment = (actor_worker_assignment_t)sdio_worker_assignment,
};
