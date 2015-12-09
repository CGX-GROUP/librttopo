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
rtmpoint_release(RTMPOINT *rtmpoint)
{
	rtgeom_release(rtmpoint_as_rtgeom(rtmpoint));
}

RTMPOINT *
rtmpoint_construct_empty(int srid, char hasz, char hasm)
{
	RTMPOINT *ret = (RTMPOINT*)rtcollection_construct_empty(RTMULTIPOINTTYPE, srid, hasz, hasm);
	return ret;
}

RTMPOINT* rtmpoint_add_rtpoint(RTMPOINT *mobj, const RTPOINT *obj)
{
	RTDEBUG(4, "Called");
	return (RTMPOINT*)rtcollection_add_rtgeom((RTCOLLECTION*)mobj, (RTGEOM*)obj);
}

RTMPOINT *
rtmpoint_construct(int srid, const RTPOINTARRAY *pa)
{
	int i;
	int hasz = ptarray_has_z(pa);
	int hasm = ptarray_has_m(pa);
	RTMPOINT *ret = (RTMPOINT*)rtcollection_construct_empty(RTMULTIPOINTTYPE, srid, hasz, hasm);
	
	for ( i = 0; i < pa->npoints; i++ )
	{
		RTPOINT *rtp;
		RTPOINT4D p;
		getPoint4d_p(pa, i, &p);		
		rtp = rtpoint_make(srid, hasz, hasm, &p);
		rtmpoint_add_rtpoint(ret, rtp);
	}
	
	return ret;
}


void rtmpoint_free(RTMPOINT *mpt)
{
	int i;

	if ( ! mpt ) return;
	
	if ( mpt->bbox )
		rtfree(mpt->bbox);

	for ( i = 0; i < mpt->ngeoms; i++ )
		if ( mpt->geoms && mpt->geoms[i] )
			rtpoint_free(mpt->geoms[i]);

	if ( mpt->geoms )
		rtfree(mpt->geoms);

	rtfree(mpt);
}

RTGEOM*
rtmpoint_remove_repeated_points(const RTMPOINT *mpoint, double tolerance)
{
	uint32_t nnewgeoms;
	uint32_t i, j;
	RTGEOM **newgeoms;

	newgeoms = rtalloc(sizeof(RTGEOM *)*mpoint->ngeoms);
	nnewgeoms = 0;
	for (i=0; i<mpoint->ngeoms; ++i)
	{
		/* Brute force, may be optimized by building an index */
		int seen=0;
		for (j=0; j<nnewgeoms; ++j)
		{
			if ( rtpoint_same((RTPOINT*)newgeoms[j],
			                  (RTPOINT*)mpoint->geoms[i]) )
			{
				seen=1;
				break;
			}
		}
		if ( seen ) continue;
		newgeoms[nnewgeoms++] = (RTGEOM*)rtpoint_clone(mpoint->geoms[i]);
	}

	return (RTGEOM*)rtcollection_construct(mpoint->type,
	                                       mpoint->srid, mpoint->bbox ? gbox_copy(mpoint->bbox) : NULL,
	                                       nnewgeoms, newgeoms);

}

