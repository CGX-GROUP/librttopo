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
#include "rttopo_config.h"
/*#define RTGEOM_DEBUG_LEVEL 4*/
#include "librtgeom_internal.h"
#include "rtgeom_log.h"


/*
 * Convenience functions to hide the RTPOINTARRAY
 * TODO: obsolete this
 */
int
rtpoint_getPoint2d_p(const RTPOINT *point, RTPOINT2D *out)
{
	return getPoint2d_p(point->point, 0, out);
}

/* convenience functions to hide the RTPOINTARRAY */
int
rtpoint_getPoint3dz_p(const RTPOINT *point, RTPOINT3DZ *out)
{
	return getPoint3dz_p(point->point,0,out);
}
int
rtpoint_getPoint3dm_p(const RTPOINT *point, RTPOINT3DM *out)
{
	return getPoint3dm_p(point->point,0,out);
}
int
rtpoint_getPoint4d_p(const RTPOINT *point, RTPOINT4D *out)
{
	return getPoint4d_p(point->point,0,out);
}

double
rtpoint_get_x(const RTPOINT *point)
{
	RTPOINT4D pt;
	if ( rtpoint_is_empty(point) )
		rterror("rtpoint_get_x called with empty geometry");
	getPoint4d_p(point->point, 0, &pt);
	return pt.x;
}

double
rtpoint_get_y(const RTPOINT *point)
{
	RTPOINT4D pt;
	if ( rtpoint_is_empty(point) )
		rterror("rtpoint_get_y called with empty geometry");
	getPoint4d_p(point->point, 0, &pt);
	return pt.y;
}

double
rtpoint_get_z(const RTPOINT *point)
{
	RTPOINT4D pt;
	if ( rtpoint_is_empty(point) )
		rterror("rtpoint_get_z called with empty geometry");
	if ( ! RTFLAGS_GET_Z(point->flags) )
		rterror("rtpoint_get_z called without z dimension");
	getPoint4d_p(point->point, 0, &pt);
	return pt.z;
}

double
rtpoint_get_m(const RTPOINT *point)
{
	RTPOINT4D pt;
	if ( rtpoint_is_empty(point) )
		rterror("rtpoint_get_m called with empty geometry");
	if ( ! RTFLAGS_GET_M(point->flags) )
		rterror("rtpoint_get_m called without m dimension");
	getPoint4d_p(point->point, 0, &pt);
	return pt.m;
}

/*
 * Construct a new point.  point will not be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTPOINT *
rtpoint_construct(int srid, RTGBOX *bbox, RTPOINTARRAY *point)
{
	RTPOINT *result;
	uint8_t flags = 0;

	if (point == NULL)
		return NULL; /* error */

	result = rtalloc(sizeof(RTPOINT));
	result->type = RTPOINTTYPE;
	RTFLAGS_SET_Z(flags, RTFLAGS_GET_Z(point->flags));
	RTFLAGS_SET_M(flags, RTFLAGS_GET_M(point->flags));
	RTFLAGS_SET_BBOX(flags, bbox?1:0);
	result->flags = flags;
	result->srid = srid;
	result->point = point;
	result->bbox = bbox;

	return result;
}

RTPOINT *
rtpoint_construct_empty(int srid, char hasz, char hasm)
{
	RTPOINT *result = rtalloc(sizeof(RTPOINT));
	result->type = RTPOINTTYPE;
	result->flags = gflags(hasz, hasm, 0);
	result->srid = srid;
	result->point = ptarray_construct(hasz, hasm, 0);
	result->bbox = NULL;
	return result;
}

RTPOINT *
rtpoint_make2d(int srid, double x, double y)
{
	RTPOINT4D p = {x, y, 0.0, 0.0};
	RTPOINTARRAY *pa = ptarray_construct_empty(0, 0, 1);

	ptarray_append_point(pa, &p, RT_TRUE);
	return rtpoint_construct(srid, NULL, pa);
}

