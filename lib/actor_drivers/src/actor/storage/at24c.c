#include <actor/storage/at24c.h>
#include <actor/lib/bytes.h>
#include <actor/transport/i2c.h>

static ODR_t at24c_property_write(OD_stream_t *stream, const void *buf, OD_size_t count, OD_size_t *countWritten) {
    storage_at24c_t *at24c = stream->object;
    (void)at24c;
    ODR_t result = OD_writeOriginal(stream, buf, count, countWritten);
    return result;
}

static actor_signal_t at24c_validate(storage_at24c_properties_t *properties) {
    return 0;
}

static actor_signal_t at24c_construct(storage_at24c_t *at24c) {
    actor_event_subscribe(at24c->actor, ACTOR_EVENT_DIAGNOSE);
    return 0;
}

static actor_signal_t at24c_start(storage_at24c_t *at24c) {
    (void)at24c;
    return 0;
}

static actor_signal_t at24c_stop(storage_at24c_t *at24c) {
    (void)at24c;
    return 0;
}

static actor_signal_t at24c_link(storage_at24c_t *at24c) {
    actor_link(at24c->actor, (void **)&at24c->i2c, at24c->properties->i2c_index, NULL);
    return 0;
}

static actor_signal_t at24c_phase(storage_at24c_t *at24c, actor_phase_t phase) {
    return 0;
}

static actor_signal_t at24c_on_signal(storage_at24c_t *at24c, actor_t *actor, actor_signal_t signal, void *source) {
    return 0;
}

static actor_job_signal_t at24c_task_read(actor_job_t *job, uint8_t address, uint8_t *data, uint32_t size) {
    storage_at24c_t *at24c = job->actor->object;
    uint32_t bytes_on_page;

    switch (job->task_phase) {
    case 0:
        job->counter = 0;
        at24c->target_buffer = actor_buffer_target(job->actor, data, size);
        if (at24c->target_buffer == NULL) {
            return ACTOR_JOB_FAILURE;
        } else {
            return ACTOR_JOB_TASK_CONTINUE;
        }
    case 1:
        bytes_on_page = get_number_of_bytes_intesecting_page(address + job->counter, size, at24c->properties->page_size);
        actor_publish(job->actor->node, &((actor_event_t){
                                          .type = ACTOR_EVENT_READ_TO_BUFFER,
                                          .consumer = at24c->i2c->actor,
                                          .producer = job->actor,
                                          .data = &at24c->target_buffer->data[job->counter],
                                          .size = bytes_on_page,
                                          .argument = i2c_pack_event_argument(at24c->properties->i2c_address, address + job->counter),
                                      }));
        job->counter += bytes_on_page;
        if (job->counter == size) {
            return ACTOR_JOB_TASK_WAIT;
        } else {
            return ACTOR_JOB_TASK_LOOP;
        }
    case 2:
        return ACTOR_JOB_TASK_SUCCESS;
    }

    return 0;
}

static actor_signal_t at24c_task_write(actor_job_t *job, uint8_t address, uint8_t *data, uint32_t size) {
    storage_at24c_t *at24c = job->actor->object;
    uint32_t bytes_on_page;

    switch (job->task_phase) {
    case 0:
        job->counter = 0;
        at24c->source_buffer = actor_buffer_snapshot(job->actor, data, size);
        if (at24c->source_buffer == NULL) {
            return ACTOR_JOB_FAILURE;
        } else {
            return ACTOR_JOB_TASK_CONTINUE;
        }
    case 1:
        bytes_on_page = get_number_of_bytes_intesecting_page(address + job->counter, size, at24c->properties->page_size);
        actor_publish(job->actor->node, &((actor_event_t){
                                          .type = ACTOR_EVENT_WRITE,
                                          .consumer = at24c->i2c->actor,
                                          .producer = job->actor,
                                          .data = &at24c->source_buffer->data[job->counter],
                                          .size = bytes_on_page,
                                          .argument = i2c_pack_event_argument(at24c->properties->i2c_address, address + job->counter),
                                      }));
        job->counter += bytes_on_page;
        if (job->counter == size) {
            return ACTOR_JOB_TASK_WAIT;
        } else {
            return ACTOR_JOB_TASK_LOOP;
        }
    case 2:
        return ACTOR_JOB_TASK_SUCCESS;
    }
    return 0;
}
static actor_job_signal_t at24c_job_diagnose(actor_job_t *job) {
    storage_at24c_t *at24c = job->actor->object;
    actor_buffer_t *first_buffer;
    bool ok;

    switch (job->job_phase) {
    case 0:
        return at24c_task_read(job,0x0010, NULL, 2);
    case 1:
        return at24c_task_write(job,0x0010,
                                &((uint8_t[]){
                                    at24c->target_buffer->data[0] + 1,
                                    at24c->target_buffer->data[1],
                                })[0],
                                2);
    case 2:
        if (job->task_phase == 0) {
            job->result = at24c->target_buffer;
            at24c->target_buffer = NULL;
        }
        return at24c_task_read(job,0x0010, NULL, 2);

    case 3:
        first_buffer = (actor_buffer_t *)job->result;
        ok = (first_buffer->data[0] + 1 == at24c->target_buffer->data[0] && first_buffer->data[1] == at24c->target_buffer->data[1] &&
              first_buffer->size == at24c->target_buffer->size);

        if (ok) {
            debug_printf("  - Diagnostics OK: 0x%x %s\n", actor_index(job->actor), get_actor_type_name(job->actor->class->type));
            return ACTOR_JOB_SUCCESS;
        } else {
            debug_printf("  - Diagnostics ERROR: 0x%x %s\n", actor_index(job->actor), get_actor_type_name(job->actor->class->type));
            return ACTOR_JOB_FAILURE;
        }
        break;

    case ACTOR_JOB_SUCCESS:
    case ACTOR_JOB_FAILURE:
        actor_buffer_release(at24c->target_buffer, job->actor);
        actor_buffer_release(at24c->source_buffer, job->actor);
        actor_buffer_release((actor_buffer_t *)job->result, job->actor);
    }

    return ACTOR_JOB_SUCCESS;
}

