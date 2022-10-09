#include "i2c.h"
#include <actor/lib/dma.h>
#include <actor/lib/gpio.h>
#include <actor/lib/time.h>
#include <actor/module/mcu.h>
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


static actor_signal_t i2c_validate(transport_i2c_properties_t *properties) {
    (void)properties;
    return 0;
}

static void i2c_dma_tx_start(transport_i2c_t *i2c, uint8_t *data, uint32_t size) {
    actor_register_dma(i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream, i2c->actor);

    debug_printf("│ │ ├ I2C%u\t\t", i2c->actor->seq + 1);
    debug_printf("TX started\tDMA%u(%u/%u)\n", i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream,
                 i2c->properties->dma_tx_channel);

    actor_dma_tx_start((uint32_t) & (I2C_DR(i2c->address)), i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream,
                       i2c->properties->dma_tx_channel, i2c->source_buffer->data, i2c->source_buffer->size, false, 1, 0, 0);

    i2c_enable_dma(i2c->address);
}

static void i2c_dma_tx_stop(transport_i2c_t *i2c) {
    debug_printf("│ │ ├ I2C%u\t\t", i2c->actor->seq + 1);
    debug_printf("TX stopped\tDMA%u(%u/%u)\n", i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream,
                 i2c->properties->dma_tx_channel);

    actor_buffer_release(i2c->source_buffer, i2c->actor);
    actor_dma_tx_stop(i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream, i2c->properties->dma_tx_channel);
    actor_unregister_dma(i2c->properties->dma_tx_unit, i2c->properties->dma_tx_stream);
    i2c_disable_dma(i2c->address);
}

static void i2c_dma_rx_start(transport_i2c_t *i2c, uint8_t *destination, uint32_t expected_size) {
    actor_register_dma(i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream, i2c->actor);

    debug_printf("│ │ ├ I2C%u\t\t", i2c->actor->seq + 1);
    debug_printf("RX started\tDMA%u(%u/%u)\n", i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream,
                 i2c->properties->dma_rx_channel);

    i2c_enable_dma(i2c->address);

    actor_buffer_t *input = actor_double_buffer_get_input(i2c->ring_buffer, i2c->output_buffer);
    actor_dma_rx_start((uint32_t) & (I2C_DR(i2c->address)), i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream,
                       i2c->properties->dma_rx_channel, input->data, input->allocated_size, input == i2c->ring_buffer, 1, 0, 0);
}

static void i2c_dma_rx_stop(transport_i2c_t *i2c) {
    debug_printf("│ │ ├ I2C%u\t\t", i2c->actor->seq + 1);
    debug_printf("RX stopped\tDMA%u(%u/%u)\n", i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream,
                 i2c->properties->dma_rx_channel);

    actor_dma_rx_stop(i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream, i2c->properties->dma_rx_channel);
    actor_unregister_dma(i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream);
    i2c_disable_dma(i2c->address);
}

static actor_signal_t i2c_construct(transport_i2c_t *i2c) {
    // ring buffer struct is allocated right away, but not its memory
    // output buffer is allocated on demand
    i2c->ring_buffer = actor_buffer_wrapper(i2c->actor);

    if (!i2c->ring_buffer) {
        return ACTOR_SIGNAL_OUT_OF_MEMORY;
    }

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
        return ACTOR_SIGNAL_INVALID_ARGUMENT;
#endif
#ifdef I2C3_BASE
    case 2:
        i2c->clock = RCC_I2C3;
        i2c->address = I2C3;
        i2c->ev_irq = NVIC_I2C3_EV_IRQ;
        i2c->er_irq = NVIC_I2C3_ER_IRQ;
        break;
#else
        return ACTOR_SIGNAL_INVALID_ARGUMENT;
#endif
    }
    return 0;
}

transport_i2c_t *i2c_units[I2C_UNITS] = {};

void *i2c_pack_message_argument(uint8_t slave_address, uint16_t memory_address) {
    transport_i2c_message_argument_t address =
        (transport_i2c_message_argument_t){.slave_address = slave_address, .memory_address = memory_address};
    return (void *)*((uint32_t *)&address);
}

