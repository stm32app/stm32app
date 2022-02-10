#include "storage/at24c.h"
#include "lib/bytes.h"
#include "transport/i2c.h"

static ODR_t at24c_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    storage_at24c_t *at24c = stream->object;
    (void)at24c;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static app_signal_t at24c_validate(storage_at24c_properties_t *properties) {
    return 0;
}

static app_signal_t at24c_construct(storage_at24c_t *at24c) {
    actor_event_subscribe(at24c->actor, APP_EVENT_DIAGNOSE);
    return 0;
}

static app_signal_t at24c_start(storage_at24c_t *at24c) {
    (void)at24c;
    return 0;
}

static app_signal_t at24c_stop(storage_at24c_t *at24c) {
    (void)at24c;
    return 0;
}

static app_signal_t at24c_link(storage_at24c_t *at24c) {
    actor_link(at24c->actor, (void **)&at24c->i2c, at24c->properties->i2c_index, NULL);
    return 0;
}

static app_signal_t at24c_phase(storage_at24c_t *at24c, actor_phase_t phase) {
    return 0;
}

static app_signal_t at24c_on_signal(storage_at24c_t *at24c, actor_t *actor, app_signal_t signal, void *source) {
    return 0;
}

static app_task_signal_t at24c_step_read(app_task_t *task, uint8_t address, uint8_t *data, uint32_t size) {
    storage_at24c_t *at24c = task->actor->object;
    uint32_t bytes_on_page;

    switch (task->step_index) {
    case 0:
        task->counter = 0;
        at24c->target_buffer = app_buffer_target(task->actor, data, size);
        if (at24c->target_buffer == NULL) {
            return APP_TASK_FAILURE;
        } else {
            return APP_TASK_STEP_CONTINUE;
        }
    case 1:
        bytes_on_page = get_number_of_bytes_intesecting_page(address + task->counter, size, at24c->properties->page_size);
        app_publish(task->actor->app, &((app_event_t){
                                          .type = APP_EVENT_READ_TO_BUFFER,
                                          .consumer = at24c->i2c->actor,
                                          .producer = task->actor,
                                          .data = &at24c->target_buffer->data[task->counter],
                                          .size = bytes_on_page,
                                          .argument = i2c_pack_event_argument(at24c->properties->i2c_address, address + task->counter),
                                      }));
        task->counter += bytes_on_page;
        if (task->counter == size) {
            return APP_TASK_STEP_WAIT;
        } else {
            return APP_TASK_STEP_LOOP;
        }
    case 2:
        return APP_TASK_STEP_SUCCESS;
    }

    return 0;
}

static app_signal_t at24c_step_write(app_task_t *task, uint8_t address, uint8_t *data, uint32_t size) {
    storage_at24c_t *at24c = task->actor->object;
    uint32_t bytes_on_page;

    switch (task->step_index) {
    case 0:
        task->counter = 0;
        at24c->source_buffer = app_buffer_source_copy(task->actor, data, size);
        if (at24c->source_buffer == NULL) {
            return APP_TASK_FAILURE;
        } else {
            return APP_TASK_STEP_CONTINUE;
        }
    case 1:
        bytes_on_page = get_number_of_bytes_intesecting_page(address + task->counter, size, at24c->properties->page_size);
        app_publish(task->actor->app, &((app_event_t){
                                          .type = APP_EVENT_WRITE,
                                          .consumer = at24c->i2c->actor,
                                          .producer = task->actor,
                                          .data = &at24c->source_buffer->data[task->counter],
                                          .size = bytes_on_page,
                                          .argument = i2c_pack_event_argument(at24c->properties->i2c_address, address + task->counter),
                                      }));
        task->counter += bytes_on_page;
        if (task->counter == size) {
            return APP_TASK_STEP_WAIT;
        } else {
            return APP_TASK_STEP_LOOP;
        }
    case 2:
        return APP_TASK_STEP_SUCCESS;
    }
    return 0;
}
static app_task_signal_t at24c_task_diagnose(app_task_t *task) {
    storage_at24c_t *at24c = task->actor->object;
    app_buffer_t *first_buffer;
    bool_t ok;

    switch (task->phase_index) {
    case 0:
        return at24c_step_read(task, 0x0010, NULL, 2);
    case 1:
        return at24c_step_write(task, 0x0010,
                                &((uint8_t[]){
                                    at24c->target_buffer->data[0] + 1,
                                    at24c->target_buffer->data[1],
                                })[0],
                                2);
    case 2:
        if (task->step_index == 0) {
            task->result = at24c->target_buffer;
            at24c->target_buffer = NULL;
        }
        return at24c_step_read(task, 0x0010, NULL, 2);

    case 3:
        first_buffer = (app_buffer_t *)task->result;
        ok = (first_buffer->data[0] + 1 == at24c->target_buffer->data[0] && first_buffer->data[1] == at24c->target_buffer->data[1] &&
              first_buffer->size == at24c->target_buffer->size);

        if (ok) {
            log_printf("  - Diagnostics OK: 0x%x %s\n", actor_index(task->actor), get_actor_type_name(task->actor->class->type));
            return APP_TASK_SUCCESS;
        } else {
            log_printf("  - Diagnostics ERROR: 0x%x %s\n", actor_index(task->actor), get_actor_type_name(task->actor->class->type));
            return APP_TASK_FAILURE;
        }
        break;

    case APP_TASK_SUCCESS:
    case APP_TASK_FAILURE:
        app_buffer_release(at24c->target_buffer, task->actor);
        app_buffer_release(at24c->source_buffer, task->actor);
        app_buffer_release((app_buffer_t *)task->result, task->actor);
    }

    return APP_TASK_SUCCESS;
}

