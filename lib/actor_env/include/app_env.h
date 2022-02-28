#ifndef INC_ENV
#define INC_ENV

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define APP_CONFIG_PATH(x) APP_CONFIG_PATH2(x)
#define APP_CONFIG_PATH2(x) #x

#ifdef APP_CONFIG
#include APP_CONFIG_PATH(APP_CONFIG)
#endif

#include <app_stubs.h>

#endif