#include <actor/lib/bytes.h>
#include <actor/storage/at24c.h>
#include <actor/transport/i2c.h>

static actor_signal_t at24c_validate(storage_at24c_properties_t* properties) {
  return 0;
}

static actor_signal_t at24c_construct(storage_at24c_t* at24c) {
  actor_message_subscribe(at24c->actor, ACTOR_MESSAGE_DIAGNOSE);
  return 0;
}

static actor_signal_t at24c_start(storage_at24c_t* at24c) {
  (void)at24c;
  return 0;
}

static actor_signal_t at24c_stop(storage_at24c_t* at24c) {
  (void)at24c;
  return 0;
}

static actor_signal_t at24c_link(storage_at24c_t* at24c) {
  actor_link(at24c->actor, (void**)&at24c->i2c, at24c->properties->i2c_index, NULL);
  return 0;
}

static actor_signal_t at24c_signal_received(storage_at24c_t* at24c,
                                            actor_signal_t signal,
                                            actor_t* caller,
                                            void* source) {
  return 0;
}

static actor_signal_t at24c_task_read(actor_job_t* job,
                                      actor_signal_t signal,
                                      actor_t* caller,
                                      uint8_t address,
                                      uint8_t* data,
                                      uint32_t size) {
  storage_at24c_t* at24c = job->actor->object;
  actor_async_task_begin();
  actor_async_assert(at24c->target_buffer = actor_buffer_target(job->actor, data, size),
                     ACTOR_SIGNAL_OUT_OF_MEMORY);

  for (job->async_task.counter = 0; job->async_task.counter < size;) {
    uint32_t bytes_on_page = get_number_of_bytes_intesecting_page(
        address + job->async_task.counter, size, at24c->properties->page_size);
    actor_publish(job->actor,
                  &((actor_message_t){
                      .type = ACTOR_MESSAGE_READ_TO_BUFFER,
                      .consumer = at24c->i2c,
                      .producer = job->actor,
                      .data = &at24c->target_buffer->data[job->async_task.counter],
                      .size = bytes_on_page,
                      .argument = i2c_pack_message_argument(at24c->properties->i2c_address,
                                                            address + job->async_task.counter),
                  }));
    actor_async_sleep();  // todo: signal handled message
    job->async_task.counter += bytes_on_page;
  }

  actor_async_task_end({
    if (async->status < 0)
      actor_buffer_release(at24c->target_buffer, at24c->actor);
  });
}

static actor_signal_t at24c_task_write(actor_job_t* job,
                                       actor_signal_t signal,
                                       actor_t* caller,
                                       uint8_t address,
                                       uint8_t* data,
                                       uint32_t size) {
  storage_at24c_t* at24c = job->actor->object;
  actor_async_task_begin();
  actor_async_assert(at24c->target_buffer = actor_buffer_snapshot(job->actor, data, size),
                     ACTOR_SIGNAL_OUT_OF_MEMORY);

  for (job->async_task.counter = 0; job->async_task.counter < size;) {
    uint32_t bytes_on_page = get_number_of_bytes_intesecting_page(
        address + job->async_task.counter, size, at24c->properties->page_size);
    actor_publish(job->actor,
                  &((actor_message_t){
                      .type = ACTOR_MESSAGE_WRITE,
                      .consumer = at24c->i2c,
                      .producer = job->actor,
                      .data = &at24c->source_buffer->data[job->async_task.counter],
                      .size = bytes_on_page,
                      .argument = i2c_pack_message_argument(at24c->properties->i2c_address,
                                                            address + job->async_task.counter),
                  }));
    actor_async_sleep();  // todo: signal handled message
    job->async_task.counter += bytes_on_page;
  }

  actor_async_task_end({ actor_buffer_release(at24c->target_buffer, at24c->actor); });
}

static actor_signal_t at24c_job_diagnose(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  storage_at24c_t* at24c = job->actor->object;
  actor_buffer_t* first_buffer;
  actor_async_task_begin();
  actor_async_await(at24c_task_read(job, signal, caller, 0x0010, NULL, 2));
  actor_async_await(at24c_task_write(job, signal, caller, 0x0010,
                                    &((uint8_t[]){
                                        at24c->target_buffer->data[0] + 1,
                                        at24c->target_buffer->data[1],
                                    })[0],
                                    2));
  job->output_buffer = at24c->target_buffer;
  at24c->target_buffer = NULL;
  actor_async_await(at24c_task_read(job, signal, caller, 0x0010, NULL, 2));

  if ((job->output_buffer->data[0] + 1 == at24c->target_buffer->data[0] &&
       job->output_buffer->data[1] == at24c->target_buffer->data[1] &&
       job->output_buffer->size == at24c->target_buffer->size)) {
    debug_printf("  - Diagnostics OK: 0x%x %s\n", actor_index(job->actor),
                 actor_stringify(job->actor));
  } else {
    debug_printf("  - Diagnostics ERROR: 0x%x %s\n", actor_index(job->actor),
                 actor_stringify(job->actor));
    actor_async_fail();
  }
  actor_async_task_end({
    actor_buffer_release(at24c->target_buffer, job->actor);
    actor_buffer_release(at24c->source_buffer, job->actor);
    actor_buffer_release((actor_buffer_t*)job->output_buffer, job->actor);
  });
}