static app_task_signal_t at24c_task_read(app_task_t *task) {
    return APP_TASK_SUCCESS;
}
static app_task_signal_t at24c_task_write(app_task_t *task) {
    return APP_TASK_SUCCESS;
}
static app_task_signal_t at24c_task_erase(app_task_t *task) {
    return APP_TASK_SUCCESS;
}

static app_signal_t at24c_tick_input(storage_at24c_t *at24c, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_DIAGNOSE:
        return actor_event_receive_and_start_task(at24c->actor, event, &at24c->task, at24c->actor->app->threads->low_priority,
                                                  at24c_task_diagnose);
    case APP_EVENT_RESPONSE:
        return actor_event_handle_and_pass_to_task(at24c->actor, event, &at24c->task, at24c->task.thread, at24c->task.handler);

    case APP_EVENT_READ:
        return actor_event_receive_and_start_task(at24c->actor, event, &at24c->task, at24c->actor->app->threads->low_priority,
                                                  at24c_task_read);
    case APP_EVENT_WRITE:
        return actor_event_receive_and_start_task(at24c->actor, event, &at24c->task, at24c->actor->app->threads->low_priority,
                                                  at24c_task_write);
    case APP_EVENT_ERASE:
        return actor_event_receive_and_start_task(at24c->actor, event, &at24c->task, at24c->actor->app->threads->low_priority,
                                                  at24c_task_erase);
    default:
        return 0;
    }
}

static app_signal_t at24c_tick_high_priority(storage_at24c_t *at24c, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    return app_task_execute_if_running_in_thread(&at24c->task, thread);
}
static app_signal_t at24c_tick_low_priority(storage_at24c_t *at24c, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    return app_task_execute_if_running_in_thread(&at24c->task, thread);
}

static app_signal_t at24c_on_report(storage_at24c_t *at24c, app_event_t *event) {
    switch (event->type) {
    case APP_EVENT_WRITE:
    case APP_EVENT_READ_TO_BUFFER:
        // return actor_event_handle_and_pass_to_task(at24c->actor, event, &at24c->task, at24c->actor->app->threads->low_priority,
        //                                        at24c->task.handler);

        // app_task_execute(&at24c->task);

        app_thread_actor_schedule(at24c->task.thread, at24c->actor, at24c->task.thread->current_time);
        return 0;
    default:
        return 0;
    }
}

actor_class_t storage_at24c_class = {
    .type = STORAGE_AT24C,
    .size = sizeof(storage_at24c_t),
    .phase_subindex = STORAGE_AT24C_PHASE,
    .validate = (app_method_t)at24c_validate,
    .construct = (app_method_t)at24c_construct,
    .link = (app_method_t)at24c_link,
    .start = (app_method_t)at24c_start,
    .stop = (app_method_t)at24c_stop,
    .on_phase = (actor_on_phase_t)at24c_phase,
    .property_write = at24c_property_write,
    .on_signal = (actor_on_signal_t)at24c_on_signal,
    .on_report = (actor_on_report_t)at24c_on_report,
    .tick_low_priority = (actor_on_tick_t)at24c_tick_low_priority,
    .tick_high_priority = (actor_on_tick_t)at24c_tick_high_priority,
    .tick_input = (actor_on_tick_t)at24c_tick_input,
};