static actor_job_signal_t at24c_job_read(actor_job_t *job) {
    return ACTOR_JOB_SUCCESS;
}
static actor_job_signal_t at24c_job_write(actor_job_t *job) {
    return ACTOR_JOB_SUCCESS;
}
static actor_job_signal_t at24c_job_erase(actor_job_t *job) {
    return ACTOR_JOB_SUCCESS;
}

static actor_signal_t at24c_worker_input(storage_at24c_t *at24c, actor_event_t *event, actor_worker_t *tick, actor_thread_t *thread) {
    switch (event->type) {
    case ACTOR_EVENT_DIAGNOSE:
        return actor_event_receive_and_start_job(at24c->actor, event, &at24c->job, at24c->actor->node->low_priority,
                                                  at24c_job_diagnose);
    case ACTOR_EVENT_RESPONSE:
        return actor_event_handle_and_pass_to_job(at24c->actor, event, &at24c->job, at24c->job->thread, at24c->job->handler);

    case ACTOR_EVENT_READ:
        return actor_event_receive_and_start_job(at24c->actor, event, &at24c->job, at24c->actor->node->low_priority,
                                                  at24c_job_read);
    case ACTOR_EVENT_WRITE:
        return actor_event_receive_and_start_job(at24c->actor, event, &at24c->job, at24c->actor->node->low_priority,
                                                  at24c_job_write);
    case ACTOR_EVENT_ERASE:
        return actor_event_receive_and_start_job(at24c->actor, event, &at24c->job, at24c->actor->node->low_priority,
                                                  at24c_job_erase);
    default:
        return 0;
    }
}

static actor_signal_t at24c_worker_high_priority(storage_at24c_t *at24c, actor_event_t *event, actor_worker_t *tick, actor_thread_t *thread) {
    return actor_job_execute_if_running_in_thread(&at24c->job, thread);
}
static actor_signal_t at24c_worker_low_priority(storage_at24c_t *at24c, actor_event_t *event, actor_worker_t *tick, actor_thread_t *thread) {
    return actor_job_execute_if_running_in_thread(&at24c->job, thread);
}

static actor_signal_t at24c_on_event_report(storage_at24c_t *at24c, actor_event_t *event) {
    switch (event->type) {
    case ACTOR_EVENT_WRITE:
    case ACTOR_EVENT_READ_TO_BUFFER:
        // return actor_event_handle_and_pass_to_job(at24c->actor, event, &at24c->job, at24c->actor->node->low_priority,
        //                                        at24c->job.handler);

        // actor_job_execute(&at24c->job);

        actor_thread_actor_schedule(at24c->job->thread, at24c->actor, at24c->job->thread->current_time);
        return 0;
    default:
        return 0;
    }
}

static actor_worker_callback_t at24c_on_worker_assignment(storage_at24c_t *at24c, actor_thread_t *thread) {
    if (thread == at24c->actor->node->input) {
        return (actor_worker_callback_t) at24c_worker_input;
    } else if (thread == at24c->actor->node->high_priority) {
        return (actor_worker_callback_t) at24c_worker_high_priority;
    } else if (thread == at24c->actor->node->low_priority) {
        return (actor_worker_callback_t) at24c_worker_low_priority;
    }
    return NULL;
}

actor_class_t storage_at24c_class = {
    .type = STORAGE_AT24C,
    .size = sizeof(storage_at24c_t),
    .phase_subindex = STORAGE_AT24C_PHASE,
    .validate = (actor_method_t)at24c_validate,
    .construct = (actor_method_t)at24c_construct,
    .link = (actor_method_t)at24c_link,
    .start = (actor_method_t)at24c_start,
    .stop = (actor_method_t)at24c_stop,
    .on_phase = (actor_on_phase_t)at24c_phase,
    .property_write = at24c_property_write,
    .on_signal = (actor_on_signal_t)at24c_on_signal,
    .on_event_report = (actor_on_event_report_t)at24c_on_event_report,
    .on_worker_assignment = (actor_on_worker_assignment_t) at24c_on_worker_assignment,
};
