#include "ili9341.h"
#include <libopencm3/stm32/gpio.h>

static void ili9341_send_command(screen_ili9341_t *ili9341, uint8_t command) {
    *ili9341->cmd_register = command;
} 
static void ili9341_send_data(screen_ili9341_t *ili9341, uint8_t data) {
    *ili9341->data_register = data;
} 

static ODR_t ili9341_property_after_change(screen_ili9341_t *ili9341, uint8_t index, void *value, void *old) {
    switch (index) {
    case SCREEN_ILI9341_ORIENTATION:

        break;
    case SCREEN_ILI9341_BACKLIGHT:

        break;
    case SCREEN_ILI9341_INVERT_COLORS:
        ili9341_send_command(ili9341, ili9341->properties->invert_colors ? ILI9341_CMD_INVERT_COLORS_ON : ILI9341_CMD_INVERT_COLORS_OFF);
        break;
    default:
        break;
    }
    return ACTOR_SIGNAL_OK;
}

static actor_signal_t ili9341_validate(screen_ili9341_properties_t *properties) {
    return 0;
}

static struct {
    uint32_t gpio;
    uint16_t pins;
} sdram_pins[4] = {
    {GPIOD, GPIO0 | GPIO1 | GPIO4 | GPIO5 | GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15},
    {GPIOE, GPIO0 | GPIO1 | GPIO7 | GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15},
    {GPIOF, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO12 | GPIO13 | GPIO14 | GPIO15},
    {GPIOG, GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO10},
};

static actor_signal_t ili9341_construct(screen_ili9341_t *ili9341) {
    ili9341->cmd_register = (uint16_t *)(0x6C000000 | 0x00001FFE);
    ili9341->data_register = (uint16_t *)(0x6C000000 | 0x00002000);
    /*FSMC_Bank1E->BWTR[6] &= ~(0XF << 0); // The address setup time (ADDSET) is cleared
    FSMC_Bank1E->BWTR[6] &= ~(0XF << 8); // Clear data save time
    FSMC_Bank1E->BWTR[6] |= 3 << 0;      // Address setup time (ADDSET) is 3 HCLK = 18ns
    FSMC_Bank1E->BWTR[6] |= 2 << 8;      // Data storage time is 6ns*3 HCLK=18ns
*/
    return 0;
}

static actor_signal_t ili9341_start(screen_ili9341_t *ili9341) {
    return 0;
}

static actor_signal_t ili9341_stop(screen_ili9341_t *ili9341) {
    return 0;
}

static actor_signal_t ili9341_link(screen_ili9341_t *ili9341) {
    return 0;
}

static actor_signal_t ili9341_signal_received(screen_ili9341_t *ili9341, actor_signal_t signal, actor_t *caller, void *source) {
    return 0;
}

