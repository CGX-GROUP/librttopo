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

/* basic RTCIRCSTRING functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtgeom_internal.h"
#include "rtgeom_log.h"

void printRTCIRCSTRING(RTCIRCSTRING *curve);
void rtcircstring_reverse(RTCIRCSTRING *curve);
void rtcircstring_release(RTCIRCSTRING *rtcirc);
char rtcircstring_same(const RTCIRCSTRING *me, const RTCIRCSTRING *you);
RTCIRCSTRING *rtcircstring_from_rtpointarray(int srid, uint32_t npoints, RTPOINT **points);
RTCIRCSTRING *rtcircstring_from_rtmpoint(int srid, RTMPOINT *mpoint);
RTCIRCSTRING *rtcircstring_addpoint(RTCIRCSTRING *curve, RTPOINT *point, uint32_t where);
RTCIRCSTRING *rtcircstring_removepoint(RTCIRCSTRING *curve, uint32_t index);
void rtcircstring_setPoint4d(RTCIRCSTRING *curve, uint32_t index, RTPOINT4D *newpoint);



/*
 * Construct a new RTCIRCSTRING.  points will *NOT* be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTCIRCSTRING *
rtcircstring_construct(int srid, RTGBOX *bbox, RTPOINTARRAY *points)
{
	RTCIRCSTRING *result;

	/*
	* The first arc requires three points.  Each additional
	* arc requires two more points.  Thus the minimum point count
	* is three, and the count must be odd.
	*/
	if (points->npoints % 2 != 1 || points->npoints < 3)
	{
		rtnotice("rtcircstring_construct: invalid point count %d", points->npoints);
	}

	result = (RTCIRCSTRING*) rtalloc(sizeof(RTCIRCSTRING));

	result->type = RTCIRCSTRINGTYPE;
	
	result->flags = points->flags;
	RTFLAGS_SET_BBOX(result->flags, bbox?1:0);

	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

RTCIRCSTRING *
rtcircstring_construct_empty(int srid, char hasz, char hasm)
{
	RTCIRCSTRING *result = rtalloc(sizeof(RTCIRCSTRING));
	result->type = RTCIRCSTRINGTYPE;
	result->flags = gflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}

void
rtcircstring_release(RTCIRCSTRING *rtcirc)
{
	rtgeom_release(rtcircstring_as_rtgeom(rtcirc));
}


void rtcircstring_free(RTCIRCSTRING *curve)
{
	if ( ! curve ) return;
	
	if ( curve->bbox )
		rtfree(curve->bbox);
	if ( curve->points )
		ptarray_free(curve->points);
	rtfree(curve);
}



void printRTCIRCSTRING(RTCIRCSTRING *curve)
{
	rtnotice("RTCIRCSTRING {");
	rtnotice("    ndims = %i", (int)RTFLAGS_NDIMS(curve->flags));
	rtnotice("    srid = %i", (int)curve->srid);
	printPA(curve->points);
	rtnotice("}");
}

/* @brief Clone RTCIRCSTRING object. Serialized point lists are not copied.
 *
 * @see ptarray_clone 
 */
RTCIRCSTRING *
rtcircstring_clone(const RTCIRCSTRING *g)
{
	return (RTCIRCSTRING *)rtline_clone((RTLINE *)g);
}


void rtcircstring_reverse(RTCIRCSTRING *curve)
{
	ptarray_reverse(curve->points);
}

/* check coordinate equality */
char
rtcircstring_same(const RTCIRCSTRING *me, const RTCIRCSTRING *you)
{
	return ptarray_same(me->points, you->points);
}

/*
 * Construct a RTCIRCSTRING from an array of RTPOINTs
 * RTCIRCSTRING dimensions are large enough to host all input dimensions.
 */
RTCIRCSTRING *
rtcircstring_from_rtpointarray(int srid, uint32_t npoints, RTPOINT **points)
{
	int zmflag=0;
	uint32_t i;
	RTPOINTARRAY *pa;
	uint8_t *newpoints, *ptr;
	size_t ptsize, size;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i = 0; i < npoints; i++)
	{
		if (points[i]->type != RTPOINTTYPE)
		{
			rterror("rtcurve_from_rtpointarray: invalid input type: %s",
			        rttype_name(points[i]->type));
			return NULL;
		}
		if (RTFLAGS_GET_Z(points[i]->flags)) zmflag |= 2;
		if (RTFLAGS_GET_M(points[i]->flags)) zmflag |= 1;
		if (zmflag == 3) break;
	}

	if (zmflag == 0) ptsize = 2 * sizeof(double);
	else if (zmflag == 3) ptsize = 4 * sizeof(double);
	else ptsize = 3 * sizeof(double);

	/*
	 * Allocate output points array
	 */
	size = ptsize * npoints;
	newpoints = rtalloc(size);
	memset(newpoints, 0, size);

	ptr = newpoints;
	for (i = 0; i < npoints; i++)
	{
		size = ptarray_point_size(points[i]->point);
		memcpy(ptr, getPoint_internal(points[i]->point, 0), size);
		ptr += ptsize;
	}
	pa = ptarray_construct_reference_data(zmflag&2, zmflag&1, npoints, newpoints);
	
	return rtcircstring_construct(srid, NULL, pa);
}

