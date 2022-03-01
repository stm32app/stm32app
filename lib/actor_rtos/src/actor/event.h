#ifndef INC_CORE_EVENT
#define INC_CORE_EVENT

#ifdef __cplusplus
extern "C" {
#endif

#include <actor/types.h>
#include <actor/env.h>

enum actor_event_type {
    // Internal events that dont need subscription
    ACTOR_EVENT_THREAD_START = 32, // Set up a schedule, prepare for work
    ACTOR_EVENT_THREAD_STOP,       // Deallocate and destruct working objects
    ACTOR_EVENT_THREAD_ALARM,      // Wake up by software timer alarm

    ACTOR_EVENT_IDLE = 0,       // initial/empty state of an event
    ACTOR_EVENT_READ,           // abstract peripherial read events for cases with known receiver
    ACTOR_EVENT_READ_TO_BUFFER, // read without sending ACTOR_EVENT_RESPONSE
    ACTOR_EVENT_WRITE,
    ACTOR_EVENT_TRANSFER,

    ACTOR_EVENT_ERASE,
    ACTOR_EVENT_MOUNT,
    ACTOR_EVENT_UNMOUNT,
    ACTOR_EVENT_RESPONSE,
    ACTOR_EVENT_LOCK,
    ACTOR_EVENT_UNLOCK,
    ACTOR_EVENT_INTROSPECTION,
    ACTOR_EVENT_DIAGNOSE,

    ACTOR_EVENT_OPEN,
    ACTOR_EVENT_QUERY,
    ACTOR_EVENT_DELETE,
    ACTOR_EVENT_CLOSE,
    ACTOR_EVENT_SYNC,
    ACTOR_EVENT_STATS,

    ACTOR_EVENT_START,
    ACTOR_EVENT_ENABLE,
    ACTOR_EVENT_DISABLE,

};

/* Is event owned by some specific actor */
enum actor_event_status {
    ACTOR_EVENT_WAITING,   // Event is waiting to be routed
    ACTOR_EVENT_RECEIVED,  // Some actors receieved the event
    ACTOR_EVENT_ADDRESSED, // A actor that could handle event was busy, others still can claim it
    ACTOR_EVENT_HANDLED,   // Device processed the event so no others will receive it
    ACTOR_EVENT_DEFERRED,  // A busy actor wants this event exclusively
    ACTOR_EVENT_FINALIZED
};

enum actor_signal {
    ACTOR_SIGNAL_OK = 0,
    ACTOR_SIGNAL_PENDING = 1,

    ACTOR_SIGNAL_TIMER,
    ACTOR_SIGNAL_DMA_TRANSFERRING,
    ACTOR_SIGNAL_DMA_IDLE,

    ACTOR_SIGNAL_RX_COMPLETE,
    ACTOR_SIGNAL_TX_COMPLETE,

    ACTOR_SIGNAL_CATCHUP,
    ACTOR_SIGNAL_RESCHEDULE,
    ACTOR_SIGNAL_INCOMING,

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
    ACTOR_SIGNAL_NOT_IMPLEMENTED = -12
};


struct actor_event {
    actor_event_type_t type;     /* Kind of event*/
    actor_event_status_t status; /* Status of events handling*/
    uint8_t *data;             /* Pointer to data package*/
    uint32_t size;             /* Size of data payload*/
    void *argument;            /* Optional argument */
    actor_t *producer;         /* Where event originated at */
    actor_t *consumer;         /* Device that handled the event*/
};

/* Attempt to store event in a memory destination if it's not occupied yet */
actor_signal_t actor_event_accept_and_process_generic(actor_t *actor, actor_event_t *event, actor_event_t *destination,
                                                    actor_event_status_t ready_status, actor_event_status_t busy_status,
                                                    actor_on_event_report_t handler);

actor_signal_t actor_event_accept_and_start_job_generic(actor_t *actor, actor_event_t *event, actor_job_t **job_slot, actor_thread_t *thread,
                                                      actor_on_job_complete_t handler, actor_event_status_t ready_status,
                                                      actor_event_status_t busy_status);

actor_signal_t actor_event_accept_and_pass_to_job_generic(actor_t *actor, actor_event_t *event, actor_job_t **job_slot, actor_thread_t *thread,
                                                        actor_on_job_complete_t handler, actor_event_status_t ready_status,
                                                        actor_event_status_t busy_status);

/* Consume event if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle(actor, event, destination)                                                                                      \
    actor_event_accept_and_process_generic(actor, event, destination, ACTOR_EVENT_HANDLED, ACTOR_EVENT_DEFERRED, NULL)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_process(actor, event, destination, handler)                                                                 \
    actor_event_accept_and_process_generic(actor, event, destination, ACTOR_EVENT_HANDLED, ACTOR_EVENT_DEFERRED,                               \
                                           (actor_on_event_report_t)handler)
/* Consume event by starting a task if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_start_job(actor, event, task, thread, handler)                                                              \
    actor_event_accept_and_start_job_generic(actor, event, task, thread, handler, ACTOR_EVENT_HANDLED, ACTOR_EVENT_DEFERRED)
/* Consume event with a running task if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_pass_to_job(actor, event, task, thread, handler)                                                            \
    actor_event_accept_and_pass_to_job_generic(actor, event, task, thread, handler, ACTOR_EVENT_HANDLED, ACTOR_EVENT_DEFERRED)

/* Consume event if not busy, otherwise keep it enqueued for later unless other actors take it first  */
#define actor_event_accept(actor, event, destination)                                                                                      \
    actor_event_accept_and_process_generic(actor, event, destination, ACTOR_EVENT_HANDLED, ACTOR_EVENT_ADDRESSED, NULL)
/* Consume event with a given handler if not busy, otherwise keep it enqueued for later unless other actors take it first */
#define actor_event_accept_and_process(actor, event, destination, handler)                                                                 \
    actor_event_accept_and_process_generic(actor, event, destination, ACTOR_EVENT_HANDLED, ACTOR_EVENT_ADDRESSED, (actor_event_t)handler)
/* Consume event by starting a task if not busy, otherwise keep it enqueued for later unless other actors take it first  */
#define actor_event_accept_and_start_job(actor, event, task, thread, handler)                                                              \
    actor_event_accept_and_start_job_generic(actor, event, task, thread, handler, ACTOR_EVENT_HANDLED, ACTOR_EVENT_ADDRESSED)
/* Consume event with a running task  if not busy, otherwise keep it enqueued for later unless other actors take it first */
#define actor_event_accept_and_pass_to_job(actor, event, task, thread, handler)                                                            \
    actor_event_accept_and_pass_to_job_generic(actor, event, task, thread, handler, ACTOR_EVENT_HANDLED, ACTOR_EVENT_ADDRESSED)
/* Process event and let others receieve it too */
#define actor_event_receive(actor, event, destination)                                                                                     \
    actor_event_accept_and_process_generic(actor, event, destination, ACTOR_EVENT_RECEIVED, ACTOR_EVENT_RECEIVED, NULL)
/* Process event with a given handler and let others receieve it too */
#define actor_event_receive_and_process(actor, event, destination, handler)                                                                \
    actor_event_accept_and_process_generic(actor, event, destination, ACTOR_EVENT_RECEIVED, ACTOR_EVENT_RECEIVED, (actor_event_t)handler)
/* Consume even by starting a task if not busy, and let others receieve it too  */
#define actor_event_receive_and_start_job(actor, event, task, thread, handler)                                                             \
    actor_event_accept_and_start_job_generic(actor, event, task, thread, handler, ACTOR_EVENT_RECEIVED, ACTOR_EVENT_RECEIVED)
/* Consume event with a running task  if not busy, and let others receieve it too  */
#define actor_event_receive_and_pass_to_job(actor, event, task, thread, handler)                                                           \
    actor_event_accept_and_pass_to_job_generic(actor, event, task, thread, handler, ACTOR_EVENT_RECEIVED, ACTOR_EVENT_RECEIVED)

/* Pass signal to job, schedule it to run in the given thread */
actor_signal_t actor_signal_pass_to_job(actor_t *actor, actor_signal_t signal, actor_job_t **job_slot, actor_thread_t *thread);
/* Pass signal to job and run it right away */
actor_signal_t actor_signal_job(actor_t *actor, actor_signal_t signal, actor_job_t **job_slot);

actor_signal_t actor_worker_catchup(actor_t *actor, actor_worker_t *tick);




actor_signal_t actor_event_report(actor_t *actor, actor_event_t *event);
actor_signal_t actor_event_finalize(actor_t *actor, actor_event_t *event);

actor_signal_t actor_publish_event_generic(actor_t *actor, actor_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument);
#define actor_publish_event(actor, type) actor_publish_event_generic(actor, type, NULL, NULL, 0, NULL)
#define actor_publish_event_for(actor, type, target) actor_publish_event_generic(actor, type, target, NULL, 0, NULL)
#define actor_publish_event_with_data(actor, type, data, size) actor_publish_event_generic(actor, type, NULL, data, size, NULL)
#define actor_publish_event_with_data_for(actor, type, target, data, size)                                                                 \
    actor_publish_event_generic(actor, type, target, data, size, NULL)
#define actor_publish_event_with_argument(actor, type, data, size, arg) actor_publish_event_generic(actor, type, NULL, data, size, arg)
#define actor_publish_event_with_argument_for(actor, type, target, data, size, arg)                                                        \
    actor_publish_event_generic(actor, type, target, data, size, arg)


actor_signal_t actor_job_publish_event_generic(actor_job_t *job, actor_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument);

#define actor_job_publish_event(job, type) actor_job_publish_event_generic(job, type, NULL, NULL, 0, NULL)
#define actor_job_publish_event_for(job, type, target) actor_job_publish_event_generic(job, type, target, NULL, 0, NULL)
#define actor_job_publish_event_with_data(job, type, data, size) actor_job_publish_event_generic(job, type, NULL, data, size, NULL)
#define actor_job_publish_event_with_data_for(job, type, target, data, size)                                                                 \
    actor_job_publish_event_generic(job, type, target, data, size, NULL)
#define actor_job_publish_event_with_argument(job, type, data, size, arg) actor_job_publish_event_generic(job, type, NULL, data, size, arg)
#define actor_job_publish_event_with_argument_for(job, type, target, data, size, arg)                                                        \
    actor_job_publish_event_generic(job, type, target, data, size, arg)


void actor_event_report_callback(actor_t *actor, actor_event_t *event);

#ifdef __cplusplus
}
#endif

#endif