RTPOINT *
rtpoint_make3dz(int srid, double x, double y, double z)
{
	RTPOINT4D p = {x, y, z, 0.0};
	RTPOINTARRAY *pa = ptarray_construct_empty(1, 0, 1);

	ptarray_append_point(pa, &p, RT_TRUE);

	return rtpoint_construct(srid, NULL, pa);
}

RTPOINT *
rtpoint_make3dm(int srid, double x, double y, double m)
{
	RTPOINT4D p = {x, y, 0.0, m};
	RTPOINTARRAY *pa = ptarray_construct_empty(0, 1, 1);

	ptarray_append_point(pa, &p, RT_TRUE);

	return rtpoint_construct(srid, NULL, pa);
}

RTPOINT *
rtpoint_make4d(int srid, double x, double y, double z, double m)
{
	RTPOINT4D p = {x, y, z, m};
	RTPOINTARRAY *pa = ptarray_construct_empty(1, 1, 1);

	ptarray_append_point(pa, &p, RT_TRUE);

	return rtpoint_construct(srid, NULL, pa);
}

RTPOINT *
rtpoint_make(int srid, int hasz, int hasm, const RTPOINT4D *p)
{
	RTPOINTARRAY *pa = ptarray_construct_empty(hasz, hasm, 1);
	ptarray_append_point(pa, p, RT_TRUE);
	return rtpoint_construct(srid, NULL, pa);
}

void rtpoint_free(RTPOINT *pt)
{
	if ( ! pt ) return;
	
	if ( pt->bbox )
		rtfree(pt->bbox);
	if ( pt->point )
		ptarray_free(pt->point);
	rtfree(pt);
}

void printRTPOINT(RTPOINT *point)
{
	rtnotice("RTPOINT {");
	rtnotice("    ndims = %i", (int)RTFLAGS_NDIMS(point->flags));
	rtnotice("    BBOX = %i", RTFLAGS_GET_BBOX(point->flags) ? 1 : 0 );
	rtnotice("    SRID = %i", (int)point->srid);
	printPA(point->point);
	rtnotice("}");
}

/* @brief Clone RTPOINT object. Serialized point lists are not copied.
 *
 * @see ptarray_clone 
 */
RTPOINT *
rtpoint_clone(const RTPOINT *g)
{
	RTPOINT *ret = rtalloc(sizeof(RTPOINT));

	RTDEBUG(2, "rtpoint_clone called");

	memcpy(ret, g, sizeof(RTPOINT));

	ret->point = ptarray_clone(g->point);

	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	return ret;
}



void
rtpoint_release(RTPOINT *rtpoint)
{
	rtgeom_release(rtpoint_as_rtgeom(rtpoint));
}


/* check coordinate equality  */
char
rtpoint_same(const RTPOINT *p1, const RTPOINT *p2)
{
	return ptarray_same(p1->point, p2->point);
}


RTPOINT*
rtpoint_force_dims(const RTPOINT *point, int hasz, int hasm)
{
	RTPOINTARRAY *pdims = NULL;
	RTPOINT *pointout;
	
	/* Return 2D empty */
	if( rtpoint_is_empty(point) )
	{
		pointout = rtpoint_construct_empty(point->srid, hasz, hasm);
	}
	else
	{
		/* Artays we duplicate the ptarray and return */
		pdims = ptarray_force_dims(point->point, hasz, hasm);
		pointout = rtpoint_construct(point->srid, NULL, pdims);
	}
	pointout->type = point->type;
	return pointout;
}

int rtpoint_is_empty(const RTPOINT *point)
{
	if ( ! point->point || point->point->npoints < 1 )
		return RT_TRUE;
	return RT_FALSE;
}


RTPOINT *
rtpoint_grid(const RTPOINT *point, const gridspec *grid)
{
	RTPOINTARRAY *opa = ptarray_grid(point->point, grid);
	return rtpoint_construct(point->srid, NULL, opa);
}