static actor_signal_t at24c_job_read(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  return ACTOR_SIGNAL_JOB_COMPLETE;
}
static actor_signal_t at24c_job_write(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  return ACTOR_SIGNAL_JOB_COMPLETE;
}
static actor_signal_t at24c_job_erase(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  return ACTOR_SIGNAL_JOB_COMPLETE;
}

static actor_signal_t at24c_worker_input(storage_at24c_t* at24c,
                                         actor_message_t* event,
                                         actor_worker_t* tick,
                                         actor_thread_t* thread) {
  switch (event->type) {
    case ACTOR_MESSAGE_DIAGNOSE:
      return actor_message_receive_and_start_job(
          at24c->actor, event, &at24c->job, at24c->actor->node->low_priority, at24c_job_diagnose);
    case ACTOR_MESSAGE_READ:
      return actor_message_receive_and_start_job(at24c->actor, event, &at24c->job,
                                                 at24c->actor->node->low_priority, at24c_job_read);
    case ACTOR_MESSAGE_WRITE:
      return actor_message_receive_and_start_job(at24c->actor, event, &at24c->job,
                                                 at24c->actor->node->low_priority, at24c_job_write);
    case ACTOR_MESSAGE_ERASE:
      return actor_message_receive_and_start_job(at24c->actor, event, &at24c->job,
                                                 at24c->actor->node->low_priority, at24c_job_erase);
    case ACTOR_MESSAGE_RESPONSE:
      return actor_message_handle_and_pass_to_job(at24c->actor, event, at24c->job,
                                                  at24c->job->thread, at24c->job->handler);

    default:
      return 0;
  }
}

static actor_signal_t at24c_worker_high_priority(storage_at24c_t* at24c,
                                                 actor_message_t* event,
                                                 actor_worker_t* tick,
                                                 actor_thread_t* thread) {
  return actor_job_execute_if_running_in_thread(at24c->job, thread);
}
static actor_signal_t at24c_worker_low_priority(storage_at24c_t* at24c,
                                                actor_message_t* event,
                                                actor_worker_t* tick,
                                                actor_thread_t* thread) {
  return actor_job_execute_if_running_in_thread(at24c->job, thread);
}

static actor_signal_t at24c_message_handled(storage_at24c_t* at24c,
                                            actor_message_t* event,
                                            actor_signal_t signal) {
  switch (event->type) {
    case ACTOR_MESSAGE_WRITE:
    case ACTOR_MESSAGE_READ_TO_BUFFER:
      actor_thread_actor_wakeup(at24c->job->thread, at24c->actor);
      return 0;
    default:
      return 0;
  }
}

static actor_worker_callback_t at24c_worker_assignment(storage_at24c_t* at24c,
                                                       actor_thread_t* thread) {
  if (thread == at24c->actor->node->input) {
    return (actor_worker_callback_t)at24c_worker_input;
  } else if (thread == at24c->actor->node->high_priority) {
    return (actor_worker_callback_t)at24c_worker_high_priority;
  } else if (thread == at24c->actor->node->low_priority) {
    return (actor_worker_callback_t)at24c_worker_low_priority;
  }
  return NULL;
}

static actor_signal_t at24c_phase_changed(storage_at24c_t* at24c, actor_phase_t phase) {
  switch (phase) {
    case ACTOR_PHASE_CONSTRUCTION:
      return at24c_construct(at24c);
    case ACTOR_PHASE_LINKAGE:
      return at24c_link(at24c);
    case ACTOR_PHASE_START:
      return at24c_start(at24c);
    case ACTOR_PHASE_STOP:
      return at24c_stop(at24c);
    default:
      return 0;
  }
}

actor_interface_t storage_at24c_class = {
    .type = STORAGE_AT24C,
    .size = sizeof(storage_at24c_t),
    .phase_subindex = STORAGE_AT24C_PHASE,
    .validate = (actor_method_t)at24c_validate,
    .phase_changed = (actor_phase_changed_t)at24c_phase_changed,
    .signal_received = (actor_signal_received_t)at24c_signal_received,
    .message_handled = (actor_message_handled_t)at24c_message_handled,
    .worker_assignment = (actor_worker_assignment_t)at24c_worker_assignment,
};