transport_i2c_message_argument_t *i2c_unpack_message_argument(void **argument) {
    return (transport_i2c_message_argument_t *)argument;
}

static actor_signal_t i2c_task_setup(actor_job_t *job, actor_signal_t signal, actor_t *caller, uint8_t address,
                                         uint32_t memory_register) {
    transport_i2c_t *i2c = job->actor->object;
    switch (job->task_phase) {
    case 0:
        i2c_peripheral_enable(i2c->address);
        i2c_send_start(i2c->address);
        return ACTOR_SIGNAL_OK;
    case 1:
        if (!I2C_EVENT_MASTER_MODE_SELECT(i2c->address)) {
            return ACTOR_SIGNAL_LOOP;
        } else {
            i2c_send_7bit_address(i2c->address, address, I2C_WRITE);
            return ACTOR_SIGNAL_WAIT;
        }
    case 2:
        if (!I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED(i2c->address) && !I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED(i2c->address)) {
            return ACTOR_SIGNAL_LOOP;
        } else {
            I2C_DR(i2c->address) = memory_register;
            return ACTOR_SIGNAL_OK;
        }
    case 3:
        if (!I2C_EVENT_TXE(i2c->address)) {
            return ACTOR_SIGNAL_LOOP;
        } else {
            return ACTOR_SIGNAL_TASK_COMPLETE;
        }
    }
    return 0;
}

static actor_signal_t i2c_task_write_data(actor_job_t *job, actor_signal_t signal, actor_t *caller, uint8_t address, uint8_t *data,
                                              uint32_t size) {
    (void)address;
    transport_i2c_t *i2c = job->actor->object;
    if (signal > ACTOR_SIGNAL_FAILURE) {
        switch (job->task_phase) {
        case 0:
            nvic_disable_irq(i2c->ev_irq);
            return actor_job_switch_thread(job, job->thread);
        case 1:
            i2c->source_buffer = actor_buffer_source(i2c->actor, data, size);
            if (!i2c->source_buffer) {
                signal = ACTOR_SIGNAL_OUT_OF_MEMORY;
                break;
            } else {
                return ACTOR_SIGNAL_OK;
            }
        case 2:
            i2c_dma_tx_start(i2c, data, size);
            return ACTOR_SIGNAL_OK;
        case 3:
            if (signal != ACTOR_SIGNAL_DMA_TRANSFERRING) {
                return ACTOR_SIGNAL_LOOP;
            } else {
                i2c_dma_tx_stop(i2c);
                nvic_enable_irq(i2c->ev_irq);
                return ACTOR_SIGNAL_OK;
            }
        case 4:
            if (!I2C_EVENT_BTF(i2c->address) && !I2C_EVENT_TXE(i2c->address)) {
                return ACTOR_SIGNAL_LOOP;
            } else {
                i2c_send_stop(i2c->address);
                i2c_peripheral_disable(i2c->address);
                return ACTOR_SIGNAL_TASK_COMPLETE;
            }
        }
    }

    i2c_peripheral_disable(i2c->address);
    actor_buffer_release(i2c->source_buffer, i2c->actor);
    i2c->source_buffer = NULL;
    return signal;
}

