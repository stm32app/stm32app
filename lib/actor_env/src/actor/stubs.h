#ifndef INC_ENV_MACROS
#define INC_ENV_MACROS

#include <actor/types.h>

void actor_disable_interrupts(void);
void actor_enable_interrupts(void);

void actor_tasks_suspend(void);
void actor_tasks_resume(void);

void actor_node_error_report(actor_node_t* node, const uint8_t errorBit, uint16_t errorCode, uint32_t index);
void actor_node_error_reset(actor_node_t* node, const uint8_t errorBit, uint16_t errorCode, uint32_t index);

#define actor_error_report(actor, errorBit, errorCode) actor_node_error_report(actor->node, errorBit, errorCode, actor->index)
#define actor_error_reset(actor, errorBit, errorCode) actor_node_error_report(actor->node, errorBit, errorCode, actor->index)

char *actor_event_stringify(actor_event_t *event);
char *actor_signal_stringify(int signal);
char *actor_phase_stringify(int phase);
char *actor_stringify(actor_t *actor);
void *actor_unbox(actor_t *actor);
actor_t *actor_box(void *object);

#endif