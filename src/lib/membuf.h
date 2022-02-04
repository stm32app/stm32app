// https://github.com/liigo/membuf
// by Liigo.

#include "lib/heap.h"

#ifndef ZZ_MEMBUF_H
#define ZZ_MEMBUF_H

// `membuf_t` is a growable continuous in-memory buffer.
// It also support "local buffer" to use stack memory efficiently.
// https://github.com/liigo/membuf
// by Liigo, 2013-7-5, 2014-8-16, 2014-10-21, 2014-11-18, 2015-3-7, 2015-7-21.

// Forked to support growing by fixed block size instead of doubling

#ifdef __cplusplus
extern "C"	{
#endif

#include <stdint.h>
#include <stdlib.h>

typedef struct {
	unsigned char* data;
	uint32_t   size;
	uint32_t   buffer_size;
	uint32_t   block_size;
	unsigned char  uses_local_buffer;  // local buffer, e.g. on stack
} membuf_t;

#ifndef MEMBUF_INIT_LOCAL
	#define MEMBUF_INIT_LOCAL(buf,n) membuf_t buf; unsigned char buf##n[n]; membuf_init_local(&buf, &buf##n, n);
#endif

void membuf_init(membuf_t* buf, uint32_t initial_buffer_size);
void membuf_init_local(membuf_t* buf, void* local_buffer, uint32_t local_buffer_size);
void membuf_init_move_from(membuf_t* buf, membuf_t* other); // don't use other anymore
void membuf_uninit(membuf_t* buf);

// returns the offset of the new added data
uint32_t membuf_append(membuf_t* buf, void* data, uint32_t size);
uint32_t membuf_append_zeros(membuf_t* buf, uint32_t size);

static void* membuf_get_data(membuf_t* buf) { return (buf->size == 0 ? NULL : buf->data); }
static uint32_t membuf_get_size(membuf_t* buf) { return buf->size; }
static uint32_t membuf_is_empty(membuf_t* buf) { return buf->size == 0; }
static void membuf_empty(membuf_t* buf) { buf->size = 0; }

void membuf_insert(membuf_t* buf, uint32_t offset, void* data, uint32_t size);
void membuf_remove(membuf_t* buf, uint32_t offset, uint32_t size);
void membuf_reserve(membuf_t* buf, uint32_t extra_size);
void* membuf_release(membuf_t* buf, uint32_t* psize); // need free() result if not NULL

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ZZ_MEMBUF_H