static actor_signal_t ili9341_task_configure(actor_job_t *job) {
    screen_ili9341_t *ili9341 = job->actor->object;
    switch (job->task_phase) {
    case 0:
        ili9341_send_command(ili9341, ILI9341_CMD_POWER_CONTROL_B);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0xC1);
        ili9341_send_data(ili9341, 0X30);
        ili9341_send_command(ili9341, ILI9341_CMD_POWER_SEQUENCE_CONTROL);
        ili9341_send_data(ili9341, 0x64);
        ili9341_send_data(ili9341, 0x03);
        ili9341_send_data(ili9341, 0X12);
        ili9341_send_data(ili9341, 0X81);
        ili9341_send_command(ili9341, ILI9341_CMD_DRIVER_TIMING_CONTROL_A);
        ili9341_send_data(ili9341, 0x85);
        ili9341_send_data(ili9341, 0x10);
        ili9341_send_data(ili9341, 0x7A);
        ili9341_send_command(ili9341, ILI9341_CMD_POWER_CONTROL_A); 
        ili9341_send_data(ili9341, 0x39);
        ili9341_send_data(ili9341, 0x2C);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x34);
        ili9341_send_data(ili9341, 0x02);
        ili9341_send_command(ili9341, ILI9341_CMD_PUMP_RATIO_CONTROL);
        ili9341_send_data(ili9341, 0x20);
        ili9341_send_command(ili9341, ILI9341_CMD_DRIVER_TIMING_CONTROL_B);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_command(ili9341, ILI9341_CMD_POWER_CONTROL_1);
        ili9341_send_data(ili9341, 0x1B);
        ili9341_send_command(ili9341, ILI9341_CMD_POWER_CONTROL_2);
        ili9341_send_data(ili9341, 0x01);
        ili9341_send_command(ili9341, ILI9341_CMD_VCOM_CONTROL_1);
        ili9341_send_data(ili9341, 0x30);
        ili9341_send_data(ili9341, 0x30);
        ili9341_send_command(ili9341, ILI9341_CMD_VCOM_CONTROL_2);
        ili9341_send_data(ili9341, 0XB7);
        ili9341_send_command(ili9341, ILI9341_CMD_MEMORY_CONTROL);
        ili9341_send_data(ili9341, 0x48);
        ili9341_send_command(ili9341, ILI9341_CMD_PIXEL_FORMAT_SET);
        ili9341_send_data(ili9341, 0x55);
        ili9341_send_command(ili9341, ILI9341_CMD_FRAME_RATE_CONTROL_NORMAL);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x1A);
        ili9341_send_command(ili9341, ILI9341_CMD_DISPLAY_FUNCTION_CONTROL);
        ili9341_send_data(ili9341, 0x0A);
        ili9341_send_data(ili9341, 0xA2);
        ili9341_send_command(ili9341, ILI9341_CMD_ENABLE_3g);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_command(ili9341, ILI9341_CMD_GAMMA_SET);
        ili9341_send_data(ili9341, 0x01);
        ili9341_send_command(ili9341, ILI9341_CMD_POSITIVE_GAMMA_CORRECTION);
        ili9341_send_data(ili9341, 0x0F);
        ili9341_send_data(ili9341, 0x2A);
        ili9341_send_data(ili9341, 0x28);
        ili9341_send_data(ili9341, 0x08);
        ili9341_send_data(ili9341, 0x0E);
        ili9341_send_data(ili9341, 0x08);
        ili9341_send_data(ili9341, 0x54);
        ili9341_send_data(ili9341, 0XA9);
        ili9341_send_data(ili9341, 0x43);
        ili9341_send_data(ili9341, 0x0A);
        ili9341_send_data(ili9341, 0x0F);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_command(ili9341, ILI9341_CMD_NEGATIVE_GAMMA_CORRECTION);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x15);
        ili9341_send_data(ili9341, 0x17);
        ili9341_send_data(ili9341, 0x07);
        ili9341_send_data(ili9341, 0x11);
        ili9341_send_data(ili9341, 0x06);
        ili9341_send_data(ili9341, 0x2B);
        ili9341_send_data(ili9341, 0x56);
        ili9341_send_data(ili9341, 0x3C);
        ili9341_send_data(ili9341, 0x05);
        ili9341_send_data(ili9341, 0x10);
        ili9341_send_data(ili9341, 0x0F);
        ili9341_send_data(ili9341, 0x3F);
        ili9341_send_data(ili9341, 0x3F);
        ili9341_send_data(ili9341, 0x0F);
        ili9341_send_command(ili9341, ILI9341_CMD_PAGE_ADDRESS_SET);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x01);
        ili9341_send_data(ili9341, 0x3f);
        ili9341_send_command(ili9341, ILI9341_CMD_COLUMN_ADDRESS_SET);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0x00);
        ili9341_send_data(ili9341, 0xef);
        return ACTOR_SIGNAL_OK;
    default:
        return ACTOR_SIGNAL_TASK_COMPLETE;
    }
}

static actor_signal_t ili9341_task_sleep(actor_job_t *job, bool arg) {
    screen_ili9341_t *ili9341 = job->actor->object;
    switch (job->task_phase) {
    case 0:
        // wait until it is allowed to send sleep commands
        return actor_mutex_acquire(&ili9341->sleep_mutex, job);
    case 1:
        // sleep commands can only be issued every 120ms, so set mutex to auto-expire in the background
        actor_mutex_expiration_delay(job, &ili9341->sleep_mutex_expiration_time, 120);
        ili9341_send_command(ili9341, arg ? ILI9341_CMD_SLEEP_IN : ILI9341_CMD_SLEEP_OUT); 
        // screen can accept commands not related to sleep in only 5ms
        return actor_job_delay(job, 5);
    default:
        return ACTOR_SIGNAL_TASK_COMPLETE;
    }
}


