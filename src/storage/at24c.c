#include "storage/at24c.h"
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

static app_signal_t at24c_step_send_ack(transport_i2c_t *i2c, uint32_t address, uint32_t size) {
}

static app_signal_t at24c_step_wait_ack(transport_i2c_t *i2c, uint32_t address, uint32_t size) {
}

static app_task_signal_t at24c_step_read(app_task_t *task, uint8_t address, uint32_t size) {
    storage_at24c_t *at24c = task->actor->object;
    switch (task->step_index) {
    case 0:
        app_publish(task->actor->app, &((app_event_t){
                                          .type = APP_EVENT_READ,
                                          .consumer = at24c->i2c->actor,
                                          .producer = task->actor,
                                          .size = size,
                                          .argument = i2c_pack_event_argument(at24c->properties->i2c_address, address),
                                      }));
        return APP_TASK_STEP_WAIT;
    case 1:
        configASSERT(task->awaited_event.size == size);
        return APP_TASK_STEP_COMPLETE;
    }
}

static app_signal_t at24c_step_write(app_task_t *task, uint8_t address, uint8_t *data, uint32_t size) {
    storage_at24c_t *at24c = task->actor->object;
    uint8_t *buffer;

    switch (task->step_index) {
    case 0:
        buffer = malloc(size);
        memcpy(buffer, data, size);
        app_publish(task->actor->app, &((app_event_t){
                                          .type = APP_EVENT_WRITE,
                                          .consumer = at24c->i2c->actor,
                                          .producer = task->actor,
                                          .size = size,
                                          .data = buffer,
                                          .argument = i2c_pack_event_argument(at24c->properties->i2c_address, address),
                                      }));
        return APP_TASK_STEP_WAIT;
    default:
        return APP_TASK_STEP_COMPLETE;
    }
}

static app_signal_t at24c_step_write_within_page(app_task_t *task, uint8_t address, uint8_t *data, uint32_t size) {
    storage_at24c_t *at24c = ((storage_at24c_t *)task->actor->object);
    app_publish(task->actor->app, &((app_event_t){
                                      .type = APP_EVENT_WRITE,
                                      .consumer = at24c->i2c->actor,
                                      .producer = task->actor,
                                      .size = size,
                                      .argument = i2c_pack_event_argument(at24c->properties->i2c_address, address),
                                  }));
}

static app_signal_t at24c_task_diagnose(app_task_t *task) {
    switch (task->phase_index) {
    case 0:
        return at24c_step_read(task, 0x0010, 2);
    case 1:
        if (task->data == NULL)
          task->data = ((uint8_t *) (uint32_t) task->awaited_event.data[0]);
        return at24c_step_write(task, 0x0010,
                                &((uint8_t[]){
                                    task->awaited_event.data[0] + 1,
                                    task->awaited_event.data[1],
                                }),
                                2);
    case 2:
        return at24c_step_read(task, 0x0010, 2);
    default:
        if (task->data + 1 == ((uint8_t *) (uint32_t)  task->awaited_event.data[0])) {
            log_printf("  - Diagnostics OK: 0x%x %s %s <= %s\n", actor_index(task->actor), get_actor_type_name(task->actor->class->type));
            return APP_TASK_COMPLETE;
        } else {
            log_printf("  - Diagnostics ERROR: 0x%x %s %s <= %s\n", actor_index(task->actor), get_actor_type_name(task->actor->class->type));
            return APP_TASK_FAILURE;
        }
    }
}

static app_task_signal_t at24c_task_read(app_task_t *task) {
}
static app_task_signal_t at24c_task_write(app_task_t *task) {
}
static app_task_signal_t at24c_task_erase(app_task_t *task) {
}

static app_signal_t at24c_tick_input(storage_at24c_t *at24c, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_DIAGNOSE:
        return actor_event_receive_and_start_task(at24c->actor, event, &at24c->task, at24c->actor->app->threads->low_priority,
                                                 at24c_task_diagnose);
    case APP_EVENT_RESPONSE:
        return actor_event_handle_and_pass_to_task(at24c->actor, event, &at24c->task, at24c->task.thread, at24c->task.handler);
    default:
        return 0;
    }
}

static app_signal_t at24c_tick_high_priority(storage_at24c_t *at24c, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    if (event->type == APP_EVENT_THREAD_ALARM && at24c->task.thread == thread) {
        return app_task_execute(&at24c->task);
    }
    return 0;
}
static app_signal_t at24c_tick_low_priority(storage_at24c_t *at24c, app_event_t *event, actor_tick_t *tick, app_thread_t *thread) {
    if ((event->type == APP_EVENT_THREAD_ALARM || event->type == APP_EVENT_THREAD_START) && at24c->task.thread == thread) {
        return app_task_execute(&at24c->task);
    }
    return 0;
}

static app_signal_t at24c_on_report(storage_at24c_t *at24c, app_event_t *event) {
    if (event->type == APP_EVENT_WRITE) {
        free(event->data);
        return app_task_execute(&at24c->task);
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
