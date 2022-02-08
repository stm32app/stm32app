#include "i2c.h"
#include "lib/dma.h"
#include "system/mcu.h"
#include <libopencm3/stm32/i2c.h>

#define I2C_EVENT_MASTER_MODE_SELECT(I2CX) ((I2C_SR1(I2CX) & (I2C_SR1_SB)) == (I2C_SR1_SB))
#define I2C_EVENT_MASTER_MODE_ADDRESS10(I2CX)                                                                                              \
    ((((I2C_SR1(I2CX) & (I2C_SR1_ADD10)) == (I2C_SR1_ADD10)) &&                                                                            \
      (I2C_SR2(I2CX) & (I2C_SR2_BUSY | I2C_SR2_MSL)) == (I2C_SR2_BUSY | I2C_SR2_MSL)))
#define I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED(I2CX)                                                                                   \
    ((((I2C_SR1(I2CX) & (I2C_SR1_TxE | I2C_SR1_ADDR)) == (I2C_SR1_TxE | I2C_SR1_ADDR)) &&                                                  \
      (I2C_SR2(I2CX) & (I2C_SR2_BUSY | I2C_SR2_MSL | I2C_SR2_TRA)) == (I2C_SR2_BUSY | I2C_SR2_MSL | I2C_SR2_TRA)))
#define I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED(I2CX) ((I2C_SR2(I2CX) & (I2C_SR2_BUSY | I2C_SR2_MSL)) == (I2C_SR2_BUSY | I2C_SR2_MSL))
#define I2C_EVENT_TXE(I2CX) ((I2C_SR1(I2CX) & (I2C_SR1_TxE)) == (I2C_SR1_TxE))
#define I2C_EVENT_BTF(I2CX) ((I2C_SR1(I2CX) & (I2C_SR1_BTF)) == (I2C_SR1_BTF))
#define I2C_EVENT_TIMEOUT(I2CX) (I2C_SR1(I2CX) & (I2C_SR1_TIMEOUT) == (I2C_SR1_TIMEOUT))
#define I2C_EVENT_ERROR(I2CX)                                                                                                              \
    (I2C_SR1(I2CX) & (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | I2C_SR1_OVR | I2C_SR1_PECERR | I2C_SR1_TIMEOUT | I2C_SR1_SMBALERT))

static ODR_t i2c_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    transport_i2c_t *i2c = stream->object;
    (void)i2c;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}
static app_signal_t i2c_validate(transport_i2c_properties_t *properties) {
    (void)properties;
    return 0;
}

static void i2c_dma_tx_start(transport_i2c_t *i2c, uint8_t *data, uint32_t size) {
    actor_register_dma(i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream, i2c->actor);

    log_printf("   > I2C%u\t", i2c->actor->seq + 1);
    log_printf("TX started\tDMA%u(%u/%u)\n", i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream, i2c->properties->dma_tx_channel);

    actor_dma_tx_start((uint32_t) & (I2C_DR(i2c->address)), i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream,
                       i2c->properties->dma_tx_channel, data, size, false);

    volatile uint32_t s1 = I2C_SR1(i2c->address);
    volatile uint32_t s2 = I2C_SR2(i2c->address);
    i2c_enable_dma(i2c->address);
}

static void i2c_dma_tx_stop(transport_i2c_t *i2c) {
    log_printf("   > I2C%u\t", i2c->actor->seq + 1);
    log_printf("TX stopped\tDMA%u(%u/%u)\n", i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream, i2c->properties->dma_tx_channel);

    i2c->incoming_signal = 0;
    actor_dma_tx_stop(i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream, i2c->properties->dma_tx_channel);
    actor_unregister_dma(i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream);
}

static void i2c_dma_rx_start(transport_i2c_t *i2c, uint8_t *data, uint32_t size) {
    actor_register_dma(i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream, i2c->actor);

    log_printf("   > I2C%u\t", i2c->actor->seq + 1);
    log_printf("RX started\tDMA%u(%u/%u)\n", i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream, i2c->properties->dma_rx_channel);

    i2c_enable_dma(i2c->address);
    app_double_buffer_start(&i2c->read, data, size);

    actor_dma_rx_start((uint32_t) & (I2C_DR(i2c->address)), i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream,
                       i2c->properties->dma_rx_channel, app_double_buffer_get_buffer(&i2c->read), app_double_buffer_get_sufficent_buffer_size(&i2c->read),
                       app_double_buffer_uses_ring_buffer(&i2c->read));
}