static actor_signal_t ili9341_task_apply(actor_job_t *job) {
    screen_ili9341_t *ili9341 = job->actor->object;
    switch (job->task_phase) {
    case 0:
        ili9341_send_command(ili9341, ILI9341_CMD_DISPLAY_ON);
        screen_ili9341_set_orientation(ili9341->actor, screen_ili9341_get_orientation(ili9341->actor));
        screen_ili9341_set_backlight(ili9341->actor, screen_ili9341_get_backlight(ili9341->actor));
        screen_ili9341_set_invert_colors(ili9341->actor, screen_ili9341_get_invert_colors(ili9341->actor));
        return ACTOR_SIGNAL_OK;
    default:
        return ACTOR_SIGNAL_TASK_COMPLETE;
    }
}

static actor_signal_t ili9341_job_initialize(actor_job_t *job, actor_signal_t signal, actor_t *caller) {
    screen_ili9341_t *ili9341 = job->actor->object;
    switch (job->job_phase) {
    case 0:
        return ili9341_task_configure(job);
    case 1:
        return ili9341_task_sleep(job, false);
    case 2:
        return ili9341_task_apply(job);
    case 3:
        return ACTOR_SIGNAL_JOB_COMPLETE;
    }
    return ACTOR_SIGNAL_FAILURE;
}


static actor_signal_t ili9341_on_input(screen_ili9341_t *ili9341, actor_message_t *event, actor_worker_t *worker, actor_thread_t *thread) {
    switch (event->type) {
        case ACTOR_MESSAGE_THREAD_START:
            actor_message_receive_and_start_job(ili9341->actor, event, &ili9341->job, ili9341->actor->node->high_priority, ili9341_job_initialize);
            break;
        default:
            break;
    }
    return 0;
}

static actor_signal_t ili9341_high_priority(screen_ili9341_t *ili9341, actor_message_t *event, actor_worker_t *worker, actor_thread_t *thread) {
    actor_mutex_attempt_expiration(&ili9341->sleep_mutex, ili9341->job, &ili9341->sleep_mutex_expiration_time);
    actor_job_execute_if_running_in_thread(ili9341->job, thread);
    return 0;
}

static actor_worker_callback_t ili9341_worker_assignment(screen_ili9341_t *ili9341, actor_thread_t *thread) {
    if (thread == ili9341->actor->node->input) {
        return (actor_worker_callback_t)ili9341_on_input;
    } else if (thread == ili9341->actor->node->high_priority) {
        return (actor_worker_callback_t)ili9341_high_priority;
    }
    return NULL;
}

static actor_signal_t ili9341_phase_changed(screen_ili9341_t *ili9341, actor_phase_t phase) {
    switch (phase) {
    case ACTOR_PHASE_CONSTRUCTION:
        return ili9341_construct(ili9341);
    case ACTOR_PHASE_LINKAGE:
        return ili9341_link(ili9341);
    case ACTOR_PHASE_START:
        return ili9341_start(ili9341);
    case ACTOR_PHASE_STOP:
        return ili9341_stop(ili9341);
    default:
        return 0;
    }
}

actor_interface_t screen_ili9341_class = {
    .type = SCREEN_ILI9341,
    .size = sizeof(screen_ili9341_t),
    .phase_subindex = SCREEN_ILI9341_PHASE,
    .validate = (actor_method_t)ili9341_validate,
    .phase_changed = (actor_phase_changed_t)  ili9341_phase_changed,
    .signal_received = (actor_signal_received_t)ili9341_signal_received,
    .worker_assignment = (actor_worker_assignment_t) ili9341_worker_assignment,
    .property_after_change = (actor_property_changed_t) ili9341_property_after_change,
};
