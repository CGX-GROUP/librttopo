/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2012 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/* basic RTLINE functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtgeom_internal.h"
#include "rtgeom_log.h"



/*
 * Construct a new RTLINE.  points will *NOT* be copied
 * use SRID=SRID_UNKNOWN for unknown SRID (will have 8bit type's S = 0)
 */
RTLINE *
rtline_construct(int srid, RTGBOX *bbox, POINTARRAY *points)
{
	RTLINE *result;
	result = (RTLINE*) rtalloc(sizeof(RTLINE));

	RTDEBUG(2, "rtline_construct called.");

	result->type = RTLINETYPE;
	
	result->flags = points->flags;
	FLAGS_SET_BBOX(result->flags, bbox?1:0);

	RTDEBUGF(3, "rtline_construct type=%d", result->type);

	result->srid = srid;
	result->points = points;
	result->bbox = bbox;

	return result;
}

RTLINE *
rtline_construct_empty(int srid, char hasz, char hasm)
{
	RTLINE *result = rtalloc(sizeof(RTLINE));
	result->type = RTLINETYPE;
	result->flags = gflags(hasz,hasm,0);
	result->srid = srid;
	result->points = ptarray_construct_empty(hasz, hasm, 1);
	result->bbox = NULL;
	return result;
}


void rtline_free (RTLINE  *line)
{
	if ( ! line ) return;
	
	if ( line->bbox )
		rtfree(line->bbox);
	if ( line->points )
		ptarray_free(line->points);
	rtfree(line);
}


void printRTLINE(RTLINE *line)
{
	rtnotice("RTLINE {");
	rtnotice("    ndims = %i", (int)FLAGS_NDIMS(line->flags));
	rtnotice("    srid = %i", (int)line->srid);
	printPA(line->points);
	rtnotice("}");
}

/* @brief Clone RTLINE object. Serialized point lists are not copied.
 *
 * @see ptarray_clone 
 */
RTLINE *
rtline_clone(const RTLINE *g)
{
	RTLINE *ret = rtalloc(sizeof(RTLINE));

	RTDEBUGF(2, "rtline_clone called with %p", g);

	memcpy(ret, g, sizeof(RTLINE));

	ret->points = ptarray_clone(g->points);

	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	return ret;
}

/* Deep clone RTLINE object. POINTARRAY *is* copied. */
RTLINE *
rtline_clone_deep(const RTLINE *g)
{
	RTLINE *ret = rtalloc(sizeof(RTLINE));

	RTDEBUGF(2, "rtline_clone_deep called with %p", g);
	memcpy(ret, g, sizeof(RTLINE));

	if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	if ( g->points ) ret->points = ptarray_clone_deep(g->points);
	FLAGS_SET_READONLY(ret->flags,0);

	return ret;
}


void
rtline_release(RTLINE *rtline)
{
	rtgeom_release(rtline_as_rtgeom(rtline));
}

void
rtline_reverse(RTLINE *line)
{
	if ( rtline_is_empty(line) ) return;
	ptarray_reverse(line->points);
}

RTLINE *
rtline_segmentize2d(RTLINE *line, double dist)
{
	POINTARRAY *segmentized = ptarray_segmentize2d(line->points, dist);
	if ( ! segmentized ) return NULL;
	return rtline_construct(line->srid, NULL, segmentized);
}

/* check coordinate equality  */
char
rtline_same(const RTLINE *l1, const RTLINE *l2)
{
	return ptarray_same(l1->points, l2->points);
}

/*
 * Construct a RTLINE from an array of point and line geometries
 * RTLINE dimensions are large enough to host all input dimensions.
 */
