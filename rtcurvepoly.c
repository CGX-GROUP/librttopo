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

/* basic RTCURVEPOLY manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtgeom_internal.h"
#include "rtgeom_log.h"


RTCURVEPOLY *
rtcurvepoly_construct_empty(int srid, char hasz, char hasm)
{
	RTCURVEPOLY *ret;

	ret = rtalloc(sizeof(RTCURVEPOLY));
	ret->type = RTCURVEPOLYTYPE;
	ret->flags = gflags(hasz, hasm, 0);
	ret->srid = srid;
	ret->nrings = 0;
	ret->maxrings = 1; /* Allocate room for sub-members, just in case. */
	ret->rings = rtalloc(ret->maxrings * sizeof(RTGEOM*));
	ret->bbox = NULL;

	return ret;
}

RTCURVEPOLY *
rtcurvepoly_construct_from_rtpoly(RTPOLY *rtpoly)
{
	RTCURVEPOLY *ret;
	int i;
	ret = rtalloc(sizeof(RTCURVEPOLY));
	ret->type = RTCURVEPOLYTYPE;
	ret->flags = rtpoly->flags;
	ret->srid = rtpoly->srid;
	ret->nrings = rtpoly->nrings;
	ret->maxrings = rtpoly->nrings; /* Allocate room for sub-members, just in case. */
	ret->rings = rtalloc(ret->maxrings * sizeof(RTGEOM*));
	ret->bbox = rtpoly->bbox ? gbox_clone(rtpoly->bbox) : NULL;
	for ( i = 0; i < ret->nrings; i++ )
	{
		ret->rings[i] = rtline_as_rtgeom(rtline_construct(ret->srid, NULL, ptarray_clone_deep(rtpoly->rings[i])));
	}
	return ret;
}

int rtcurvepoly_add_ring(RTCURVEPOLY *poly, RTGEOM *ring)
{
	int i;
	
	/* Can't do anything with NULLs */
	if( ! poly || ! ring ) 
	{
		RTDEBUG(4,"NULL inputs!!! quitting");
		return RT_FAILURE;
	}

	/* Check that we're not working with garbage */
	if ( poly->rings == NULL && (poly->nrings || poly->maxrings) )
	{
		RTDEBUG(4,"mismatched nrings/maxrings");
		rterror("Curvepolygon is in inconsistent state. Null memory but non-zero collection counts.");
	}

	/* Check that we're adding an allowed ring type */
	if ( ! ( ring->type == RTLINETYPE || ring->type == RTCIRCSTRINGTYPE || ring->type == RTCOMPOUNDTYPE ) )
	{
		RTDEBUGF(4,"got incorrect ring type: %s",rttype_name(ring->type));
		return RT_FAILURE;
	}

		
	/* In case this is a truly empty, make some initial space  */
	if ( poly->rings == NULL )
	{
		poly->maxrings = 2;
		poly->nrings = 0;
		poly->rings = rtalloc(poly->maxrings * sizeof(RTGEOM*));
	}

	/* Allocate more space if we need it */
	if ( poly->nrings == poly->maxrings )
	{
		poly->maxrings *= 2;
		poly->rings = rtrealloc(poly->rings, sizeof(RTGEOM*) * poly->maxrings);
	}

	/* Make sure we don't already have a reference to this geom */
	for ( i = 0; i < poly->nrings; i++ )
	{
		if ( poly->rings[i] == ring )
		{
			RTDEBUGF(4, "Found duplicate geometry in collection %p == %p", poly->rings[i], ring);
			return RT_SUCCESS;
		}
	}

	/* Add the ring and increment the ring count */
	poly->rings[poly->nrings] = (RTGEOM*)ring;
	poly->nrings++;
	return RT_SUCCESS;	
}

/**
 * This should be rewritten to make use of the curve itself.
 */
double
rtcurvepoly_area(const RTCURVEPOLY *curvepoly)
{
	double area = 0.0;
	RTPOLY *poly;
	if( rtgeom_is_empty((RTGEOM*)curvepoly) )
		return 0.0;
	poly = rtcurvepoly_stroke(curvepoly, 32);
	area = rtpoly_area(poly);
	rtpoly_free(poly);
	return area;
}


double
rtcurvepoly_perimeter(const RTCURVEPOLY *poly)
{
	double result=0.0;
	int i;

	for (i=0; i<poly->nrings; i++)
		result += rtgeom_length(poly->rings[i]);

	return result;
}

double
rtcurvepoly_perimeter_2d(const RTCURVEPOLY *poly)
{
	double result=0.0;
	int i;

	for (i=0; i<poly->nrings; i++)
		result += rtgeom_length_2d(poly->rings[i]);

	return result;
}
