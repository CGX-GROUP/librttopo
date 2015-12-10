/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright 2011 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* Workaround for GEOS 2.2 compatibility: old geos_c.h does not contain
   header guards to protect from multiple inclusion */
#ifndef GEOS_C_INCLUDED
#define GEOS_C_INCLUDED
#include "geos_c.h"
#endif

#include "librtgeom.h"


/*
** Public prototypes for GEOS utility functions.
*/
RTGEOM *GEOS2RTGEOM(const RTCTX *ctx, const GEOSGeometry *geom, char want3d);
GEOSGeometry * RTGEOM2GEOS(const RTCTX *ctx, const RTGEOM *g, int autofix);
GEOSGeometry * GBOX2GEOS(const RTCTX *ctx, const RTGBOX *g);
GEOSGeometry * RTGEOM_GEOS_buildArea(const RTCTX *ctx, const GEOSGeometry* geom_in);

RTPOINTARRAY *ptarray_from_GEOSCoordSeq(const RTCTX *ctx, const GEOSCoordSequence *cs, char want3d);


extern char rtgeom_geos_errmsg[];
extern void rtgeom_geos_error(const char *fmt, ...);

extern void rtgeom_geos_ensure_init(const RTCTX *ctx);

