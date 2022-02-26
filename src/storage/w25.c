#include "w25.h"
#include "transport/spi.h"
#include "lib/dma.h"

static app_job_signal_t w25_spi_transfer(app_job_t *job, uint8_t *write_data, size_t write_size, size_t read_size) {
    app_publish(job->actor->app, &((app_event_t){
                                       .type = APP_EVENT_TRANSFER,
                                       .consumer = ((storage_w25_t *)job->actor->object)->spi->actor,
                                       .producer = job->actor,
                                       .data = write_data,
                                       .size = write_size,
                                       .argument = (void *) read_size
                                   }));
    return APP_JOB_TASK_WAIT;
}

// Send command and receieve response
static app_job_signal_t w25_set_lock(app_job_t *job, bool_t state) {
    return w25_spi_transfer(job,(uint8_t[]){state ? W25_CMD_UNLOCK : W25_CMD_LOCK}, 1, 0);
}

// Query SR signal in a loop to check if actor is ready to accept commands
static app_job_signal_t w25_task_wait_until_ready(app_job_t *job) {
    switch (job->task_phase) {
    case 0: return w25_spi_transfer(job,(uint8_t[]){W25_CMD_READ_SR1}, 1, 1);
    default: return (*job->incoming_event.data & W25_SR1_BUSY) ? APP_JOB_TASK_RETRY : APP_JOB_TASK_SUCCESS;
    }
}

static app_job_signal_t w25_task_write_in_pages(app_job_t *job, uint32_t address, uint8_t *data, size_t size, size_t page_size) {
    uint32_t bytes_on_page = get_number_of_bytes_intesecting_page(address + job->counter, size, page_size);
    switch (job->task_phase) {
    case 0: return w25_set_lock(job,false);
    case 2: return w25_spi_transfer(job,data, bytes_on_page, bytes_on_page);
    default:
        job->counter += bytes_on_page;
        if (job->counter == size) {
            job->counter = 0;
            return APP_JOB_TASK_SUCCESS;
        } else {
            return APP_JOB_TASK_RETRY;
        }
    }
}

static app_job_signal_t w25_job_introspection(app_job_t *job) {
    switch (job->job_phase) {
    case 0: return w25_task_wait_until_ready(job);
    case 1: return w25_spi_transfer(job,(uint8_t[]){W25_CMD_MANUF_ACTOR, 0x00, 0x00, 0x00}, 4, 2);
    default: return APP_JOB_SUCCESS;
    }
}

static app_job_signal_t w25_send_command(app_job_t *job, uint8_t command) {
    switch (job->job_phase) {
    case 0: return w25_task_wait_until_ready(job);
    case 1: return w25_spi_transfer(job,(uint8_t[]){command}, 1, 0);
    default: return APP_JOB_SUCCESS;
    }
}
static app_job_signal_t w25_job_lock(app_job_t *job) {
    switch (job->job_phase) {
    case 0: return w25_task_wait_until_ready(job);
    case 1: return w25_set_lock(job,true);
    default: return APP_JOB_SUCCESS;
    }
}
static app_job_signal_t w25_job_unlock(app_job_t *job) {
    switch (job->job_phase) {
    case 0: return w25_task_wait_until_ready(job);
    case 1: return w25_set_lock(job,false);
    default: return APP_JOB_SUCCESS;
    }
}
static app_job_signal_t w25_job_enable(app_job_t *job) {
    switch (job->job_phase) {
    case 0: return w25_task_wait_until_ready(job);
    case 1: return w25_spi_transfer(job,(uint8_t[]){W25_CMD_PWR_ON, 0xFF, 0xFF, 0xFF}, 4, 1);
    default: return APP_JOB_SUCCESS;
    }
}

static app_job_signal_t w25_job_disable(app_job_t *job) {
    switch (job->job_phase) {
    case 0: return w25_task_wait_until_ready(job);
    case 1: return w25_spi_transfer(job,(uint8_t[]){W25_CMD_PWR_OFF}, 1, 0);
    default: return APP_JOB_SUCCESS;
    }
}
    
static app_job_signal_t w25_job_write(app_job_t *job) {
    switch (job->job_phase) {
    case 1: return w25_task_wait_until_ready(job);
    case 2:
        return w25_task_write_in_pages(job,(uint32_t)job->inciting_event.argument, job->inciting_event.data, job->inciting_event.size,
                                   256);
    default: return APP_JOB_SUCCESS;
    }
}
static app_job_signal_t w25_job_read(app_job_t *job) {
    switch (job->job_phase) {
    default: return APP_JOB_SUCCESS;
    }
}

static app_signal_t w25_worker_high_priority(storage_w25_t *w25, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    return app_job_execute_if_running_in_thread(&w25->job, thread);
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
    return actor_event_handle_and_start_job(w25->actor, &((app_event_t){.type = APP_EVENT_DISABLE}), &w25->job,
                                              w25->actor->app->high_priority, w25_job_disable);
}

static app_signal_t w25_start(storage_w25_t *w25) {
    //return actor_event_handle_and_start_job(w25->actor, &((app_event_t){.type = APP_EVENT_ENABLE}), &w25->job,
    //                                          w25->actor->app->high_priority, w25_job_enable);
    return 0;
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
static app_signal_t w25_on_event_report(storage_w25_t *w25, app_event_t *event) {
    switch (event->type) {
    case APP_EVENT_WRITE:
        app_thread_actor_schedule(w25->actor->app->high_priority, w25->actor,
                                   w25->actor->app->high_priority->current_time);
        break;
    default: break;
    }
    return 0;
}

static app_signal_t w25_worker_input(storage_w25_t *w25, app_event_t *event, actor_worker_t *tick, app_thread_t *thread) {
    switch (event->type) {
    case APP_EVENT_INTROSPECTION:
        return actor_event_handle_and_start_job(w25->actor, event, &w25->job, w25->actor->app->high_priority,
                                                  w25_job_introspection);
    case APP_EVENT_WRITE:
        return actor_event_handle_and_start_job(w25->actor, event, &w25->job, w25->actor->app->high_priority, w25_job_write);
    case APP_EVENT_READ:
        return actor_event_handle_and_start_job(w25->actor, event, &w25->job, w25->actor->app->high_priority, w25_job_read);
    case APP_EVENT_LOCK:
        return actor_event_handle_and_start_job(w25->actor, event, &w25->job, w25->actor->app->high_priority, w25_job_lock);
    case APP_EVENT_UNLOCK:
        return actor_event_handle_and_start_job(w25->actor, event, &w25->job, w25->actor->app->high_priority,
                                                  w25_job_unlock);
    case APP_EVENT_RESPONSE:
        if (event->producer == w25->spi->actor && w25->job != NULL) {
            return actor_event_handle_and_pass_to_job(w25->actor, event, &w25->job, w25->actor->app->high_priority,
                                                        w25->job->handler);
        }
        break;
    default:
        break;
    }
    return 0;
}

static actor_worker_callback_t w25_on_worker_assignment(storage_w25_t *w25, app_thread_t *thread) {
    if (thread == w25->actor->app->input) {
        return (actor_worker_callback_t) w25_worker_input;
    } else if (thread == w25->actor->app->high_priority) {
        return (actor_worker_callback_t) w25_worker_high_priority;
    }
    return NULL;
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
    .on_worker_assignment = (actor_on_worker_assignment_t) w25_on_worker_assignment,
    .on_phase = (actor_on_phase_t)w25_on_phase,
    .on_event_report = (actor_on_event_report_t)w25_on_event_report,
    .property_write = w25_property_write,
};