static void i2c_dma_rx_stop(transport_i2c_t *i2c) {
    log_printf("   > I2C%u\t", i2c->actor->seq + 1);
    log_printf("RX stopped\tDMA%u(%u/%u)\n", i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream, i2c->properties->dma_rx_channel);

    i2c->incoming_signal = 0;
    actor_dma_rx_stop(i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream, i2c->properties->dma_rx_channel);
    actor_unregister_dma(i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream);
}

static app_signal_t i2c_construct(transport_i2c_t *i2c) {
    i2c->read.growable_buffer_initial_size = i2c->properties->rx_pool_initial_size;
    i2c->read.growable_buffer.block_size = i2c->properties->rx_pool_block_size;
    i2c->read.growable_buffer.max_size = i2c->properties->rx_pool_max_size;
    i2c->read.ring_buffer_size = i2c->properties->dma_rx_circular_buffer_size;

    switch (i2c->actor->seq) {
    case 0:
        i2c->clock = RCC_I2C1;
        i2c->address = I2C1;
        i2c->ev_irq = NVIC_I2C1_EV_IRQ;
        i2c->er_irq = NVIC_I2C1_ER_IRQ;
        break;
#ifdef I2C2_BASE
    case 1:
        i2c->clock = RCC_I2C2;
        i2c->address = I2C2;
        i2c->ev_irq = NVIC_I2C2_EV_IRQ;
        i2c->er_irq = NVIC_I2C2_ER_IRQ;
        break;
#else
        return 1;
#endif
#ifdef I2C3_BASE
    case 2:
        i2c->clock = RCC_I2C3;
        i2c->address = I2C3;
        i2c->ev_irq = NVIC_I2C3_EV_IRQ;
        i2c->er_irq = NVIC_I2C3_ER_IRQ;
        break;
#else
        return 1;
#endif
    }
    return 0;
}

transport_i2c_t *i2c_units[I2C_UNITS] = {};

static app_signal_t i2c_start(transport_i2c_t *i2c) {
    log_printf("    > I2C%i SCL", i2c->actor->seq + 1);
    gpio_configure_af_opendrain(i2c->properties->scl_port, i2c->properties->scl_pin, GPIO_FAST, i2c->properties->af);
    log_printf("    > I2C%i SMBA", i2c->actor->seq + 1);
    gpio_configure_af_opendrain(i2c->properties->smba_port, i2c->properties->smba_pin, GPIO_FAST, i2c->properties->af);
    log_printf("    > I2C%i SDA", i2c->actor->seq + 1);
    gpio_configure_af_opendrain(i2c->properties->sda_port, i2c->properties->sda_pin, GPIO_FAST, i2c->properties->af);

    rcc_periph_clock_enable(i2c->clock);
    i2c_peripheral_disable(i2c->address); /* disable i2c during setup */
    i2c_reset(i2c->address);
    i2c_clear_stop(i2c->address);

    i2c_set_fast_mode(i2c->address);
    // i2c_set_clock_frequency(i2c->address, i2c->properties->frequency);

    i2c_set_clock_frequency(i2c->address, 42);
    i2c_set_ccr(i2c->address, 35);
    i2c_set_trise(i2c->address, 43);
    // i2c_set_speed(i2c->address, i2c_speed_sm_100k, i2c->actor->app->mcu->clock->apb1_frequency / 1000000);

    // i2c_set_ccr(i2c->address, 35);
    // i2c_set_trise(i2c->address, 43);

    i2c_peripheral_enable(i2c->address);

    i2c_set_own_7bit_slave_address(i2c->address, i2c->properties->slave_address);
    i2c_units[i2c->actor->seq] = i2c;
    i2c_enable_dma(i2c->address);
    return 0;
}

static app_signal_t i2c_stop(transport_i2c_t *i2c) {
    i2c_send_stop(i2c->address);
    i2c_reset(i2c->address);
    i2c_peripheral_disable(i2c->address);
    return 0;
}

static app_signal_t i2c_link(transport_i2c_t *i2c) {
    (void)i2c;
    return 0;
}

static app_signal_t i2c_phase(transport_i2c_t *i2c, actor_phase_t phase) {
    (void)i2c;
    (void)phase;
    return 0;
}

static app_signal_t i2c_handle_error(app_task_t *task) {
    transport_i2c_t *i2c = task->actor->object;
    if (I2C_EVENT_ERROR(i2c->address)) {
        if (i2c->task_retries > 3) {
            return APP_TASK_FAILURE;
        } else {
            i2c->task_retries++;
            return APP_TASK_RETRY;
        }
    }
    return 0;
}

