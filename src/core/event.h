#ifndef INC_CORE_EVENT
#define INC_CORE_EVENT

#ifdef __cplusplus
extern "C" {
#endif

#include "core/types.h"

enum app_event_type {
    // Internal events that dont need subscription
    APP_EVENT_THREAD_START = 32, // Set up a schedule, prepare for work
    APP_EVENT_THREAD_STOP,       // Deallocate and destruct working objects
    APP_EVENT_THREAD_ALARM,      // Wake up by software timer alarm

    APP_EVENT_IDLE = 0,      // initial/empty state of an event
    APP_EVENT_READ,          // abstract peripherial read events for cases with known receiver
    APP_EVENT_READ_TO_BUFFER, // read without sending APP_EVENT_RESPONSE
    APP_EVENT_WRITE,
    APP_EVENT_TRANSFER,

    APP_EVENT_ERASE,
    APP_EVENT_RESPONSE,
    APP_EVENT_LOCK,
    APP_EVENT_UNLOCK,
    APP_EVENT_INTROSPECTION,
    APP_EVENT_DIAGNOSE,

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

struct app_event {
    app_event_type_t type;     /* Kind of event*/
    app_event_status_t status; /* Status of events handling*/
    uint8_t *data;             /* Pointer to data package*/
    uint32_t size;             /* Size of data payload*/
    void *argument;            /* Optional argument */
    actor_t *producer;         /* Where event originated at */
    actor_t *consumer;         /* Device that handled the event*/
};

void app_event_set_status(app_event_t *event, app_event_status_t status);

#ifdef __cplusplus
}
#endif

#endif