RTLINE *
rtline_from_rtgeom_array(int srid, uint32_t ngeoms, RTGEOM **geoms)
{
 	int i;
	int hasz = RT_FALSE;
	int hasm = RT_FALSE;
	POINTARRAY *pa;
	RTLINE *line;
	RTPOINT4D pt;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i=0; i<ngeoms; i++)
	{
		if ( FLAGS_GET_Z(geoms[i]->flags) ) hasz = RT_TRUE;
		if ( FLAGS_GET_M(geoms[i]->flags) ) hasm = RT_TRUE;
		if ( hasz && hasm ) break; /* Nothing more to learn! */
	}

	/* ngeoms should be a guess about how many points we have in input */
	pa = ptarray_construct_empty(hasz, hasm, ngeoms);
	
	for ( i=0; i < ngeoms; i++ )
	{
		RTGEOM *g = geoms[i];

		if ( rtgeom_is_empty(g) ) continue;

		if ( g->type == RTPOINTTYPE )
		{
			rtpoint_getPoint4d_p((RTPOINT*)g, &pt);
			ptarray_append_point(pa, &pt, RT_TRUE);
		}
		else if ( g->type == RTLINETYPE )
		{
			ptarray_append_ptarray(pa, ((RTLINE*)g)->points, -1);
		}
		else
		{
			ptarray_free(pa);
			rterror("rtline_from_ptarray: invalid input type: %s", rttype_name(g->type));
			return NULL;
		}
	}

	if ( pa->npoints > 0 )
		line = rtline_construct(srid, NULL, pa);
	else  {
		/* Is this really any different from the above ? */
		ptarray_free(pa);
		line = rtline_construct_empty(srid, hasz, hasm);
	}
	
	return line;
}

/*
 * Construct a RTLINE from an array of RTPOINTs
 * RTLINE dimensions are large enough to host all input dimensions.
 */
RTLINE *
rtline_from_ptarray(int srid, uint32_t npoints, RTPOINT **points)
{
 	int i;
	int hasz = RT_FALSE;
	int hasm = RT_FALSE;
	POINTARRAY *pa;
	RTLINE *line;
	RTPOINT4D pt;

	/*
	 * Find output dimensions, check integrity
	 */
	for (i=0; i<npoints; i++)
	{
		if ( points[i]->type != RTPOINTTYPE )
		{
			rterror("rtline_from_ptarray: invalid input type: %s", rttype_name(points[i]->type));
			return NULL;
		}
		if ( FLAGS_GET_Z(points[i]->flags) ) hasz = RT_TRUE;
		if ( FLAGS_GET_M(points[i]->flags) ) hasm = RT_TRUE;
		if ( hasz && hasm ) break; /* Nothing more to learn! */
	}

	pa = ptarray_construct_empty(hasz, hasm, npoints);
	
	for ( i=0; i < npoints; i++ )
	{
		if ( ! rtpoint_is_empty(points[i]) )
		{
			rtpoint_getPoint4d_p(points[i], &pt);
			ptarray_append_point(pa, &pt, RT_TRUE);
		}
	}

	if ( pa->npoints > 0 )
		line = rtline_construct(srid, NULL, pa);
	else 
		line = rtline_construct_empty(srid, hasz, hasm);
	
	return line;
}

/*
 * Construct a RTLINE from a RTMPOINT
 */
RTLINE *
rtline_from_rtmpoint(int srid, const RTMPOINT *mpoint)
{
	uint32_t i;
	POINTARRAY *pa = NULL;
	RTGEOM *rtgeom = (RTGEOM*)mpoint;
	RTPOINT4D pt;

	char hasz = rtgeom_has_z(rtgeom);
	char hasm = rtgeom_has_m(rtgeom);
	uint32_t npoints = mpoint->ngeoms;

	if ( rtgeom_is_empty(rtgeom) ) 
	{
		return rtline_construct_empty(srid, hasz, hasm);
	}

	pa = ptarray_construct(hasz, hasm, npoints);

	for (i=0; i < npoints; i++)
	{
		getPoint4d_p(mpoint->geoms[i]->point, 0, &pt);
		ptarray_set_point4d(pa, i, &pt);
	}
	
	RTDEBUGF(3, "rtline_from_rtmpoint: constructed pointarray for %d points", mpoint->ngeoms);

	return rtline_construct(srid, NULL, pa);
}

