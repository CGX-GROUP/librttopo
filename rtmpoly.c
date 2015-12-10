/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtgeom_internal.h"
#include "rtgeom_log.h"


void
rtmpoly_release(const RTCTX *ctx, RTMPOLY *rtmpoly)
{
	rtgeom_release(ctx, rtmpoly_as_rtgeom(ctx, rtmpoly));
}

RTMPOLY *
rtmpoly_construct_empty(const RTCTX *ctx, int srid, char hasz, char hasm)
{
	RTMPOLY *ret = (RTMPOLY*)rtcollection_construct_empty(ctx, RTMULTIPOLYGONTYPE, srid, hasz, hasm);
	return ret;
}


RTMPOLY* rtmpoly_add_rtpoly(const RTCTX *ctx, RTMPOLY *mobj, const RTPOLY *obj)
{
	return (RTMPOLY*)rtcollection_add_rtgeom(ctx, (RTCOLLECTION*)mobj, (RTGEOM*)obj);
}


void rtmpoly_free(const RTCTX *ctx, RTMPOLY *mpoly)
{
	int i;
	if ( ! mpoly ) return;
	if ( mpoly->bbox )
		rtfree(ctx, mpoly->bbox);

	for ( i = 0; i < mpoly->ngeoms; i++ )
		if ( mpoly->geoms && mpoly->geoms[i] )
			rtpoly_free(ctx, mpoly->geoms[i]);

	if ( mpoly->geoms )
		rtfree(ctx, mpoly->geoms);

	rtfree(ctx, mpoly);
}

