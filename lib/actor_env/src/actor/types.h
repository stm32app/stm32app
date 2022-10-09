#ifndef INC_TYPES
#define INC_TYPES

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef BOOL_T_DEFINED
#define BOOL_T_DEFINED
typedef bool bool_t;
#endif

#if UINTPTR_MAX > (1ULL << 32)
typedef double floatptr_t;
#define ENV64BIT
typedef char strptr_t[8];
#else
typedef float floatptr_t;
typedef char strptr_t[4];
#define ENV32BIT
#endif

#ifdef ENV64BIT
#else
#endif

#ifndef ACTOR_NODE_STRUCT
#define ACTOR_NODE_STRUCT actor_node
#endif

typedef struct node actor_generic_node_t;      /* Generic application object, acts as a global root */
typedef struct ACTOR_NODE_STRUCT actor_node_t; /* Generic application object, acts as a global root */
typedef struct actor actor_t;                  /* Generic object container correspondig to Object Dictionary entry */

typedef struct actor_thread actor_thread_t;               /* A FreeRTOS thread that handles multiple actors*/
typedef struct actor_message actor_message_t;             /* Data sent between actors placed into a bus */
typedef enum actor_message_status actor_message_status_t; /* Status of event interaction with actor */
typedef enum actor_message_type actor_message_type_t;     /* Possible types of messages */
typedef struct actor_worker actor_worker_t;               /* A actor callback running within specific node thread*/
typedef struct actor_interface actor_interface_t;         /* Method list to essentially subclass actors */
typedef enum actor_type actor_type_t;                     /* List of actor groups found in Object Dictionary*/
typedef enum actor_phase actor_phase_t;                   /* All phases that actor can be in*/
typedef enum actor_signal actor_signal_t;                 /* Things that actor tell each other */
typedef struct actor_job actor_job_t;                     /* State machine dealing with high_priority commands*/
typedef enum actor_signal actor_signal_t;                 /* Commands to advance step machine*/
typedef struct actor_mutex actor_mutex_t;                 /* Enable exclusive control to shared resources  */
typedef struct actor_blank actor_blank_t;                 /* Example blank actor */
typedef struct actor_buffer actor_buffer_t;               /* Growable owned buffer */
typedef struct actor_double_buffer actor_double_buffer_t; /* Typical read operation for peripheral*/
typedef actor_signal_t (*actor_job_handler_t)(actor_job_t *job, actor_signal_t signal, actor_t *caller);
typedef actor_signal_t (*actor_on_job_complete_t)(actor_job_t *job);
typedef actor_signal_t (*actor_on_message_t)(void *object, actor_message_t *event);
typedef actor_signal_t (*actor_message_handled_t)(void *object, actor_message_t *event, actor_signal_t signal);
typedef actor_signal_t (*actor_worker_callback_t)(void *object, actor_message_t *event, actor_worker_t *worker, actor_thread_t *thread);
typedef actor_signal_t (*actor_phase_changed_t)(void *object, actor_phase_t phase);
typedef actor_signal_t (*actor_value_received_t)(void *object, actor_t *actor, void *value, void *argument);
typedef actor_signal_t (*actor_on_link_t)(void *object, actor_t *actor, void *argument);
typedef actor_signal_t (*actor_signal_received_t)(void *object, actor_signal_t signal, actor_t *caller, void *argument);
typedef actor_signal_t (*actor_buffer_allocation_t)(void *object, actor_buffer_t *buffer, uint32_t size);
typedef actor_signal_t (*actor_property_changed_t)(void *object, uint8_t index, void *value, void *old);
typedef actor_worker_callback_t (*actor_worker_assignment_t)(void *object, actor_thread_t *thread);

typedef actor_signal_t (*actor_method_t)(void *object);
typedef void (*actor_procedure_t)(void *object);

typedef struct actor_generic_device_t actor_generic_device_t; // unknown polymorphic object type

/*
 union {
    uintptr_t _uint;
    intptr_t _int;
    floatptr_t _float;
    strptr_t _str;
    void *_pointer;
} actor_argument_t;*/

#endif