/**********************************************************************
 * 
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 * Copyright 2008 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright 2007-2008 Mark Cave-Ayland
 * Copyright 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * Internal logging routines
 *
 **********************************************************************/

#ifndef RTGEOM_LOG_H
#define RTGEOM_LOG_H 1

#include "librtgeom_internal.h"
#include <stdarg.h>

/*
 * Debug macros
 */
#if RTGEOM_DEBUG_LEVEL > 0

/* Display a notice at the given debug level */
#define RTDEBUG(level, msg) \
        do { \
            if (RTGEOM_DEBUG_LEVEL >= level) \
              rtdebug(RTCTX *ctx, level, "[%s:%s:%d] " msg, __FILE__, __func__, __LINE__); \
        } while (0);

/* Display a formatted notice at the given debug level
 * (like printf, with variadic arguments) */
#define RTDEBUGF(level, msg, ...) \
        do { \
            if (RTGEOM_DEBUG_LEVEL >= level) \
              rtdebug(RTCTX *ctx, level, "[%s:%s:%d] " msg, \
                __FILE__, __func__, __LINE__, __VA_ARGS__); \
        } while (0);

#else /* RTGEOM_DEBUG_LEVEL <= 0 */

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define RTDEBUG(level, msg) \
        ((void) 0)

/* Empty prototype that can be optimised away by the compiler
 * for non-debug builds */
#define RTDEBUGF(level, msg, ...) \
        ((void) 0)

#endif /* RTGEOM_DEBUG_LEVEL <= 0 */

/**
 * Write a notice out to the notice handler.
 *
 * Uses standard printf() substitutions.
 * Use for messages you artays want output.
 * For debugging, use RTDEBUG() or RTDEBUGF().
 * @ingroup logging
 */
void rtnotice(RTCTX *ctx, const char *fmt, ...);

/**
 * Write a notice out to the error handler.
 *
 * Uses standard printf() substitutions.
 * Use for errors you artays want output.
 * For debugging, use RTDEBUG() or RTDEBUGF().
 * @ingroup logging
 */
void rterror(const RTCTX *ctx, const char *fmt, ...);

/**
 * Write a debug message out. 
 * Don't call this function directly, use the 
 * macros, RTDEBUG() or RTDEBUGF(), for
 * efficiency.
 * @ingroup logging
 */
void rtdebug(RTCTX *ctx, int level, const char *fmt, ...);



#endif /* RTGEOM_LOG_H */