/*
 * Construct a RTCIRCSTRING from a RTMPOINT
 */
RTCIRCSTRING *
rtcircstring_from_rtmpoint(int srid, RTMPOINT *mpoint)
{
	uint32_t i;
	RTPOINTARRAY *pa;
	char zmflag = RTFLAGS_GET_ZM(mpoint->flags);
	size_t ptsize, size;
	uint8_t *newpoints, *ptr;

	if (zmflag == 0) ptsize = 2 * sizeof(double);
	else if (zmflag == 3) ptsize = 4 * sizeof(double);
	else ptsize = 3 * sizeof(double);

	/* Allocate space for output points */
	size = ptsize * mpoint->ngeoms;
	newpoints = rtalloc(size);
	memset(newpoints, 0, size);

	ptr = newpoints;
	for (i = 0; i < mpoint->ngeoms; i++)
	{
		memcpy(ptr,
		       getPoint_internal(mpoint->geoms[i]->point, 0),
		       ptsize);
		ptr += ptsize;
	}

	pa = ptarray_construct_reference_data(zmflag&2, zmflag&1, mpoint->ngeoms, newpoints);
	
	RTDEBUGF(3, "rtcurve_from_rtmpoint: constructed pointarray for %d points, %d zmflag", mpoint->ngeoms, zmflag);

	return rtcircstring_construct(srid, NULL, pa);
}

RTCIRCSTRING *
rtcircstring_addpoint(RTCIRCSTRING *curve, RTPOINT *point, uint32_t where)
{
	RTPOINTARRAY *newpa;
	RTCIRCSTRING *ret;

	newpa = ptarray_addPoint(curve->points,
	                         getPoint_internal(point->point, 0),
	                         RTFLAGS_NDIMS(point->flags), where);
	ret = rtcircstring_construct(curve->srid, NULL, newpa);

	return ret;
}

RTCIRCSTRING *
rtcircstring_removepoint(RTCIRCSTRING *curve, uint32_t index)
{
	RTPOINTARRAY *newpa;
	RTCIRCSTRING *ret;

	newpa = ptarray_removePoint(curve->points, index);
	ret = rtcircstring_construct(curve->srid, NULL, newpa);

	return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 * */
void
rtcircstring_setPoint4d(RTCIRCSTRING *curve, uint32_t index, RTPOINT4D *newpoint)
{
	ptarray_set_point4d(curve->points, index, newpoint);
}

int
rtcircstring_is_closed(const RTCIRCSTRING *curve)
{
	if (RTFLAGS_GET_Z(curve->flags))
		return ptarray_is_closed_3d(curve->points);

	return ptarray_is_closed_2d(curve->points);
}

int rtcircstring_is_empty(const RTCIRCSTRING *circ)
{
	if ( !circ->points || circ->points->npoints < 1 )
		return RT_TRUE;
	return RT_FALSE;
}

double rtcircstring_length(const RTCIRCSTRING *circ)
{
	return rtcircstring_length_2d(circ);
}

double rtcircstring_length_2d(const RTCIRCSTRING *circ)
{
	if ( rtcircstring_is_empty(circ) )
		return 0.0;
	
	return ptarray_arc_length_2d(circ->points);
}

/*
 * Returns freshly allocated #RTPOINT that corresponds to the index where.
 * Returns NULL if the geometry is empty or the index invalid.
 */
RTPOINT* rtcircstring_get_rtpoint(const RTCIRCSTRING *circ, int where) {
	RTPOINT4D pt;
	RTPOINT *rtpoint;
	RTPOINTARRAY *pa;

	if ( rtcircstring_is_empty(circ) || where < 0 || where >= circ->points->npoints )
		return NULL;

	pa = ptarray_construct_empty(RTFLAGS_GET_Z(circ->flags), RTFLAGS_GET_M(circ->flags), 1);
	pt = getPoint4d(circ->points, where);
	ptarray_append_point(pa, &pt, RT_TRUE);
	rtpoint = rtpoint_construct(circ->srid, NULL, pa);
	return rtpoint;
}

/*
* Snap to grid 
*/
RTCIRCSTRING* rtcircstring_grid(const RTCIRCSTRING *line, const gridspec *grid)
{
	RTCIRCSTRING *oline;
	RTPOINTARRAY *opa;

	opa = ptarray_grid(line->points, grid);

	/* Skip line3d with less then 2 points */
	if ( opa->npoints < 2 ) return NULL;

	/* TODO: grid bounding box... */
	oline = rtcircstring_construct(line->srid, NULL, opa);

	return oline;
}

