#ifndef INC_CORE_EVENT
#define INC_CORE_EVENT

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/env.h>
#include <actor/types.h>

enum actor_message_type {
    // Internal events that dont need subscription
    ACTOR_MESSAGE_THREAD_START = 32, // Set up a schedule, prepare for work
    ACTOR_MESSAGE_THREAD_STOP,       // Deallocate and destruct working objects
    ACTOR_MESSAGE_THREAD_ALARM,      // Wake up by software timer alarm

    ACTOR_MESSAGE_IDLE = 0,       // initial/empty state of an event
    ACTOR_MESSAGE_READ,           // abstract peripherial read events for cases with known receiver
    ACTOR_MESSAGE_READ_TO_BUFFER, // read without sending ACTOR_MESSAGE_RESPONSE
    ACTOR_MESSAGE_WRITE,
    ACTOR_MESSAGE_TRANSFER,

    ACTOR_MESSAGE_ERASE,
    ACTOR_MESSAGE_MOUNT,
    ACTOR_MESSAGE_UNMOUNT,
    ACTOR_MESSAGE_RESPONSE,
    ACTOR_MESSAGE_LOCK,
    ACTOR_MESSAGE_UNLOCK,
    ACTOR_MESSAGE_INTROSPECTION,
    ACTOR_MESSAGE_DIAGNOSE,

    ACTOR_MESSAGE_OPEN,
    ACTOR_MESSAGE_QUERY,
    ACTOR_MESSAGE_DELETE,
    ACTOR_MESSAGE_CLOSE,
    ACTOR_MESSAGE_SYNC,
    ACTOR_MESSAGE_STATS,

    ACTOR_MESSAGE_START,
    ACTOR_MESSAGE_STOP,
    ACTOR_MESSAGE_ENABLE,
    ACTOR_MESSAGE_DISABLE,

};

/* Is event owned by some specific actor */
enum actor_message_status {
    ACTOR_MESSAGE_WAITING,   // Event is waiting to be routed
    ACTOR_MESSAGE_RECEIVED,  // Some actors receieved the event
    ACTOR_MESSAGE_ADDRESSED, // A actor that could handle event was busy, others still can claim it
    ACTOR_MESSAGE_HANDLED,   // Device processed the event so no others will receive it
    ACTOR_MESSAGE_DEFERRED,  // A busy actor wants this event exclusively
    ACTOR_MESSAGE_FINALIZED
};

enum actor_signal {
    ACTOR_SIGNAL_OK = 0,
    ACTOR_SIGNAL_REPEAT,
    ACTOR_SIGNAL_ACCEPTED,
    ACTOR_SIGNAL_UNAFFECTED,

    ACTOR_SIGNAL_TIMER,
    ACTOR_SIGNAL_DMA_TRANSFERRING,
    ACTOR_SIGNAL_DMA_IDLE,

    ACTOR_SIGNAL_CANCEL,

    ACTOR_SIGNAL_RX_COMPLETE,
    ACTOR_SIGNAL_TX_COMPLETE,

    ACTOR_SIGNAL_CATCHUP,
    ACTOR_SIGNAL_RESCHEDULE,
    ACTOR_SIGNAL_INCOMING,

    ACTOR_SIGNAL_LINK,

    // Asynchronous action is happening, await callback
    ACTOR_SIGNAL_WAIT,
    // Jobs: Do not advance task/job counters, next job invocation will retry the action
    ACTOR_SIGNAL_LOOP,
    // Jobs: Move to next task
    ACTOR_SIGNAL_TASK_COMPLETE,
    // Jobs: Finish job
    ACTOR_SIGNAL_JOB_COMPLETE,


    ACTOR_SIGNAL_FAILURE = -1,
    ACTOR_SIGNAL_TIMEOUT = -2,
    ACTOR_SIGNAL_DMA_ERROR = -3,
    ACTOR_SIGNAL_BUSY = -4,
    ACTOR_SIGNAL_NOT_FOUND = -5,
    ACTOR_SIGNAL_UNCONFIGURED = -6,
    ACTOR_SIGNAL_INCOMPLETE = -7,
    ACTOR_SIGNAL_CORRUPTED = -8,
    ACTOR_SIGNAL_NO_RESPONSE = -9,
    ACTOR_SIGNAL_OUT_OF_MEMORY = -10,
    ACTOR_SIGNAL_INVALID_ARGUMENT = -11,
    ACTOR_SIGNAL_NOT_IMPLEMENTED = -12,
    ACTOR_SIGNAL_TRANSFER_FAILED = -13,
    ACTOR_SIGNAL_UNSUPPORTED = -14
};

struct actor_message {
    actor_message_type_t type;     /* Kind of event*/
    actor_message_status_t status; /* Status of events handling*/
    uint8_t *data;               /* Pointer to data package*/
    uint32_t size;               /* Size of data payload*/
    void *argument;              /* Optional argument */
    actor_t *producer;           /* Where event originated at */
    actor_t *consumer;           /* Device that handled the event*/
};

/* Attempt to store event in a memory destination if it's not occupied yet */
actor_signal_t actor_message_accept_and_process_generic(actor_t *actor, actor_message_t *event, actor_message_t *destination,
                                                      actor_message_status_t ready_status, actor_message_status_t busy_status,
                                                      actor_on_message_t handler);

actor_signal_t actor_message_accept_and_start_job_generic(actor_t *actor, actor_message_t *event, actor_job_t **job_slot,
                                                        actor_thread_t *thread, actor_job_handler_t handler,
                                                        actor_message_status_t ready_status, actor_message_status_t busy_status);

actor_signal_t actor_message_accept_and_pass_to_job_generic(actor_t *actor, actor_message_t *event, actor_job_t *job, actor_thread_t *thread,
                                                          actor_job_handler_t handler, actor_message_status_t ready_status,
                                                          actor_message_status_t busy_status);

