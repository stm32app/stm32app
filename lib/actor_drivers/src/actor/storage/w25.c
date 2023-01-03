#include "w25.h"
#include <actor/lib/dma.h>
#include <actor/transport/spi.h>

static actor_signal_t w25_spi_transfer(actor_job_t* job,
                                       uint8_t* write_data,
                                       size_t write_size,
                                       size_t read_size) {
  actor_publish(job->actor,
                &((actor_message_t){.type = ACTOR_MESSAGE_TRANSFER,
                                    .consumer = ((storage_w25_t*)job->actor->object)->spi,
                                    .producer = job->actor,
                                    .data = write_data,
                                    .size = write_size,
                                    .argument = (void*)read_size}));
  return ACTOR_SIGNAL_WAIT;
}

// Send command and receieve response
static actor_signal_t w25_set_lock(actor_job_t* job, bool state) {
  return w25_spi_transfer(job, (uint8_t[]){state ? W25_CMD_UNLOCK : W25_CMD_LOCK}, 1, 0);
}

// Query SR signal in a loop to check if actor is ready to accept commands
static actor_signal_t w25_task_wait_until_ready(actor_job_t* job,
                                                actor_signal_t signal,
                                                actor_t* caller) {
  actor_async_task_begin();
  do {
    w25_spi_transfer(job, (uint8_t[]){W25_CMD_READ_SR1}, 1, 1);
    actor_async_sleep();
  } while ((*job->incoming_message.data & W25_SR1_BUSY));
  actor_async_task_end();
}

static actor_signal_t w25_task_write_in_pages(actor_job_t* job,
                                            actor_signal_t signal,
                                            actor_t* caller,
                                              uint32_t address,
                                              uint8_t* data,
                                              size_t size,
                                              size_t page_size) {
  actor_async_task_begin();

  for (job->subroutine.counter = 0; job->subroutine.counter < size;) {
    uint32_t bytes_on_page =
        get_number_of_bytes_intesecting_page(address + job->subroutine.counter, size, page_size);
    w25_spi_transfer(job, data, bytes_on_page, bytes_on_page);
    actor_async_sleep();  // todo: signal handled message
    job->subroutine.counter += bytes_on_page;
  }

  actor_async_task_end();
}

static actor_signal_t w25_job_introspection(actor_job_t* job,
                                            actor_signal_t signal,
                                            actor_t* caller) {
  actor_async_job_begin();
  w25_task_wait_until_ready(job, signal, caller);
  w25_spi_transfer(job, (uint8_t[]){W25_CMD_MANUF_ACTOR, 0x00, 0x00, 0x00}, 4, 2);
  actor_async_job_end();
}

static actor_signal_t w25_job_lock(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(w25_task_wait_until_ready(job, signal, caller));
  w25_spi_transfer(job, (uint8_t[]){W25_CMD_LOCK}, 1, 0);
  actor_async_job_end();
}
static actor_signal_t w25_job_unlock(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(w25_task_wait_until_ready(job, signal, caller));
  w25_spi_transfer(job, (uint8_t[]){W25_CMD_UNLOCK}, 1, 0);
  actor_async_job_end();
}
static actor_signal_t w25_job_enable(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(w25_task_wait_until_ready(job, signal, caller));
  w25_spi_transfer(job, (uint8_t[]){W25_CMD_PWR_ON, 0xFF, 0xFF, 0xFF}, 4, 1);
  actor_async_job_end();
}

static actor_signal_t w25_job_disable(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(w25_task_wait_until_ready(job, signal, caller));
  w25_spi_transfer(job, (uint8_t[]){W25_CMD_PWR_OFF}, 1, 0);
  actor_async_job_end();
}

static actor_signal_t w25_job_write(actor_job_t* job, actor_signal_t signal, actor_t* caller) {
  actor_async_job_begin();
  actor_async_await(w25_task_wait_until_ready(job, signal, caller));
  w25_task_write_in_pages(job, signal, caller, (uint32_t)job->inciting_message.argument, job->inciting_message.data,
                          job->inciting_message.size, 256);
  actor_async_job_end();
}
static actor_signal_t w25_job_read(actor_job_t* job, actor_signal_t signal, actor_t* actor) {
  return 0;
}

static actor_signal_t w25_worker_high_priority(storage_w25_t* w25,
                                               actor_message_t* event,
                                               actor_worker_t* tick,
                                               actor_thread_t* thread) {
  return actor_job_execute_if_running_in_thread(w25->job, thread);
}

