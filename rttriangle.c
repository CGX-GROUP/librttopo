/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * 
 * Copyright (C) 2010 - Oslandia
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* basic RTTRIANGLE manipulation */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtgeom_internal.h"
#include "rtgeom_log.h"



/* construct a new RTTRIANGLE.
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTTRIANGLE*
rttriangle_construct(int srid, RTGBOX *bbox, POINTARRAY *points)
{
	RTTRIANGLE *result;

	result = (RTTRIANGLE*) rtalloc(sizeof(RTTRIANGLE));
	result->type = RTTRIANGLETYPE;

	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);
	
	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

RTTRIANGLE*
rttriangle_construct_empty(int srid, char hasz, char hasm)
{
	RTTRIANGLE *result = rtalloc(sizeof(RTTRIANGLE));
	result->type = RTTRIANGLETYPE;
	result->flags = gflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}

void rttriangle_free(RTTRIANGLE  *triangle)
{
	if ( ! triangle ) return;
	
	if (triangle->bbox)
		rtfree(triangle->bbox);
		
	if (triangle->points)
		ptarray_free(triangle->points);
		
	rtfree(triangle);
}

void printRTTRIANGLE(RTTRIANGLE *triangle)
{
	if (triangle->type != RTTRIANGLETYPE)
                rterror("printRTTRIANGLE called with something else than a Triangle");

	rtnotice("RTTRIANGLE {");
	rtnotice("    ndims = %i", (int)FLAGS_NDIMS(triangle->flags));
	rtnotice("    SRID = %i", (int)triangle->srid);
	printPA(triangle->points);
	rtnotice("}");
}

/* @brief Clone RTTRIANGLE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone 
 */
RTTRIANGLE *
rttriangle_clone(const RTTRIANGLE *g)
{
	RTDEBUGF(2, "rttriangle_clone called with %p", g);
	return (RTTRIANGLE *)rtline_clone((const RTLINE *)g);
}

void
rttriangle_force_clockwise(RTTRIANGLE *triangle)
{
	if ( ptarray_isccw(triangle->points) )
		ptarray_reverse(triangle->points);
}

void
rttriangle_reverse(RTTRIANGLE *triangle)
{
	if( rttriangle_is_empty(triangle) ) return;
	ptarray_reverse(triangle->points);
}

void
rttriangle_release(RTTRIANGLE *rttriangle)
{
	rtgeom_release(rttriangle_as_rtgeom(rttriangle));
}

/* check coordinate equality  */
char
rttriangle_same(const RTTRIANGLE *t1, const RTTRIANGLE *t2)
{
	char r = ptarray_same(t1->points, t2->points);
	RTDEBUGF(5, "returning %d", r);
	return r;
}

/*
 * Construct a triangle from a RTLINE being
 * the shell
 * Pointarray from intput geom are cloned.
 * Input line must have 4 points, and be closed.
 */
RTTRIANGLE *
rttriangle_from_rtline(const RTLINE *shell)
{
	RTTRIANGLE *ret;
	POINTARRAY *pa;

	if ( shell->points->npoints != 4 )
		rterror("rttriangle_from_rtline: shell must have exactly 4 points");

	if (   (!FLAGS_GET_Z(shell->flags) && !ptarray_is_closed_2d(shell->points)) ||
	        (FLAGS_GET_Z(shell->flags) && !ptarray_is_closed_3d(shell->points)) )
		rterror("rttriangle_from_rtline: shell must be closed");

	pa = ptarray_clone_deep(shell->points);
	ret = rttriangle_construct(shell->srid, NULL, pa);

	if (rttriangle_is_repeated_points(ret))
		rterror("rttriangle_from_rtline: some points are repeated in triangle");

	return ret;
}

char
rttriangle_is_repeated_points(RTTRIANGLE *triangle)
{
	char ret;
	POINTARRAY *pa;

	pa = ptarray_remove_repeated_points(triangle->points, 0.0);
	ret = ptarray_same(pa, triangle->points);
	ptarray_free(pa);

	return ret;
}

int rttriangle_is_empty(const RTTRIANGLE *triangle)
{
	if ( !triangle->points || triangle->points->npoints < 1 )
		return RT_TRUE;
	return RT_FALSE;
}

/**
 * Find the area of the outer ring 
 */
double
rttriangle_area(const RTTRIANGLE *triangle)
{
	double area=0.0;
	int i;
	RTPOINT2D p1;
	RTPOINT2D p2;

	if (! triangle->points->npoints) return area; /* empty triangle */

	for (i=0; i < triangle->points->npoints-1; i++)
	{
		getPoint2d_p(triangle->points, i, &p1);
		getPoint2d_p(triangle->points, i+1, &p2);
		area += ( p1.x * p2.y ) - ( p1.y * p2.x );
	}

	area  /= 2.0;

	return fabs(area);
}


double
rttriangle_perimeter(const RTTRIANGLE *triangle)
{
	if( triangle->points ) 
		return ptarray_length(triangle->points);
	else 
		return 0.0;
}

double
rttriangle_perimeter_2d(const RTTRIANGLE *triangle)
{
	if( triangle->points ) 
		return ptarray_length_2d(triangle->points);
	else 
		return 0.0;
}