static actor_signal_t i2c_task_read_data(actor_job_t *job, actor_signal_t signal, actor_t *caller, uint8_t address, uint8_t *data,
                                             uint32_t size) {
    transport_i2c_t *i2c = job->actor->object;
    if (signal > ACTOR_SIGNAL_FAILURE) {
        switch (job->task_phase) {
        case 0:
            nvic_disable_irq(i2c->ev_irq);
            return actor_job_switch_thread(job, job->thread);
        case 1:
            i2c->output_buffer = actor_double_buffer_target(i2c->ring_buffer, i2c->actor, data, size);
            if (!i2c->output_buffer) {
                signal = ACTOR_SIGNAL_OUT_OF_MEMORY;
                break;
            } else {
                return ACTOR_SIGNAL_OK;
            }
        case 2:
            i2c_enable_ack(i2c->address);
            i2c_send_start(i2c->address);
            nvic_enable_irq(i2c->ev_irq);
            return ACTOR_SIGNAL_OK;
        case 3:
            if (!I2C_EVENT_MASTER_MODE_SELECT(i2c->address)) {
                return ACTOR_SIGNAL_LOOP;
            } else {
                i2c_send_7bit_address(i2c->address, address, I2C_READ);
                return ACTOR_SIGNAL_OK;
            }
        case 4:
            if (!I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED(i2c->address)) {
                return ACTOR_SIGNAL_LOOP;
            } else {
                i2c_dma_rx_start(i2c, data, size);
                return ACTOR_SIGNAL_WAIT;
            }
        case 5:
            if (signal != ACTOR_SIGNAL_DMA_TRANSFERRING || i2c->output_buffer->size != size) {
                return ACTOR_SIGNAL_LOOP;
            } else {
                i2c_dma_rx_stop(i2c);
                i2c_peripheral_disable(i2c->address);
                i2c_send_stop(i2c->address);
                return ACTOR_SIGNAL_TASK_COMPLETE;
            }
        }
    }
    i2c_peripheral_disable(i2c->address);
    actor_buffer_release(i2c->output_buffer, i2c->actor);
    i2c->output_buffer = NULL;
    return signal;
}

static actor_signal_t i2c_task_publish_response(actor_job_t *job, actor_signal_t signal, actor_t *caller) {
    actor_buffer_t *buffer;
    transport_i2c_t *i2c = job->actor->object;
    switch (job->task_phase) {
    case 0:
        buffer = actor_double_buffer_detach(i2c->ring_buffer, &i2c->output_buffer, i2c->actor);

        // if event was ACTOR_MESSAGE_READ_TO_BUFFER, it is assumed that producer expect report instead of event
        if (job->inciting_message.type == ACTOR_MESSAGE_READ) {
            actor_publish(job->actor->node->actor, &((actor_message_t){.type = ACTOR_MESSAGE_RESPONSE,
                                                                     .producer = i2c->actor,
                                                                     .consumer = job->inciting_message.producer,
                                                                     .data = (uint8_t *)buffer,
                                                                     .size = ACTOR_BUFFER_DYNAMIC_SIZE}));
        } else {
            actor_buffer_release(buffer, job->actor);
        }
        return ACTOR_SIGNAL_TASK_COMPLETE;
    }
    return 0;
}