/* Consume event if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_message_handle(actor, event, destination)                                                                                      \
    actor_message_accept_and_process_generic(actor, event, destination, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_DEFERRED, NULL)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_message_handle_and_process(actor, event, destination, handler)                                                                 \
    actor_message_accept_and_process_generic(actor, event, destination, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_DEFERRED,                           \
                                           (actor_on_message_t)handler)
/* Consume event by starting a task if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_message_handle_and_start_job(actor, event, task, thread, handler)                                                              \
    actor_message_accept_and_start_job_generic(actor, event, task, thread, handler, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_DEFERRED)
/* Consume event with a running task if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_message_handle_and_pass_to_job(actor, event, task, thread, handler)                                                            \
    actor_message_accept_and_pass_to_job_generic(actor, event, task, thread, handler, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_DEFERRED)

/* Consume event if not busy, otherwise keep it enqueued for later unless other actors take it first  */
#define actor_message_accept(actor, event, destination)                                                                                      \
    actor_message_accept_and_process_generic(actor, event, destination, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_ADDRESSED, NULL)
/* Consume event with a given handler if not busy, otherwise keep it enqueued for later unless other actors take it first */
#define actor_message_accept_and_process(actor, event, destination, handler)                                                                 \
    actor_message_accept_and_process_generic(actor, event, destination, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_ADDRESSED, (actor_message_t)handler)
/* Consume event by starting a task if not busy, otherwise keep it enqueued for later unless other actors take it first  */
#define actor_message_accept_and_start_job(actor, event, task, thread, handler)                                                              \
    actor_message_accept_and_start_job_generic(actor, event, task, thread, handler, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_ADDRESSED)
/* Consume event with a running task  if not busy, otherwise keep it enqueued for later unless other actors take it first */
#define actor_message_accept_and_pass_to_job(actor, event, task, thread, handler)                                                            \
    actor_message_accept_and_pass_to_job_generic(actor, event, task, thread, handler, ACTOR_MESSAGE_HANDLED, ACTOR_MESSAGE_ADDRESSED)
/* Process event and let others receieve it too */
#define actor_message_receive(actor, event, destination)                                                                                     \
    actor_message_accept_and_process_generic(actor, event, destination, ACTOR_MESSAGE_RECEIVED, ACTOR_MESSAGE_RECEIVED, NULL)
/* Process event with a given handler and let others receieve it too */
#define actor_message_receive_and_process(actor, event, destination, handler)                                                                \
    actor_message_accept_and_process_generic(actor, event, destination, ACTOR_MESSAGE_RECEIVED, ACTOR_MESSAGE_RECEIVED, (actor_message_t)handler)
/* Consume even by starting a task if not busy, and let others receieve it too  */
#define actor_message_receive_and_start_job(actor, event, task, thread, handler)                                                             \
    actor_message_accept_and_start_job_generic(actor, event, task, thread, handler, ACTOR_MESSAGE_RECEIVED, ACTOR_MESSAGE_RECEIVED)
/* Consume event with a running task  if not busy, and let others receieve it too  */
#define actor_message_receive_and_pass_to_job(actor, event, task, thread, handler)                                                           \
    actor_message_accept_and_pass_to_job_generic(actor, event, task, thread, handler, ACTOR_MESSAGE_RECEIVED, ACTOR_MESSAGE_RECEIVED)

actor_signal_t actor_worker_catchup(actor_t *actor, actor_worker_t *tick);

actor_signal_t actor_message_report(actor_t *actor, actor_message_t *event, actor_signal_t signal);
actor_signal_t actor_message_finalize(actor_t *actor, actor_message_t *event, actor_signal_t signal);

actor_signal_t actor_publish_message_generic(actor_t *actor, actor_message_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                           void *argument);
#define actor_publish_message(actor, type) actor_publish_message_generic(actor, type, NULL, NULL, 0, NULL)
#define actor_publish_message_for(actor, type, target) actor_publish_message_generic(actor, type, target, NULL, 0, NULL)
#define actor_publish_message_with_data(actor, type, data, size) actor_publish_message_generic(actor, type, NULL, data, size, NULL)
#define actor_publish_message_with_data_for(actor, type, target, data, size)                                                                 \
    actor_publish_message_generic(actor, type, target, data, size, NULL)
#define actor_publish_message_with_argument(actor, type, data, size, arg) actor_publish_message_generic(actor, type, NULL, data, size, arg)
#define actor_publish_message_with_argument_for(actor, type, target, data, size, arg)                                                        \
    actor_publish_message_generic(actor, type, target, data, size, arg)

actor_signal_t actor_job_publish_message_generic(actor_job_t *job, actor_message_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                               void *argument);

#define actor_job_publish_message(job, type) actor_job_publish_message_generic(job, type, NULL, NULL, 0, NULL)
#define actor_job_publish_message_for(job, type, target) actor_job_publish_message_generic(job, type, target, NULL, 0, NULL)
#define actor_job_publish_message_with_data(job, type, data, size) actor_job_publish_message_generic(job, type, NULL, data, size, NULL)
#define actor_job_publish_message_with_data_for(job, type, target, data, size)                                                               \
    actor_job_publish_message_generic(job, type, target, data, size, NULL)
#define actor_job_publish_message_with_argument(job, type, data, size, arg) actor_job_publish_message_generic(job, type, NULL, data, size, arg)
#define actor_job_publish_message_with_argument_for(job, type, target, data, size, arg)                                                      \
    actor_job_publish_message_generic(job, type, target, data, size, arg)

void actor_callback_message_handled(actor_t *actor, actor_message_t *event, actor_signal_t signal);

#ifdef __cplusplus
}
#endif

#endif