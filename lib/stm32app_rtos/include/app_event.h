#ifndef INC_CORE_EVENT
#define INC_CORE_EVENT

#ifdef __cplusplus
extern "C" {
#endif

#include <app_types.h>
#include <app_env.h>

enum app_event_type {
    // Internal events that dont need subscription
    APP_EVENT_THREAD_START = 32, // Set up a schedule, prepare for work
    APP_EVENT_THREAD_STOP,       // Deallocate and destruct working objects
    APP_EVENT_THREAD_ALARM,      // Wake up by software timer alarm

    APP_EVENT_IDLE = 0,       // initial/empty state of an event
    APP_EVENT_READ,           // abstract peripherial read events for cases with known receiver
    APP_EVENT_READ_TO_BUFFER, // read without sending APP_EVENT_RESPONSE
    APP_EVENT_WRITE,
    APP_EVENT_TRANSFER,

    APP_EVENT_ERASE,
    APP_EVENT_MOUNT,
    APP_EVENT_UNMOUNT,
    APP_EVENT_RESPONSE,
    APP_EVENT_LOCK,
    APP_EVENT_UNLOCK,
    APP_EVENT_INTROSPECTION,
    APP_EVENT_DIAGNOSE,

    APP_EVENT_OPEN,
    APP_EVENT_QUERY,
    APP_EVENT_DELETE,
    APP_EVENT_CLOSE,
    APP_EVENT_SYNC,
    APP_EVENT_STATS,

    APP_EVENT_START,
    APP_EVENT_ENABLE,
    APP_EVENT_DISABLE,

};

/* Is event owned by some specific actor */
enum app_event_status {
    APP_EVENT_WAITING,   // Event is waiting to be routed
    APP_EVENT_RECEIVED,  // Some actors receieved the event
    APP_EVENT_ADDRESSED, // A actor that could handle event was busy, others still can claim it
    APP_EVENT_HANDLED,   // Device processed the event so no others will receive it
    APP_EVENT_DEFERRED,  // A busy actor wants this event exclusively
    APP_EVENT_FINALIZED
};

enum app_signal {
    APP_SIGNAL_OK = 0,
    APP_SIGNAL_PENDING = 1,

    APP_SIGNAL_TIMER,
    APP_SIGNAL_DMA_TRANSFERRING,
    APP_SIGNAL_DMA_IDLE,

    APP_SIGNAL_RX_COMPLETE,
    APP_SIGNAL_TX_COMPLETE,

    APP_SIGNAL_CATCHUP,
    APP_SIGNAL_RESCHEDULE,
    APP_SIGNAL_INCOMING,

    APP_SIGNAL_FAILURE = -1,
    APP_SIGNAL_TIMEOUT = -2,
    APP_SIGNAL_DMA_ERROR = -3,
    APP_SIGNAL_BUSY = -4,
    APP_SIGNAL_NOT_FOUND = -5,
    APP_SIGNAL_UNCONFIGURED = -6,
    APP_SIGNAL_INCOMPLETE = -7,
    APP_SIGNAL_CORRUPTED = -8,
    APP_SIGNAL_NO_RESPONSE = -9,
    APP_SIGNAL_OUT_OF_MEMORY = -10,
    APP_SIGNAL_INVALID_ARGUMENT = -11,
    APP_SIGNAL_NOT_IMPLEMENTED = -12
};


struct app_event {
    app_event_type_t type;     /* Kind of event*/
    app_event_status_t status; /* Status of events handling*/
    uint8_t *data;             /* Pointer to data package*/
    uint32_t size;             /* Size of data payload*/
    void *argument;            /* Optional argument */
    actor_t *producer;         /* Where event originated at */
    actor_t *consumer;         /* Device that handled the event*/
};

/* Attempt to store event in a memory destination if it's not occupied yet */
app_signal_t actor_event_accept_and_process_generic(actor_t *actor, app_event_t *event, app_event_t *destination,
                                                    app_event_status_t ready_status, app_event_status_t busy_status,
                                                    actor_on_event_report_t handler);

app_signal_t actor_event_accept_and_start_job_generic(actor_t *actor, app_event_t *event, app_job_t **job_slot, app_thread_t *thread,
                                                      actor_on_job_complete_t handler, app_event_status_t ready_status,
                                                      app_event_status_t busy_status);

app_signal_t actor_event_accept_and_pass_to_job_generic(actor_t *actor, app_event_t *event, app_job_t **job_slot, app_thread_t *thread,
                                                        actor_on_job_complete_t handler, app_event_status_t ready_status,
                                                        app_event_status_t busy_status);

/* Consume event if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle(actor, event, destination)                                                                                      \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_DEFERRED, NULL)
/* Consume event with a given handler  if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_process(actor, event, destination, handler)                                                                 \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_DEFERRED,                               \
                                           (actor_on_event_report_t)handler)
/* Consume event by starting a task if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_start_job(actor, event, task, thread, handler)                                                              \
    actor_event_accept_and_start_job_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)
/* Consume event with a running task if not busy, otherwise keep it enqueued for later without allowing others to take it  */
#define actor_event_handle_and_pass_to_job(actor, event, task, thread, handler)                                                            \
    actor_event_accept_and_pass_to_job_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_DEFERRED)