static actor_signal_t i2c_start(transport_i2c_t *i2c) {
    i2c_peripheral_disable(i2c->address); /* disable i2c during setup */

    // make i2c unstuck (follows official errata solution)
    gpio_configure_output_opendrain_pullup(i2c->properties->scl_port, i2c->properties->scl_pin, GPIO_FAST);
    gpio_configure_output_opendrain_pullup(i2c->properties->sda_port, i2c->properties->sda_pin, GPIO_FAST);
    uint16_t pins = (1 << i2c->properties->scl_pin) | (1 << i2c->properties->sda_pin);
    gpio_set(GPIOX(i2c->properties->scl_port), pins);
    while ((GPIO_ODR(GPIOX(i2c->properties->scl_port)) & pins) != pins)
        ;
    gpio_configure_output_opendrain_pulldown(i2c->properties->scl_port, i2c->properties->scl_pin, GPIO_FAST);
    gpio_configure_output_opendrain_pulldown(i2c->properties->sda_port, i2c->properties->sda_pin, GPIO_FAST);
    gpio_clear(GPIOX(i2c->properties->scl_port), pins);
    while ((GPIO_ODR(GPIOX(i2c->properties->scl_port)) & pins) != 0)
        ;
    gpio_configure_output_opendrain_pullup(i2c->properties->scl_port, i2c->properties->scl_pin, GPIO_FAST);
    gpio_configure_output_opendrain_pullup(i2c->properties->sda_port, i2c->properties->sda_pin, GPIO_FAST);
    gpio_set(GPIOX(i2c->properties->scl_port), pins);
    while ((GPIO_ODR(GPIOX(i2c->properties->scl_port)) & pins) != pins)
        ;

    debug_printf("    > I2C%i SCL", i2c->actor->seq + 1);
    gpio_configure_af_opendrain(i2c->properties->scl_port, i2c->properties->scl_pin, GPIO_FAST, i2c->properties->af);
    debug_printf("    > I2C%i SMBA", i2c->actor->seq + 1);
    gpio_configure_af_opendrain(i2c->properties->smba_port, i2c->properties->smba_pin, GPIO_FAST, i2c->properties->af);
    debug_printf("    > I2C%i SDA", i2c->actor->seq + 1);
    gpio_configure_af_opendrain(i2c->properties->sda_port, i2c->properties->sda_pin, GPIO_FAST, i2c->properties->af);

    I2C_CR1(i2c->address) |= I2C_CR1_SWRST;
    I2C_CR1(i2c->address) ^= I2C_CR1_SWRST;

    rcc_periph_clock_enable(i2c->clock);
    i2c_reset(i2c->address);
    // i2c_enable_analog_filter(i2c->address);
    // i2c_set_digital_filter(i2c->address, 0);

    i2c_set_speed(I2C1, i2c_speed_sm_100k, 8);

    i2c_set_fast_mode(i2c->address);
    i2c_set_clock_frequency(i2c->address, i2c->properties->frequency);

    // i2c_set_clock_frequency(i2c->address, 42);
    // i2c_set_ccr(i2c->address, 35);
    // i2c_set_trise(i2c->address, 43);
    // i2c_set_speed(i2c->address, i2c_speed_sm_100k, i2c->actor->node->mcu->clock->apb1_frequency / 1000000);

    // i2c_set_ccr(i2c->address, 35);
    // i2c_set_trise(i2c->address, 43);

    i2c_set_own_7bit_slave_address(i2c->address, i2c->properties->slave_address);

    i2c_units[i2c->actor->seq] = i2c;

    i2c_disable_ack(i2c->address);
    i2c_enable_interrupt(i2c->address, I2C_CR2_ITEVTEN | I2C_CR2_ITERREN);
    nvic_enable_irq(i2c->ev_irq);
    nvic_set_priority(i2c->ev_irq, 10 << 4);
    nvic_enable_irq(i2c->er_irq);
    nvic_set_priority(i2c->er_irq, 11 << 4);
    i2c_set_dma_last_transfer(i2c->address);

    return 0;
}

static actor_signal_t i2c_stop(transport_i2c_t *i2c) {
    i2c_send_stop(i2c->address);
    i2c_reset(i2c->address);
    i2c_peripheral_disable(i2c->address);
    return 0;
}

static actor_signal_t i2c_job_read(actor_job_t *job, actor_signal_t signal, actor_t *caller) {
    transport_i2c_message_argument_t *argument = i2c_unpack_message_argument(&job->inciting_message.argument);
    switch (job->job_phase) {
    case 0:
        return i2c_task_setup(job, signal, caller, argument->slave_address, argument->memory_address);
    case 1:
        return i2c_task_read_data(job, signal, caller, argument->slave_address, job->inciting_message.data, job->inciting_message.size);
    case 2:
        return i2c_task_publish_response(job, signal, caller);
    }
    return ACTOR_SIGNAL_JOB_COMPLETE;
}

static actor_signal_t i2c_job_write(actor_job_t *job, actor_signal_t signal, actor_t *caller) {
    transport_i2c_message_argument_t *argument = i2c_unpack_message_argument(&job->inciting_message.argument);
    switch (job->job_phase) {
    case 0:
        return i2c_task_setup(job, signal, caller, argument->slave_address, argument->memory_address);
    case 1:
        return i2c_task_write_data(job, signal, caller, argument->slave_address, job->inciting_message.data, job->inciting_message.size);
    }
    return ACTOR_SIGNAL_JOB_COMPLETE;
}

/*
    I2C tasks execute their workload right from ISR bypassing the queue
*/
static void i2c_notify(size_t index) {
    debug_printf("> I2C%i interrupt\n", index);
    transport_i2c_t *i2c = i2c_units[index - 1];
    if (i2c != NULL && i2c->job != NULL) {
        actor_job_execute(i2c->job, I2C_EVENT_ERROR(i2c->address) ? ACTOR_SIGNAL_FAILURE : ACTOR_SIGNAL_OK, i2c->job->actor);
    }
    volatile uint32_t s1 = I2C_SR1(i2c->address);
    volatile uint32_t s2 = I2C_SR2(i2c->address);
    debug_printf("< I2C%i interrupt\n", index);
    (void)s1;
    (void)s2;
}