/**
* Returns freshly allocated #RTPOINT that corresponds to the index where.
* Returns NULL if the geometry is empty or the index invalid.
*/
RTPOINT*
rtline_get_rtpoint(const RTLINE *line, int where)
{
	RTPOINT4D pt;
	RTPOINT *rtpoint;
	POINTARRAY *pa;

	if ( rtline_is_empty(line) || where < 0 || where >= line->points->npoints )
		return NULL;

	pa = ptarray_construct_empty(FLAGS_GET_Z(line->flags), FLAGS_GET_M(line->flags), 1);
	pt = getPoint4d(line->points, where);
	ptarray_append_point(pa, &pt, RT_TRUE);
	rtpoint = rtpoint_construct(line->srid, NULL, pa);
	return rtpoint;
}


int
rtline_add_rtpoint(RTLINE *line, RTPOINT *point, int where)
{
	RTPOINT4D pt;	
	getPoint4d_p(point->point, 0, &pt);

	if ( ptarray_insert_point(line->points, &pt, where) != RT_SUCCESS )
		return RT_FAILURE;

	/* Update the bounding box */
	if ( line->bbox )
	{
		rtgeom_drop_bbox(rtline_as_rtgeom(line));
		rtgeom_add_bbox(rtline_as_rtgeom(line));
	}
	
	return RT_SUCCESS;
}



RTLINE *
rtline_removepoint(RTLINE *line, uint32_t index)
{
	POINTARRAY *newpa;
	RTLINE *ret;

	newpa = ptarray_removePoint(line->points, index);

	ret = rtline_construct(line->srid, NULL, newpa);
	rtgeom_add_bbox((RTGEOM *) ret);

	return ret;
}

/*
 * Note: input will be changed, make sure you have permissions for this.
 */
void
rtline_setPoint4d(RTLINE *line, uint32_t index, RTPOINT4D *newpoint)
{
	ptarray_set_point4d(line->points, index, newpoint);
	/* Update the box, if there is one to update */
	if ( line->bbox )
	{
		rtgeom_drop_bbox((RTGEOM*)line);
		rtgeom_add_bbox((RTGEOM*)line);
	}
}

/**
* Re-write the measure ordinate (or add one, if it isn't already there) interpolating
* the measure between the supplied start and end values.
*/
RTLINE*
rtline_measured_from_rtline(const RTLINE *rtline, double m_start, double m_end)
{
	int i = 0;
	int hasm = 0, hasz = 0;
	int npoints = 0;
	double length = 0.0;
	double length_so_far = 0.0;
	double m_range = m_end - m_start;
	double m;
	POINTARRAY *pa = NULL;
	RTPOINT3DZ p1, p2;

	if ( rtline->type != RTLINETYPE )
	{
		rterror("rtline_construct_from_rtline: only line types supported");
		return NULL;
	}

	hasz = FLAGS_GET_Z(rtline->flags);
	hasm = 1;

	/* Null points or npoints == 0 will result in empty return geometry */
	if ( rtline->points )
	{
		npoints = rtline->points->npoints;
		length = ptarray_length_2d(rtline->points);
		getPoint3dz_p(rtline->points, 0, &p1);
	}

	pa = ptarray_construct(hasz, hasm, npoints);

	for ( i = 0; i < npoints; i++ )
	{
		RTPOINT4D q;
		RTPOINT2D a, b;
		getPoint3dz_p(rtline->points, i, &p2);
		a.x = p1.x;
		a.y = p1.y;
		b.x = p2.x;
		b.y = p2.y;
		length_so_far += distance2d_pt_pt(&a, &b);
		if ( length > 0.0 )
			m = m_start + m_range * length_so_far / length;
		/* #3172, support (valid) zero-length inputs */
		else if ( length == 0.0 && npoints > 1 )
			m = m_start + m_range * i / (npoints-1);
		else
			m = 0.0;
		q.x = p2.x;
		q.y = p2.y;
		q.z = p2.z;
		q.m = m;
		ptarray_set_point4d(pa, i, &q);
		p1 = p2;
	}

	return rtline_construct(rtline->srid, NULL, pa);
}