static actor_signal_t w25_validate(storage_w25_properties_t* properties) {
  return 0;
}

static actor_signal_t w25_construct(storage_w25_t* w25) {
  return 0;
}

static actor_signal_t w25_stop(storage_w25_t* w25) {
  return actor_message_handle_and_start_job(
      w25->actor, &((actor_message_t){.type = ACTOR_MESSAGE_DISABLE}), &w25->job,
      w25->actor->node->high_priority, w25_job_disable);
}

static actor_signal_t w25_start(storage_w25_t* w25) {
  // return actor_message_handle_and_start_job(w25->actor, &((actor_message_t){.type =
  // ACTOR_MESSAGE_ENABLE}), &w25->job,
  //                                           w25->actor->node->high_priority, w25_job_enable);
  return 0;
}

/* Link w25 actor with its spi module (i2c, spi, uart) */
static actor_signal_t w25_link(storage_w25_t* w25) {
  actor_link(w25->actor, (void**)&w25->spi, w25->properties->spi_index, NULL);
  return 0;
}

// Acknowledge returned event
static actor_signal_t w25_message_handled(storage_w25_t* w25,
                                          actor_message_t* event,
                                          actor_signal_t signal) {
  switch (event->type) {
    case ACTOR_MESSAGE_WRITE:
      actor_thread_actor_wakeup(w25->actor->node->high_priority, w25->actor) break;
    default:
      break;
  }
  return 0;
}

static actor_signal_t w25_worker_input(storage_w25_t* w25,
                                       actor_message_t* event,
                                       actor_worker_t* tick,
                                       actor_thread_t* thread) {
  switch (event->type) {
    case ACTOR_MESSAGE_INTROSPECTION:
      return actor_message_handle_and_start_job(
          w25->actor, event, &w25->job, w25->actor->node->high_priority, w25_job_introspection);
    case ACTOR_MESSAGE_WRITE:
      return actor_message_handle_and_start_job(w25->actor, event, &w25->job,
                                                w25->actor->node->high_priority, w25_job_write);
    case ACTOR_MESSAGE_READ:
      return actor_message_handle_and_start_job(w25->actor, event, &w25->job,
                                                w25->actor->node->high_priority, w25_job_read);
    case ACTOR_MESSAGE_LOCK:
      return actor_message_handle_and_start_job(w25->actor, event, &w25->job,
                                                w25->actor->node->high_priority, w25_job_lock);
    case ACTOR_MESSAGE_UNLOCK:
      return actor_message_handle_and_start_job(w25->actor, event, &w25->job,
                                                w25->actor->node->high_priority, w25_job_unlock);
    case ACTOR_MESSAGE_RESPONSE:
      if (event->producer == w25->spi && w25->job != NULL) {
        return actor_message_handle_and_pass_to_job(
            w25->actor, event, w25->job, w25->actor->node->high_priority, w25->job->handler);
      }
      break;
    default:
      break;
  }
  return 0;
}

static actor_worker_callback_t w25_worker_assignment(storage_w25_t* w25, actor_thread_t* thread) {
  if (thread == w25->actor->node->input) {
    return (actor_worker_callback_t)w25_worker_input;
  } else if (thread == w25->actor->node->high_priority) {
    return (actor_worker_callback_t)w25_worker_high_priority;
  }
  return NULL;
}

static actor_signal_t w25_phase_changed(storage_w25_t* w25, actor_phase_t phase) {
  switch (phase) {
    case ACTOR_PHASE_CONSTRUCTION:
      return w25_construct(w25);
    case ACTOR_PHASE_LINKAGE:
      return w25_link(w25);
    case ACTOR_PHASE_START:
      return w25_start(w25);
    case ACTOR_PHASE_STOP:
      return w25_stop(w25);
    default:
      return 0;
  }
}

actor_interface_t storage_w25_class = {
    .type = STORAGE_W25,
    .size = sizeof(storage_w25_t),
    .phase_subindex = STORAGE_W25_PHASE,
    .validate = (actor_method_t)w25_validate,
    .worker_assignment = (actor_worker_assignment_t)w25_worker_assignment,
    .phase_changed = (actor_phase_changed_t)w25_phase_changed,
    .message_handled = (actor_message_handled_t)w25_message_handled,
};
