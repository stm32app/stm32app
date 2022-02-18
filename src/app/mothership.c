#include "mothership.h"
//#include "actor/circuit.h"
//#include "actors/modbus.h"
#include "input/sensor.h"
#include "lib/sdram.h"
#include "module/adc.h"
#include "module/timer.h"
#include "storage/at24c.h"
#include "storage/sdcard.h"
#include "storage/w25.h"
#include "system/canopen.h"
#include "system/mcu.h"
//#include "screen/epaper.h"
//#include "transport/can.h"
//#include "transport/i2c.h"
#include "indicator/led.h"
#include "transport/i2c.h"
#include "transport/sdio.h"
#include "transport/spi.h"
//#include "transport/usart.h"

static ODR_t mothership_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    app_mothership_t *mothership = stream->object;
    (void)mothership;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);

    return result;
}

static app_signal_t mothership_validate(app_mothership_properties_t *properties) {
    return 0;
}

static app_signal_t mothership_construct(app_mothership_t *mothership) {
    if (!app_threads_allocate((app_t *)mothership)) {
        mothership->buffers = app_buffer_malloc(mothership->actor);
        return mothership->buffers == NULL;
    };
    return 1;
}

static app_signal_t mothership_start(app_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static app_signal_t mothership_stop(app_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static app_signal_t mothership_link(app_mothership_t *mothership) {
    actor_link(mothership->actor, (void **)&mothership->mcu, mothership->properties->mcu_index, NULL);
    actor_link(mothership->actor, (void **)&mothership->canopen, mothership->properties->canopen_index, NULL);
    actor_link(mothership->actor, (void **)&mothership->timer, mothership->properties->timer_index, NULL);
    return 0;
}

static app_signal_t mothership_phase(app_mothership_t *mothership, actor_phase_t phase) {
    (void)mothership;
    (void)phase;
    switch (phase) {
    case ACTOR_CONSTRUCTING:

//        start_sdram();
        break;

    case ACTOR_RUNNING:
        break;
    default:
        break;
    }
    return 0;
}

// Initialize all actors of all types found in OD
// If actors is NULL, it will only count the actors
size_t app_mothership_enumerate_actors(app_t *app, OD_t *od, actor_t *destination) {
    size_t count = 0;
    count += app_actor_type_enumerate(app, od, &app_mothership_class, destination, count);
    count += app_actor_type_enumerate(app, od, &system_mcu_class, destination, count);
    count += app_actor_type_enumerate(app, od, &system_canopen_class, destination, count);
    count += app_actor_type_enumerate(app, od, &module_timer_class, destination, count);
    count += app_actor_type_enumerate(app, od, &module_adc_class, destination, count);
    count += app_actor_type_enumerate(app, od, &transport_can_class, destination, count);
    count += app_actor_type_enumerate(app, od, &transport_spi_class, destination, count);
    //count += app_actor_type_enumerate(app, od, &transport_i2c_class, destination, count);
     count += app_actor_type_enumerate(app, od, &transport_sdio_class, destination, count);
    count += app_actor_type_enumerate(app, od, &storage_w25_class, destination, count);
    //count += app_actor_type_enumerate(app, od, &storage_at24c_class, destination, count);
    count += app_actor_type_enumerate(app, od, &storage_sdcard_class, destination, count);
    count += app_actor_type_enumerate(app, od, &indicator_led_class, destination, count);
    // count += app_actor_type_enumerate(MODULE_USART, &transport_usart_class, sizeof(transport_usart_t), destination, count);
    // count += app_actor_type_enumerate(app, od, TRANSPORT_I2C, &transport_i2c_class, sizeof(transport_i2c_t), destination, count);
    // count += app_actor_type_enumerate(app, od, DEVICE_CIRCUIT, &device_circuit_class, sizeof(device_circuit_t), destination, count);
    // count += app_actor_type_enumerate(app, OD, SCREEN_EPAPER, &screen_epaper_class, sizeof(screen_epaper_t), destination, count);
    // count += app_actor_type_enumerate(app, od, INPUT_SENSOR, &input_sensor_class, sizeof(input_sensor_t), destination, count);
    return count;
}

static app_signal_t mothership_high_priority(app_mothership_t *mothership, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    (void)tick;
    (void)thread;
    if (event->type == APP_EVENT_THREAD_ALARM && !mothership->initialized) {
        mothership->initialized = true;
        // test simple timeout
        module_timer_timeout(mothership->timer, mothership->actor, (void *)123, 1000000);

//        tick->next_time = thread->current_time + 1000;

        return app_publish(mothership->actor->app, &((app_event_t){.type = APP_EVENT_DIAGNOSE, .producer = mothership->actor}));
    }
    if (event->type == APP_EVENT_THREAD_ALARM && mothership->initialized && !mothership->sdram) {
        //finish_sdram();
    }
    return 0;
}

static app_job_signal_t mothership_job_stats(app_job_t *job) {
    if (job->job_phase == 0) {
        debug_printf("│ │ ├ Allocated buffers\n");
        for (app_buffer_t *page = job->actor->app->buffers; page; page = app_buffer_get_next_page(page, job->actor->app->buffers)) {
            for (uint32_t offset = 0; offset < page->allocated_size; offset += sizeof(app_buffer_t)) {
                app_buffer_t *buffer = (app_buffer_t *)&page->data[offset];
                if (!(buffer->owner == NULL && buffer->data == NULL && buffer->allocated_size == 0)) {
                    debug_printf("│ │ │ ├ %-16s%lub/%lub\n", get_actor_type_name(buffer->owner->class->type), buffer->size,
                                 buffer->allocated_size);
                }
            }
        }
        for (size_t i = 0; i < HEAP_NUM; i++) {
            debug_printf("│ │ ├ Heap #0x%lx\t%ub/%ub free, %u lowest\n", (uint32_t)multiRegionGetHeapStartAddress(i),
                         multiRegionGetFreeHeapSize(i), multiRegionGetHeapSize(i), multiRegionGetMinimumEverFreeHeapSize(i));
        }
    }
    return APP_JOB_SUCCESS;
}

static app_signal_t mothership_low_priority(app_mothership_t *mothership, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_THREAD_ALARM:
       // if (mothership->sdram)
            actor_event_receive_and_start_job(mothership->actor, event, &mothership->job, thread, mothership_job_stats);

        break;
    default:
        break;
    }

    app_job_execute_if_running_in_thread(&mothership->job, thread);
    tick->next_time = thread->current_time + 5000;
    return 0;
}

static app_signal_t mothership_on_signal(app_mothership_t mothership, actor_t *actor, app_signal_t signal, void *argument) {
    (void)mothership;
    (void)actor;
    (void)signal;
    (void)argument;
    // printf("Got signal!\n");
    return 0;
};

static app_signal_t mothership_on_buffer(app_mothership_t *mothership, app_buffer_t *buffer, uint32_t size) {

    // allocate another slab chunk for buffers that will be used by other moduels in the app
    if (app_buffer_get_last_page(buffer) == app_buffer_get_last_page(mothership->buffers)) {
        app_buffer_t *next = mothership->buffers->allocated_size == 0 ? mothership->buffers : app_buffer_malloc(mothership->actor);
        app_buffer_t *previous = app_buffer_get_last_page(buffer);
        app_buffer_set_next_page(previous, next);
        if (app_buffer_set_size(next, 10 * sizeof(app_buffer_t))) {
            return APP_SIGNAL_OUT_OF_MEMORY;
        }
        next->size = next->allocated_size;
    }
    return 0;
}

actor_class_t app_mothership_class = {
    .type = CORE_APP,
    .size = sizeof(app_mothership_t),
    .phase_subindex = CORE_APP_PHASE,
    .validate = (app_method_t)mothership_validate,
    .construct = (app_method_t)mothership_construct,
    .link = (app_method_t)mothership_link,
    .start = (app_method_t)mothership_start,
    .stop = (app_method_t)mothership_stop,
    .on_phase = (actor_on_phase_t)mothership_phase,
    .on_signal = (actor_on_signal_t)mothership_on_signal,
    .worker_high_priority = (actor_on_worker_t)mothership_high_priority,
    .worker_low_priority = (actor_on_worker_t)mothership_low_priority,
    .property_write = mothership_property_write,
    .on_buffer = (actor_on_buffer_t)mothership_on_buffer,
};
