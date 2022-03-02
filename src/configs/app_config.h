#ifndef INC_ACTOR_CONFIG
#define INC_ACTOR_CONFIG

#define ACTOR_BUFFER_INITIAL_SIZE 32
#define ACTOR_BUFFER_MAX_SIZE 1024 * 16

#ifdef DEBUG
#define DEBUG_LOG_LEVEL 5
#else
#define DEBUG_LOG_LEVEL 0
#endif

#include "definitions/dictionary.h"
#include "definitions/enums.h"

#endif