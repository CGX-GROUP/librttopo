/**********************************************************************
 * $Id: bytebuffer.h 12198 2014-01-29 17:49:35Z pramsey $
 *
 * rttopo - topology library
 * Copyright 2015 Nicklas Avén <nicklas.aven@jordogskog.no>
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * The name of the author may not be used to endorse or promote
 * products derived from this software without specific prior
 * written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 **********************************************************************/

#ifndef _BYTEBUFFER_H
#define _BYTEBUFFER_H 1

#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include "varint.h"

#include "rtgeom_log.h"
#define BYTEBUFFER_STARTSIZE 128

typedef struct
{
	size_t capacity;
	uint8_t *buf_start;
	uint8_t *writecursor;	
	uint8_t *readcursor;	
}
bytebuffer_t;

void bytebuffer_init_with_size(RTCTX *ctx, bytebuffer_t *b, size_t size);
bytebuffer_t *bytebuffer_create_with_size(RTCTX *ctx, size_t size);
bytebuffer_t *bytebuffer_create(RTCTX *ctx);
void bytebuffer_destroy(RTCTX *ctx, bytebuffer_t *s);
void bytebuffer_clear(RTCTX *ctx, bytebuffer_t *s);
void bytebuffer_append_byte(RTCTX *ctx, bytebuffer_t *s, const uint8_t val);
void bytebuffer_append_varint(RTCTX *ctx, bytebuffer_t *s, const int64_t val);
void bytebuffer_append_uvarint(RTCTX *ctx, bytebuffer_t *s, const uint64_t val);
uint64_t bytebuffer_read_uvarint(RTCTX *ctx, bytebuffer_t *s);
int64_t bytebuffer_read_varint(RTCTX *ctx, bytebuffer_t *s);
size_t bytebuffer_getlength(RTCTX *ctx, bytebuffer_t *s);
bytebuffer_t* bytebuffer_merge(RTCTX *ctx, bytebuffer_t **buff_array, int nbuffers);
void bytebuffer_reset_reading(RTCTX *ctx, bytebuffer_t *s);

void bytebuffer_append_bytebuffer(RTCTX *ctx, bytebuffer_t *write_to,bytebuffer_t *write_from);
void bytebuffer_append_bulk(RTCTX *ctx, bytebuffer_t *s, void * start, size_t size);
void bytebuffer_append_int(RTCTX *ctx, bytebuffer_t *buf, const int val, int swap);
void bytebuffer_append_double(RTCTX *ctx, bytebuffer_t *buf, const double val, int swap);
#endif /* _BYTEBUFFER_H */
