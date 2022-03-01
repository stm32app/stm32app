/*
 * coru utilities and config
 *
 * Copyright (c) 2019 Christopher Haster
 * Distributed under the MIT license
 *
 * Can be overridden by users with their own configuration by defining
 * CORU_CONFIG as a header file (-DCORU_CONFIG=coru_config.h)
 *
 * If CORU_CONFIG is defined, none of the default definitions will be emitted
 * and must be provided by the user's config file. To start, I would suggest
 * copying coru_util.h and modifying as needed.
 */
#ifndef CORU_CONFIG_H
#define CORU_CONFIG_H

#include <actor/env.h>
#include "lib/debug.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CORU_ASSERT actor_assert
#define coru_malloc actor_malloc
#define coru_free actor_free
#define coru_malloc actor_malloc



#ifdef __cplusplus
}
#endif

#endif
