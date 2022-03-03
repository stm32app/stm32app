#include "mothership.h"
#include <actor/device/circuit.h>
//#include "actors/modbus.h"
#include <actor/input/sensor.h>
#include <actor/module/adc.h>
#include <actor/module/mcu.h>
#include <actor/module/sdram.h>
#include <actor/module/timer.h>
#include <actor/storage/at24c.h>
#include <actor/storage/sdcard.h>
#include <actor/storage/w25.h>
//#include "screen/epaper.h"
//#include <actor/transport/can.h>
//#include <actor/transport/i2c.h>
#include <actor/indicator/led.h>
#include <actor/transport/can.h>
#include <actor/transport/i2c.h>
#include <actor/transport/sdio.h>
#include <actor/transport/spi.h>
//#include <actor/transport/usart.h>


#if ACTOR_NODE_USE_CANOPEN
#include <actor/canopen.h>
#endif
#if ACTOR_NODE_USE_DATABASE
#include <actor/database.h>
#endif


static ODR_t mothership_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    actor_mothership_t *mothership = stream->object;
    (void)mothership;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);

    return result;
}

static actor_signal_t mothership_validate(actor_mothership_properties_t *properties) {
    return 0;
}

static actor_signal_t mothership_construct(actor_mothership_t *mothership) {
    mothership->buffers = actor_buffer_malloc(mothership->actor);
    mothership->jobs = actor_buffer_malloc(mothership->actor);
    actor_thread_create(&mothership->input, mothership->actor, (actor_procedure_t)actor_thread_execute, "Input", 300, 20, 5,
                        ACTOR_THREAD_SHARED);
    actor_thread_create(&mothership->catchup, mothership->actor, (actor_procedure_t)actor_thread_execute, "Catchup", 200, 100, 5,
                        ACTOR_THREAD_SHARED);
    actor_thread_create(&mothership->high_priority, mothership->actor, (actor_procedure_t)actor_thread_execute, "High P", 200, 1, 4,
                        ACTOR_THREAD_SHARED);
    actor_thread_create(&mothership->medium_priority, mothership->actor, (actor_procedure_t)actor_thread_execute, "Medium P", 200, 1, 3,
                        ACTOR_THREAD_SHARED);
    actor_thread_create(&mothership->low_priority, mothership->actor, (actor_procedure_t)actor_thread_execute, "Low P", 200, 1, 2,
                        ACTOR_THREAD_SHARED);
    actor_thread_create(&mothership->bg_priority, mothership->actor, (actor_procedure_t)actor_thread_execute, "Bg P", 200, 0, 1,
                        ACTOR_THREAD_SHARED);
    return mothership->buffers == NULL || mothership->input == NULL || mothership->catchup == NULL || mothership->high_priority == NULL ||
           mothership->medium_priority == NULL || mothership->low_priority == NULL || mothership->bg_priority == NULL;
}

static actor_signal_t mothership_destruct(actor_mothership_t *mothership) {
    actor_thread_destroy(mothership->input);
    actor_thread_destroy(mothership->medium_priority);
    actor_thread_destroy(mothership->high_priority);
    actor_thread_destroy(mothership->low_priority);
    actor_thread_destroy(mothership->bg_priority);
    actor_buffer_release(mothership->buffers, mothership->actor);
    return 0;
}