void *i2c_pack_event_argument(uint8_t slave_address, uint16_t memory_address) {
    transport_i2c_event_argument_t address =
        (transport_i2c_event_argument_t){.slave_address = slave_address, .memory_address = memory_address};
    return (void *)*((uint32_t *)&address);
}

transport_i2c_event_argument_t *i2c_unpack_event_argument(void **argument) {
    return (transport_i2c_event_argument_t *)argument;
}

static app_task_signal_t i2c_step_setup(app_task_t *task, uint8_t address, uint32_t memory_register) {
    transport_i2c_t *i2c = task->actor->object;
    switch (task->step_index) {
    case 0:
        i2c_disable_ack(i2c->address);
        i2c_peripheral_enable(i2c->address);
        i2c_enable_interrupt(i2c->address, I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
        nvic_enable_irq(i2c->ev_irq);
        nvic_set_priority(i2c->ev_irq, 5);
        nvic_enable_irq(i2c->er_irq);
        i2c_set_dma_last_transfer(i2c->address);
        i2c_send_start(i2c->address);
        return APP_TASK_STEP_WAIT;
    case 1:
        if (!I2C_EVENT_MASTER_MODE_SELECT(i2c->address)) {
            return APP_TASK_STEP_LOOP;
        } else {
            return APP_TASK_STEP_CONTINUE;
        }
    case 2:
        if (!I2C_EVENT_MASTER_MODE_SELECT(i2c->address)) {
            return APP_TASK_STEP_LOOP;
        } else {
            i2c_send_7bit_address(i2c->address, address, I2C_WRITE);
            return APP_TASK_STEP_WAIT;
        }
    case 3:
        if (!I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED(i2c->address) && !I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED(i2c->address)) {
            return APP_TASK_STEP_LOOP;
        } else {
            I2C_DR(i2c->address) = memory_register;
            return APP_TASK_STEP_CONTINUE;
        }
    default:
        if (!I2C_EVENT_TXE(i2c->address)) {
            return APP_TASK_STEP_LOOP;
        } else {
            return APP_TASK_STEP_COMPLETE;
        }
    }
}

static app_task_signal_t i2c_step_write_data(app_task_t *task, uint8_t address, uint8_t *data, uint32_t size) {
    (void)address;
    transport_i2c_t *i2c = task->actor->object;
    switch (task->step_index) {
    case 0:
        i2c_dma_tx_start(i2c, data, size);
        return APP_TASK_STEP_CONTINUE;
    case 1:
        if (i2c->incoming_signal != APP_SIGNAL_DMA_TRANSFERRING) {
            return APP_TASK_STEP_LOOP;
        } else {
            i2c_dma_tx_stop(i2c);
            return APP_TASK_STEP_CONTINUE;
        }
    default:
        if (!I2C_EVENT_BTF(i2c->address) && !I2C_EVENT_TXE(i2c->address)) {
            return APP_TASK_STEP_LOOP;
        } else {
            i2c_send_stop(i2c->address);
            i2c_peripheral_disable(i2c->address);
            return APP_TASK_STEP_COMPLETE;
        }
    }
}

static app_task_signal_t i2c_step_read_data(app_task_t *task, uint8_t address, uint8_t *data, uint32_t size) {
    transport_i2c_t *i2c = task->actor->object;
    switch (task->step_index) {
    case 0:
        i2c_enable_ack(i2c->address);
        i2c_send_start(i2c->address);
        return APP_TASK_STEP_CONTINUE;
    case 1:
        if (!I2C_EVENT_MASTER_MODE_SELECT(i2c->address)) {
            return APP_TASK_STEP_LOOP;
        } else {
            i2c_send_7bit_address(i2c->address, address, I2C_READ);
            return APP_TASK_STEP_CONTINUE;
        }
    case 2:
        if (!I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED(i2c->address)) {
            return APP_TASK_STEP_LOOP;
        } else {
            i2c_dma_rx_start(i2c, data, size);
            return APP_TASK_STEP_WAIT;
        }
    default:
        if (i2c->incoming_signal != APP_SIGNAL_DMA_TRANSFERRING) {
            return APP_TASK_STEP_LOOP;
        } else {
            i2c_dma_rx_stop(i2c);
            i2c_send_stop(i2c->address);
            i2c_peripheral_disable(i2c->address);
            return APP_TASK_STEP_COMPLETE;
        }
    }
}

app_task_signal_t i2c_step_publish_response(app_task_t *task) {
    transport_i2c_t *i2c = task->actor->object;
    app_event_t *response = &(app_event_t){.type = APP_EVENT_RESPONSE, .producer = i2c->actor, .consumer = task->inciting_event.producer};
    app_double_buffer_stop(&i2c->read, &response->data, &response->size);
    app_publish(task->actor->app, response);
    return APP_TASK_STEP_COMPLETE;
}

static app_task_signal_t i2c_task_read(app_task_t *task) {
    app_task_signal_t error_signal = i2c_handle_error(task);
    if (error_signal) {
        return error_signal;
    }
    transport_i2c_event_argument_t *argument = i2c_unpack_event_argument(&task->inciting_event.argument);
    switch (task->phase_index) {
    case 0:
        return i2c_step_setup(task, argument->slave_address, argument->memory_address);
    case 1:
        return i2c_step_read_data(task, argument->slave_address, task->inciting_event.data, task->inciting_event.size);
    case 2:
        return i2c_step_publish_response(task);
    case 3:
        return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t i2c_task_write(app_task_t *task) {
    app_task_signal_t error_signal = i2c_handle_error(task);
    if (error_signal) {
        return error_signal;
    }
    transport_i2c_event_argument_t *argument = i2c_unpack_event_argument(&task->inciting_event.argument);
    switch (task->phase_index) {
    case 0:
        return i2c_step_setup(task, argument->slave_address, argument->memory_address);
    case 1:
        return i2c_step_write_data(task, argument->slave_address, task->inciting_event.data, task->inciting_event.size);
    default:
        return APP_TASK_COMPLETE;
    }
}

static app_signal_t i2c_task_erase(app_task_t *task) {
}

/*
    I2C tasks execute their workload right from ISR bypassing the queue
*/
static void i2c_notify(size_t index) {
    transport_i2c_t *i2c = i2c_units[index - 1];
    if (i2c != NULL && i2c->task.handler) {
        app_task_execute(&i2c->task);
    }
    volatile uint32_t s1 = I2C_SR1(i2c->address);
    volatile uint32_t s2 = I2C_SR2(i2c->address);
}

static app_signal_t i2c_on_report(transport_i2c_t *i2c, app_event_t *event) {
}

static app_signal_t i2c_on_signal(transport_i2c_t *i2c, actor_t *actor, app_signal_t signal, void *source) {
    switch (signal) {
    // DMA interrupts on buffer filling up half-way or all the way up
    case APP_SIGNAL_DMA_TRANSFERRING:
        if (actor_dma_match_source(source, i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream)) {
            if (app_double_buffer_from_dma(&i2c->read, dma_get_number_of_data(dma_get_address(i2c->properties->dma_rx_unit),
                                                                     i2c->properties->dma_rx_stream)) == APP_SIGNAL_INCOMPLETE) {
                break;
            }
        }
        i2c->incoming_signal = signal;
        app_task_execute(&i2c->task);
        break;
    default:
        break;
    }
    return 0;
}

static app_signal_t i2c_tick_input(transport_i2c_t *i2c, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_READ:
        return actor_event_handle_and_start_task(i2c->actor, event, &i2c->task, i2c->actor->app->threads->input, i2c_task_read);
    case APP_EVENT_WRITE:
        return actor_event_handle_and_start_task(i2c->actor, event, &i2c->task, i2c->actor->app->threads->input, i2c_task_write);
    case APP_EVENT_THREAD_ALARM:
        return app_task_execute(&i2c->task);
    default:
        return 0;
    }
}
actor_class_t transport_i2c_class = {
    .type = TRANSPORT_I2C,
    .size = sizeof(transport_i2c_t),
    .phase_subindex = TRANSPORT_I2C_PHASE,
    .validate = (app_method_t)i2c_validate,
    .construct = (app_method_t)i2c_construct,
    .link = (app_method_t)i2c_link,
    .start = (app_method_t)i2c_start,
    .stop = (app_method_t)i2c_stop,
    .on_report = (actor_on_report_t)i2c_on_report,
    .on_phase = (actor_on_phase_t)i2c_phase,
    .on_signal = (actor_on_signal_t)i2c_on_signal,
    .property_write = i2c_property_write,
    .tick_input = (actor_on_tick_t)i2c_tick_input,
};

void i2c1_ev_isr(void) {
    i2c_notify(1);
}
void i2c1_er_isr(void) {
    i2c_notify(1);
}
void i2c2_ev_isr(void) {
    i2c_notify(2);
}
void i2c2_er_isr(void) {
    i2c_notify(2);
}
void i2c3_ev_isr(void) {
    i2c_notify(3);
}
void i2c3_er_isr(void) {
    i2c_notify(3);
}