static actor_signal_t i2c_message_handled(transport_i2c_t *i2c, actor_message_t *event, actor_signal_t signal) {
    return 0;
}

static actor_signal_t i2c_signal_received(transport_i2c_t *i2c, actor_signal_t signal, actor_t *caller, void *source) {
    switch (signal) {
    // DMA interrupts on buffer filling up half-way or all the way up
    case ACTOR_SIGNAL_DMA_TRANSFERRING:
        if (actor_dma_match_source(source, i2c->properties->dma_rx_unit, i2c->properties->dma_rx_stream)) {
            uint16_t buffer_size = actor_double_buffer_get_input_size(i2c->ring_buffer, i2c->output_buffer);
            uint16_t bytes_left = dma_get_number_of_data(dma_get_address(i2c->properties->dma_rx_unit), i2c->properties->dma_rx_stream);
            actor_double_buffer_ingest_external_write(i2c->ring_buffer, i2c->output_buffer, buffer_size - bytes_left);
        }
        actor_job_execute(i2c->job, signal, caller);
        break;
    default:
        break;
    }
    return 0;
}

static actor_signal_t i2c_worker_on_input(transport_i2c_t *i2c, actor_message_t *event, actor_worker_t *tick, actor_thread_t *thread) {
    switch (event->type) {
    case ACTOR_MESSAGE_READ:
    case ACTOR_MESSAGE_READ_TO_BUFFER:
        return actor_message_handle_and_start_job(i2c->actor, event, &i2c->job, i2c->actor->node->medium_priority, i2c_job_read);
    case ACTOR_MESSAGE_WRITE:
        return actor_message_handle_and_start_job(i2c->actor, event, &i2c->job, i2c->actor->node->medium_priority, i2c_job_write);
    default:
        return 0;
    }
}

static actor_signal_t i2c_worker_medium_priority(transport_i2c_t *i2c, actor_message_t *event, actor_worker_t *tick, actor_thread_t *thread) {
    return actor_job_execute_if_running_in_thread(i2c->job, thread);
}

static actor_signal_t i2c_buffer_allocation(transport_i2c_t *i2c, actor_buffer_t *buffer, uint32_t size) {
    if (buffer == i2c->ring_buffer) {
        return actor_buffer_reserve_with_limits(buffer, size, i2c->properties->dma_rx_circular_buffer_size, 0, 0);
    } else if (buffer == i2c->output_buffer) {
        return actor_buffer_reserve_with_limits(buffer, size, i2c->properties->rx_pool_initial_size, i2c->properties->rx_pool_block_size,
                                                i2c->properties->rx_pool_max_size);
    } else {
        return actor_buffer_reserve_with_limits(buffer, size, 0, 0, 0);
    }
}

static actor_worker_callback_t i2c_worker_assignment(transport_i2c_t *i2c, actor_thread_t *thread) {
    if (thread == i2c->actor->node->input) {
        return (actor_worker_callback_t)i2c_worker_on_input;
    } else if (thread == i2c->actor->node->medium_priority) {
        return (actor_worker_callback_t)i2c_worker_medium_priority;
    }
    return NULL;
}

static actor_signal_t i2c_phase_changed(transport_i2c_t *i2c, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_START:
        return i2c_start(i2c);
    case ACTOR_PHASE_STOP:
        return i2c_stop(i2c);
    case ACTOR_PHASE_CONSTRUCTION:
        return i2c_construct(i2c);
    default:
        return 0;
    }
}
actor_interface_t transport_i2c_class = {
    .type = TRANSPORT_I2C,
    .size = sizeof(transport_i2c_t),
    .phase_subindex = TRANSPORT_I2C_PHASE,
    .validate = (actor_method_t)i2c_validate,
    .message_handled = (actor_message_handled_t)i2c_message_handled,
    .phase_changed = (actor_phase_changed_t)  i2c_phase_changed,
    .signal_received = (actor_signal_received_t)i2c_signal_received,
    .buffer_allocation = (actor_buffer_allocation_t)i2c_buffer_allocation,
    .worker_assignment = (actor_worker_assignment_t)i2c_worker_assignment,
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