/**********************************************************************
 *
 * rttopo - topology library
 * Copyright 2002 Thamer Alharbash
 * Copyright 2009 Paul Ramsey <pramsey@cleverelephant.ca>
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

#ifndef _STRINGBUFFER_H
#define _STRINGBUFFER_H 1

#include "librtgeom_internal.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define STRINGBUFFER_STARTSIZE 128

typedef struct
{
	size_t capacity;
	char *str_end;
	char *str_start;
}
stringbuffer_t;

extern stringbuffer_t *stringbuffer_create_with_size(RTCTX *ctx, size_t size);
extern stringbuffer_t *stringbuffer_create(RTCTX *ctx);
extern void stringbuffer_destroy(RTCTX *ctx, stringbuffer_t *sb);
extern void stringbuffer_clear(RTCTX *ctx, stringbuffer_t *sb);
void stringbuffer_set(RTCTX *ctx, stringbuffer_t *sb, const char *s);
void stringbuffer_copy(RTCTX *ctx, stringbuffer_t *sb, stringbuffer_t *src);
extern void stringbuffer_append(RTCTX *ctx, stringbuffer_t *sb, const char *s);
extern int stringbuffer_aprintf(RTCTX *ctx, stringbuffer_t *sb, const char *fmt, ...);
extern const char *stringbuffer_getstring(RTCTX *ctx, stringbuffer_t *sb);
extern char *stringbuffer_getstringcopy(RTCTX *ctx, stringbuffer_t *sb);
extern int stringbuffer_getlength(RTCTX *ctx, stringbuffer_t *sb);
extern char stringbuffer_lastchar(RTCTX *ctx, stringbuffer_t *s);
extern int stringbuffer_trim_trailing_white(RTCTX *ctx, stringbuffer_t *s);
extern int stringbuffer_trim_trailing_zeroes(RTCTX *ctx, stringbuffer_t *s);

#endif /* _STRINGBUFFER_H */
