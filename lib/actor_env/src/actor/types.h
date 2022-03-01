#ifndef INC_TYPES
#define INC_TYPES

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// Check GCC
#if __GNUC__
  #if __x86_64__ || __ppc64__
    #define ENV64BIT
  #else
    #define ENV32BIT
  #endif
#endif

#ifdef ENV64BIT
  #define uintXX_t uint64_t
#else
  #define uintXX_t uint32_t
#endif

#ifndef APP_NODE_STRUCT_NAME
    #define APP_NODE_STRUCT_NAME actor_node
#endif



typedef struct node actor_generic_node_t;                             /* Generic application object, acts as a global root */
typedef struct APP_NODE_STRUCT_NAME actor_node_t;                             /* Generic application object, acts as a global root */
typedef struct actor actor_t;                         /* Generic object container correspondig to Object Dictionary entry */

typedef struct actor_thread actor_thread_t;               /* A FreeRTOS thread that handles multiple actors*/
typedef struct actor_event actor_event_t;                 /* Data sent between actors placed into a bus */
typedef enum actor_event_status actor_event_status_t;     /* Status of event interaction with actor */
typedef enum actor_event_type actor_event_type_t;         /* Possible types of messages */
typedef struct actor_worker actor_worker_t;           /* A actor callback running within specific node thread*/
typedef struct actor_class actor_class_t;             /* Method list to essentially subclass actors */
typedef enum actor_type actor_type_t;                 /* List of actor groups found in Object Dictionary*/
typedef enum actor_phase actor_phase_t;               /* All phases that actor can be in*/
typedef enum actor_signal actor_signal_t;                 /* Things that actor tell each other */
typedef struct actor_job actor_job_t;                     /* State machine dealing with high_priority commands*/
typedef enum actor_job_signal actor_job_signal_t;         /* Commands to advance step machine*/
typedef struct actor_blank actor_blank_t;             /* Example blank actor */
typedef struct actor_buffer actor_buffer_t;               /* Growable owned buffer */
typedef struct actor_double_buffer actor_double_buffer_t; /* Typical read operation for peripheral*/
typedef actor_job_signal_t (*actor_on_job_complete_t)(actor_job_t *job);
typedef actor_signal_t (*actor_on_event_report_t)(void *object, actor_event_t *event);
typedef actor_signal_t (*actor_worker_callback_t)(void *object, actor_event_t *event, actor_worker_t *worker, actor_thread_t *thread);
typedef actor_signal_t (*actor_on_phase_t)(void *object, actor_phase_t phase);
typedef actor_signal_t (*actor_on_value_t)(void *object, actor_t *actor, void *value, void *argument);
typedef actor_signal_t (*actor_on_link_t)(void *object, actor_t *actor, void *argument);
typedef actor_signal_t (*actor_on_signal_t)(void *object, actor_t *actor, actor_signal_t signal, void *argument);
typedef actor_signal_t (*actor_on_buffer_allocation_t)(void *object, actor_buffer_t *buffer, uint32_t size);
typedef actor_worker_callback_t (*actor_on_worker_assignment_t)(void *object, actor_thread_t *thread);

typedef actor_signal_t (*actor_method_t)(void *object);
typedef void (*actor_procedure_t)(void *object);

typedef struct actor_generic_device_t actor_generic_device_t; // unknown polymorphic object type

#endif