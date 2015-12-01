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
rtmpoly_release(RTMPOLY *rtmpoly)
{
	rtgeom_release(rtmpoly_as_rtgeom(rtmpoly));
}

RTMPOLY *
rtmpoly_construct_empty(int srid, char hasz, char hasm)
{
	RTMPOLY *ret = (RTMPOLY*)rtcollection_construct_empty(MULTIPOLYGONTYPE, srid, hasz, hasm);
	return ret;
}


RTMPOLY* rtmpoly_add_rtpoly(RTMPOLY *mobj, const RTPOLY *obj)
{
	return (RTMPOLY*)rtcollection_add_rtgeom((RTCOLLECTION*)mobj, (RTGEOM*)obj);
}


void rtmpoly_free(RTMPOLY *mpoly)
{
	int i;
	if ( ! mpoly ) return;
	if ( mpoly->bbox )
		rtfree(mpoly->bbox);

	for ( i = 0; i < mpoly->ngeoms; i++ )
		if ( mpoly->geoms && mpoly->geoms[i] )
			rtpoly_free(mpoly->geoms[i]);

	if ( mpoly->geoms )
		rtfree(mpoly->geoms);

	rtfree(mpoly);
}