static actor_signal_t mothership_start(actor_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static actor_signal_t mothership_stop(actor_mothership_t *mothership) {
    (void)mothership;
    return 0;
}

static actor_signal_t mothership_link(actor_mothership_t *mothership) {
    actor_link(mothership->actor, (void **)&mothership->mcu, mothership->properties->mcu_index, NULL);
#if ACTOR_NODE_USE_CANOPEN
    actor_link(mothership->actor, (void **)&mothership->canopen, mothership->properties->canopen_index, NULL);
#endif
    actor_link(mothership->actor, (void **)&mothership->timer, mothership->properties->timer_index, NULL);
    return 0;
}

static actor_signal_t mothership_phase(actor_mothership_t *mothership, actor_phase_t phase) {
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
    return 1;
}

// Initialize all actors of all types found in OD
// If actors is NULL, it will only count the actors
size_t actor_mothership_enumerate_actors(actor_node_t *node, OD_t *od, actor_t *destination) {
    size_t count = 0;
    count += actor_node_type_enumerate(node, od, &actor_mothership_class, destination, count);
    count += actor_node_type_enumerate(node, od, &module_mcu_class, destination, count);
#if ACTOR_NODE_USE_CANOPEN
    count += actor_node_type_enumerate(node, od, &actor_canopen_class, destination, count);
#endif
#if ACTOR_NODE_USE_DATABASE
    count += actor_node_type_enumerate(node, od, &actor_database_class, destination, count);
#endif
    count += actor_node_type_enumerate(node, od, &module_timer_class, destination, count);
    count += actor_node_type_enumerate(node, od, &module_adc_class, destination, count);
    count += actor_node_type_enumerate(node, od, &transport_can_class, destination, count);
    count += actor_node_type_enumerate(node, od, &transport_spi_class, destination, count);
    count += actor_node_type_enumerate(node, od, &transport_i2c_class, destination, count);
    count += actor_node_type_enumerate(node, od, &transport_sdio_class, destination, count);
    count += actor_node_type_enumerate(node, od, &storage_w25_class, destination, count);
    count += actor_node_type_enumerate(node, od, &storage_at24c_class, destination, count);
    count += actor_node_type_enumerate(node, od, &storage_sdcard_class, destination, count);
    count += actor_node_type_enumerate(node, od, &indicator_led_class, destination, count);
    // count += actor_node_type_enumerate(node, od, &transport_usart_class, sizeof(transport_usart_t), destination, count);
    // count += actor_node_type_enumerate(node, od, &device_circuit_class, sizeof(device_circuit_t), destination, count);
    // count += actor_node_type_enumerate(node, od, &screen_epaper_class, sizeof(screen_epaper_t), destination, count);
    // count += actor_node_type_enumerate(node, od, &input_sensor_class, sizeof(input_sensor_t), destination, count);
    return count;
}

static actor_signal_t mothership_worker_high_priority(actor_mothership_t *mothership, actor_event_t *event, actor_worker_t *tick,
                                                      actor_thread_t *thread) {
    (void)tick;
    (void)thread;
    if (event->type == ACTOR_EVENT_THREAD_ALARM && !mothership->initialized) {
        mothership->initialized = true;
        // test simple timeout
        module_timer_timeout(mothership->timer, mothership->actor, (void *)123, 1000000);

        //        tick->next_time = thread->current_time + 1000;
        actor_publish_event(mothership->actor, ACTOR_EVENT_DIAGNOSE);
        return actor_publish_event(mothership->actor, ACTOR_EVENT_START);
    }
    if (event->type == ACTOR_EVENT_THREAD_ALARM && mothership->initialized && !mothership->sdram) {
        // finish_sdram();
    }
    return 0;
}

static actor_signal_t mothership_worker_input(actor_mothership_t *mothership, actor_event_t *event, actor_worker_t *tick,
                                              actor_thread_t *thread) {
    return 0;
}

static actor_job_signal_t mothership_job_stats(actor_job_t *job) {
    if (job->job_phase == 0) {
        debug_printf("│ │ ├ Allocated buffers\n");
        for (actor_buffer_t *page = job->actor->node->buffers; page; page = actor_buffer_get_next_page(page, job->actor->node->buffers)) {
            for (uint32_t offset = 0; offset < page->allocated_size; offset += sizeof(actor_buffer_t)) {
                actor_buffer_t *buffer = (actor_buffer_t *)&page->data[offset];
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
    return ACTOR_JOB_SUCCESS;
}

static actor_signal_t mothership_worker_low_priority(actor_mothership_t *mothership, actor_event_t *event, actor_worker_t *tick,
                                                     actor_thread_t *thread) {
    switch (event->type) {
    case ACTOR_EVENT_THREAD_ALARM:
        // if (mothership->sdram)
        actor_event_receive_and_start_job(mothership->actor, event, &mothership->job, thread, mothership_job_stats);

        break;
    default:
        break;
    }

    actor_job_execute_if_running_in_thread(&mothership->job, thread);
    tick->next_time = thread->current_time + 5000;
    return 0;
}

static actor_signal_t mothership_on_signal(actor_mothership_t mothership, actor_t *actor, actor_signal_t signal, void *argument) {
    (void)mothership;
    (void)actor;
    (void)signal;
    (void)argument;
    // printf("Got signal!\n");
    return 0;
};

static actor_signal_t mothership_on_buffer_allocation(actor_mothership_t *mothership, actor_buffer_t *buffer, uint32_t size) {
    // allocate another slab chunk for buffers that will be used by other moduels in the node
    actor_buffer_t *last_page = actor_buffer_get_last_page(buffer);
    if (last_page == actor_buffer_get_last_page(mothership->buffers)) {
        actor_buffer_t *next = mothership->buffers->allocated_size == 0 ? mothership->buffers : actor_buffer_malloc(mothership->actor);
        actor_buffer_t *previous = actor_buffer_get_last_page(buffer);
        actor_buffer_set_next_page(previous, next);
        if (actor_buffer_set_size(next, 10 * sizeof(actor_buffer_t))) {
            return ACTOR_SIGNAL_OUT_OF_MEMORY;
        }
        next->size = next->allocated_size;
        // allocate pool of jobs 10 at a time
    } else if (last_page == actor_buffer_get_last_page(mothership->jobs)) {
        actor_buffer_t *next = mothership->jobs->allocated_size == 0 ? mothership->jobs : actor_buffer_malloc(mothership->actor);
        actor_buffer_t *previous = actor_buffer_get_last_page(buffer);
        actor_buffer_set_next_page(previous, next);
        if (actor_buffer_set_size(next, 10 * sizeof(actor_job_t))) {
            return ACTOR_SIGNAL_OUT_OF_MEMORY;
        }
        next->size = next->allocated_size;
    }
    return 0;
}

static actor_worker_callback_t mothership_on_worker_assignment(actor_mothership_t *mothership, actor_thread_t *thread) {
    if (thread == mothership->input) {
        return (actor_worker_callback_t)mothership_worker_input;
    } else if (thread == mothership->high_priority) {
        return (actor_worker_callback_t)mothership_worker_high_priority;
    } else if (thread == mothership->low_priority) {
        return (actor_worker_callback_t)mothership_worker_low_priority;
    }
    return NULL;
}

actor_class_t actor_mothership_class = {
    .type = ACTOR_MOTHERSHIP,
    .size = sizeof(actor_mothership_t),
    .phase_subindex = ACTOR_MOTHERSHIP_PHASE,
    .validate = (actor_method_t)mothership_validate,
    .construct = (actor_method_t)mothership_construct,
    .destruct = (actor_method_t)mothership_destruct,
    .link = (actor_method_t)mothership_link,
    .start = (actor_method_t)mothership_start,
    .stop = (actor_method_t)mothership_stop,
    .on_phase = (actor_on_phase_t)mothership_phase,
    .on_signal = (actor_on_signal_t)mothership_on_signal,
    .property_write = mothership_property_write,
    .on_worker_assignment = (actor_on_worker_assignment_t)mothership_on_worker_assignment,
    .on_buffer_allocation = (actor_on_buffer_allocation_t)mothership_on_buffer_allocation,
};