RTGEOM*
rtline_remove_repeated_points(const RTLINE *rtline, double tolerance)
{
	POINTARRAY* npts = ptarray_remove_repeated_points_minpoints(rtline->points, tolerance, 2);

	RTDEBUGF(3, "%s: npts %p", __func__, npts);

	return (RTGEOM*)rtline_construct(rtline->srid,
	                                 rtline->bbox ? gbox_copy(rtline->bbox) : 0,
	                                 npts);
}

int
rtline_is_closed(const RTLINE *line)
{
	if (FLAGS_GET_Z(line->flags))
		return ptarray_is_closed_3d(line->points);

	return ptarray_is_closed_2d(line->points);
}

int
rtline_is_trajectory(const RTLINE *line)
{
  RTPOINT3DM p;
  int i, n;
  double m = -1 * FLT_MAX;

  if ( ! FLAGS_GET_M(line->flags) ) {
    rtnotice("Line does not have M dimension");
    return RT_FALSE;
  }

  n = line->points->npoints;
  if ( n < 2 ) return RT_TRUE; /* empty or single-point are "good" */

  for (i=0; i<n; ++i) {
    getPoint3dm_p(line->points, i, &p);
    if ( p.m <= m ) {
      rtnotice("Measure of vertex %d (%g) not bigger than measure of vertex %d (%g)",
        i, p.m, i-1, m);
      return RT_FALSE;
    }
    m = p.m;
  }

  return RT_TRUE;
}


RTLINE*
rtline_force_dims(const RTLINE *line, int hasz, int hasm)
{
	POINTARRAY *pdims = NULL;
	RTLINE *lineout;
	
	/* Return 2D empty */
	if( rtline_is_empty(line) )
	{
		lineout = rtline_construct_empty(line->srid, hasz, hasm);
	}
	else
	{	
		pdims = ptarray_force_dims(line->points, hasz, hasm);
		lineout = rtline_construct(line->srid, NULL, pdims);
	}
	lineout->type = line->type;
	return lineout;
}

int rtline_is_empty(const RTLINE *line)
{
	if ( !line->points || line->points->npoints < 1 )
		return RT_TRUE;
	return RT_FALSE;
}


int rtline_count_vertices(RTLINE *line)
{
	assert(line);
	if ( ! line->points )
		return 0;
	return line->points->npoints;
}

RTLINE* rtline_simplify(const RTLINE *iline, double dist, int preserve_collapsed)
{
	static const int minvertices = 2; /* TODO: allow setting this */
	RTLINE *oline;
	POINTARRAY *pa;

	RTDEBUG(2, "function called");

	/* Skip empty case */
	if( rtline_is_empty(iline) )
		return NULL;

	pa = ptarray_simplify(iline->points, dist, minvertices);
	if ( ! pa ) return NULL;

	/* Make sure single-point collapses have two points */
	if ( pa->npoints == 1 )
	{
		/* Make sure single-point collapses have two points */
		if ( preserve_collapsed )
		{
			RTPOINT4D pt;
			getPoint4d_p(pa, 0, &pt);		
			ptarray_append_point(pa, &pt, RT_TRUE);
		}
		/* Return null for collapse */
		else 
		{
			ptarray_free(pa);
			return NULL;
		}
	}

	oline = rtline_construct(iline->srid, NULL, pa);
	oline->type = iline->type;
	return oline;
}

double rtline_length(const RTLINE *line)
{
	if ( rtline_is_empty(line) )
		return 0.0;
	return ptarray_length(line->points);
}

double rtline_length_2d(const RTLINE *line)
{
	if ( rtline_is_empty(line) )
		return 0.0;
	return ptarray_length_2d(line->points);
}



RTLINE* rtline_grid(const RTLINE *line, const gridspec *grid)
{
	RTLINE *oline;
	POINTARRAY *opa;

	opa = ptarray_grid(line->points, grid);

	/* Skip line3d with less then 2 points */
	if ( opa->npoints < 2 ) return NULL;

	/* TODO: grid bounding box... */
	oline = rtline_construct(line->srid, NULL, opa);

	return oline;
}

