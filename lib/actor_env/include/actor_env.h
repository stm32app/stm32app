#ifndef INC_ENV
#define INC_ENV

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ACTOR_CONFIG_PATH(x) ACTOR_CONFIG_PATH2(x)
#define ACTOR_CONFIG_PATH2(x) #x

#ifdef ACTOR_CONFIG
#include ACTOR_CONFIG_PATH(ACTOR_CONFIG)
#endif

#include <actor_stubs.h>
#include <actor_types.h>
#include <actor_errors.h>
#include <multi_heap.h>

#endif