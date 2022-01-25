#include "w25.h"
#include "transport/spi.h"


static app_task_signal_t step_spi_transfer(app_task_t *task, uint8_t *write_data, size_t write_size, size_t read_size) {
    app_publish(task->actor->app, &((app_event_t){
                                       .type = APP_EVENT_TRANSFER,
                                       .consumer = ((storage_w25_t *)task->actor->object)->spi->actor,
                                       .producer = task->actor,
                                       .data = write_data,
                                       .size = write_size,
                                       .argument = (void *) read_size
                                   }));
    return APP_TASK_STEP_WAIT;
}

// Query SR signal in a loop to check if actor is ready to accept commands
static app_task_signal_t step_wait_until_ready(app_task_t *task) {
    switch (task->step_index) {
    case 0: return step_spi_transfer(task, (uint8_t[]){W25_CMD_READ_SR1}, 1, 1);
    default: return (*task->awaited_event.data & W25_SR1_BUSY) ? APP_TASK_STEP_RETRY : APP_TASK_STEP_COMPLETE;
    }
}

// Send command and receieve response
static app_task_signal_t step_set_lock(app_task_t *task, bool_t state) {
    return step_spi_transfer(task, (uint8_t[]){state ? W25_CMD_UNLOCK : W25_CMD_LOCK}, 1, 0);
}

static app_task_signal_t step_write_in_pages(app_task_t *task, uint32_t address, uint8_t *data, size_t size, size_t page_size) {
    uint32_t bytes_on_page = get_number_of_bytes_intesecting_page(address + task->counter, page_size);
    switch (task->step_index) {
    case 0: return step_set_lock(task, false);
    case 2: return step_spi_transfer(task, data, bytes_on_page, bytes_on_page);
    default:
        task->counter += bytes_on_page;
        if (task->counter == size) {
            task->counter = 0;
            return APP_TASK_STEP_COMPLETE;
        } else {
            return APP_TASK_STEP_RETRY;
        }
    }
}

static app_task_signal_t w25_task_introspection(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_spi_transfer(task, (uint8_t[]){W25_CMD_MANUF_ACTOR, 0xff, 0xff, 0x00}, 4, 2);
    default: return APP_TASK_COMPLETE;
    }
}

static app_task_signal_t w25_send_command(app_task_t *task, uint8_t command) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_send(task, (uint8_t[]){command}, 1);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_lock(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_set_lock(task, true);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_unlock(app_task_t *task) {
    switch (task->phase_index) {
    case 0: return step_wait_until_ready(task);
    case 1: return step_set_lock(task, false);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_enable(app_task_t *task) {
    return w25_send_command(task, W25_CMD_PWR_ON);
}
static app_task_signal_t w25_task_disable(app_task_t *task) {
    return w25_send_command(task, W25_CMD_PWR_OFF);
}

static app_task_signal_t w25_task_write(app_task_t *task) {
    switch (task->phase_index) {
    case 1: return step_wait_until_ready(task);
    case 2:
        return step_write_in_pages(task, (uint32_t)task->inciting_event.argument, task->inciting_event.data, task->inciting_event.size,
                                   256);
    default: return APP_TASK_COMPLETE;
    }
}
static app_task_signal_t w25_task_read(app_task_t *task) {
    switch (task->phase_index) {
    default: return APP_TASK_COMPLETE;
    }
}

static app_signal_t w25_tick_high_priority(storage_w25_t *w25, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    (void)tick;
    (void)thread;
    if (event->type == APP_EVENT_THREAD_ALARM) {
        return app_task_execute(&w25->task);
    }
    return 0;
}

static ODR_t w25_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    storage_w25_t *w25 = stream->object;
    (void)w25;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t w25_validate(storage_w25_properties_t *properties) {
    return 0;
}

static app_signal_t w25_construct(storage_w25_t *w25) {
    return 0;
}

static app_signal_t w25_stop(storage_w25_t *w25) {
    return actor_event_handle_and_start_task(w25->actor, &((app_event_t){.type = APP_EVENT_DISABLE}), &w25->task,
                                              w25->actor->app->threads->bg_priority, w25_task_disable);
}

static app_signal_t w25_start(storage_w25_t *w25) {
    return actor_event_handle_and_start_task(w25->actor, &((app_event_t){.type = APP_EVENT_ENABLE}), &w25->task,
                                              w25->actor->app->threads->bg_priority, w25_task_enable);
}

/* Link w25 actor with its spi module (i2c, spi, uart) */
static app_signal_t w25_link(storage_w25_t *w25) {
    actor_link(w25->actor, (void **)&w25->spi, w25->properties->spi_index, NULL);
    return 0;
}

static app_signal_t w25_on_phase(storage_w25_t *w25, actor_phase_t phase) {
    (void)w25;
    (void)phase;
    return 0;
}

// Acknowledge returned event
static app_signal_t w25_on_event(storage_w25_t *w25, app_event_t *event) {
    switch (event->type) {
    case APP_EVENT_WRITE:
        app_thread_actor_schedule(w25->actor->app->threads->high_priority, w25->actor,
                                   w25->actor->app->threads->high_priority->current_time);
        break;
    default: break;
    }
    return 0;
}

static app_signal_t w25_tick_input(storage_w25_t *w25, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_INTROSPECTION:
        return actor_event_handle_and_start_task(w25->actor, event, &w25->task, w25->actor->app->threads->high_priority,
                                                  w25_task_introspection);
    case APP_EVENT_WRITE:
        return actor_event_handle_and_start_task(w25->actor, event, &w25->task, w25->actor->app->threads->high_priority, w25_task_write);
    case APP_EVENT_READ:
        return actor_event_handle_and_start_task(w25->actor, event, &w25->task, w25->actor->app->threads->high_priority, w25_task_read);
    case APP_EVENT_LOCK:
        return actor_event_handle_and_start_task(w25->actor, event, &w25->task, w25->actor->app->threads->high_priority, w25_task_lock);
    case APP_EVENT_UNLOCK:
        return actor_event_handle_and_start_task(w25->actor, event, &w25->task, w25->actor->app->threads->high_priority,
                                                  w25_task_unlock);
    case APP_EVENT_RESPONSE:
        if (event->producer == w25->spi && w25->task.handler != NULL) {
            return actor_event_handle_and_pass_to_task(w25->actor, event, &w25->task, w25->actor->app->threads->high_priority,
                                                        w25->task.handler);
        }
        break;
    default: return 0;
    }
}

actor_class_t storage_w25_class = {
    .type = STORAGE_W25,
    .size = sizeof(storage_w25_t),
    .phase_subindex = STORAGE_W25_PHASE,
    .validate = (app_method_t)w25_validate,
    .construct = (app_method_t)w25_construct,
    .link = (app_method_t)w25_link,
    .start = (app_method_t)w25_start,
    .stop = (app_method_t)w25_stop,
    .tick_input = (actor_on_tick_t)w25_tick_input,
    .tick_high_priority = (actor_on_tick_t)w25_tick_high_priority,
    .on_phase = (actor_on_phase_t)w25_on_phase,
    .on_event = (actor_on_event_t)w25_on_event,
    .property_write = w25_property_write,
};