/* Consume event if not busy, otherwise keep it enqueued for later unless other actors take it first  */
#define actor_event_accept(actor, event, destination)                                                                                      \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED, NULL)
/* Consume event with a given handler if not busy, otherwise keep it enqueued for later unless other actors take it first */
#define actor_event_accept_and_process(actor, event, destination, handler)                                                                 \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED, (app_event_t)handler)
/* Consume event by starting a task if not busy, otherwise keep it enqueued for later unless other actors take it first  */
#define actor_event_accept_and_start_job(actor, event, task, thread, handler)                                                              \
    actor_event_accept_and_start_job_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED)
/* Consume event with a running task  if not busy, otherwise keep it enqueued for later unless other actors take it first */
#define actor_event_accept_and_pass_to_job(actor, event, task, thread, handler)                                                            \
    actor_event_accept_and_pass_to_job_generic(actor, event, task, thread, handler, APP_EVENT_HANDLED, APP_EVENT_ADDRESSED)
/* Process event and let others receieve it too */
#define actor_event_receive(actor, event, destination)                                                                                     \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_RECEIVED, APP_EVENT_RECEIVED, NULL)
/* Process event with a given handler and let others receieve it too */
#define actor_event_receive_and_process(actor, event, destination, handler)                                                                \
    actor_event_accept_and_process_generic(actor, event, destination, APP_EVENT_RECEIVED, APP_EVENT_RECEIVED, (app_event_t)handler)
/* Consume even by starting a task if not busy, and let others receieve it too  */
#define actor_event_receive_and_start_job(actor, event, task, thread, handler)                                                             \
    actor_event_accept_and_start_job_generic(actor, event, task, thread, handler, APP_EVENT_RECEIVED, APP_EVENT_RECEIVED)
/* Consume event with a running task  if not busy, and let others receieve it too  */
#define actor_event_receive_and_pass_to_job(actor, event, task, thread, handler)                                                           \
    actor_event_accept_and_pass_to_job_generic(actor, event, task, thread, handler, APP_EVENT_RECEIVED, APP_EVENT_RECEIVED)

/* Pass signal to job, schedule it to run in the given thread */
app_signal_t actor_signal_pass_to_job(actor_t *actor, app_signal_t signal, app_job_t **job_slot, app_thread_t *thread);
/* Pass signal to job and run it right away */
app_signal_t actor_signal_job(actor_t *actor, app_signal_t signal, app_job_t **job_slot);

app_signal_t actor_worker_catchup(actor_t *actor, actor_worker_t *tick);




app_signal_t actor_event_report(actor_t *actor, app_event_t *event);
app_signal_t actor_event_finalize(actor_t *actor, app_event_t *event);

app_signal_t actor_publish_event_generic(actor_t *actor, app_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument);
#define actor_publish_event(actor, type) actor_publish_event_generic(actor, type, NULL, NULL, 0, NULL)
#define actor_publish_event_for(actor, type, target) actor_publish_event_generic(actor, type, target, NULL, 0, NULL)
#define actor_publish_event_with_data(actor, type, data, size) actor_publish_event_generic(actor, type, NULL, data, size, NULL)
#define actor_publish_event_with_data_for(actor, type, target, data, size)                                                                 \
    actor_publish_event_generic(actor, type, target, data, size, NULL)
#define actor_publish_event_with_argument(actor, type, data, size, arg) actor_publish_event_generic(actor, type, NULL, data, size, arg)
#define actor_publish_event_with_argument_for(actor, type, target, data, size, arg)                                                        \
    actor_publish_event_generic(actor, type, target, data, size, arg)


app_signal_t app_job_publish_event_generic(app_job_t *job, app_event_type_t type, actor_t *target, uint8_t *data, uint32_t size,
                                         void *argument);

#define app_job_publish_event(job, type) app_job_publish_event_generic(job, type, NULL, NULL, 0, NULL)
#define app_job_publish_event_for(job, type, target) app_job_publish_event_generic(job, type, target, NULL, 0, NULL)
#define app_job_publish_event_with_data(job, type, data, size) app_job_publish_event_generic(job, type, NULL, data, size, NULL)
#define app_job_publish_event_with_data_for(job, type, target, data, size)                                                                 \
    app_job_publish_event_generic(job, type, target, data, size, NULL)
#define app_job_publish_event_with_argument(job, type, data, size, arg) app_job_publish_event_generic(job, type, NULL, data, size, arg)
#define app_job_publish_event_with_argument_for(job, type, target, data, size, arg)                                                        \
    app_job_publish_event_generic(job, type, target, data, size, arg)


void app_event_report_callback(actor_t *actor, app_event_t *event);

#ifdef __cplusplus
}
#endif

#endif