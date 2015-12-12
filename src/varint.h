/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2014 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2013 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 * 
 * Handle varInt values, as described here:
 * http://developers.google.com/protocol-buffers/docs/encoding#varints
 *
 **********************************************************************/

#ifndef _LIBRTGEOM_VARINT_H
#define _LIBRTGEOM_VARINT_H 1

#include "librtgeom_internal.h"

#include <stdint.h>
#include <stdlib.h>


/* NEW SIGNATURES */

size_t varint_u32_encode_buf(const RTCTX *ctx, uint32_t val, uint8_t *buf);
size_t varint_s32_encode_buf(const RTCTX *ctx, int32_t val, uint8_t *buf);
size_t varint_u64_encode_buf(const RTCTX *ctx, uint64_t val, uint8_t *buf);
size_t varint_s64_encode_buf(const RTCTX *ctx, int64_t val, uint8_t *buf);
int64_t varint_s64_decode(const RTCTX *ctx, const uint8_t *the_start, const uint8_t *the_end, size_t *size);
uint64_t varint_u64_decode(const RTCTX *ctx, const uint8_t *the_start, const uint8_t *the_end, size_t *size);

size_t varint_size(const RTCTX *ctx, const uint8_t *the_start, const uint8_t *the_end);

uint64_t zigzag64(const RTCTX *ctx, int64_t val);
uint32_t zigzag32(const RTCTX *ctx, int32_t val);
uint8_t zigzag8(const RTCTX *ctx, int8_t val);
int64_t unzigzag64(const RTCTX *ctx, uint64_t val);
int32_t unzigzag32(const RTCTX *ctx, uint32_t val);
int8_t unzigzag8(const RTCTX *ctx, uint8_t val);

#endif /* !defined _LIBRTGEOM_VARINT_H  */

