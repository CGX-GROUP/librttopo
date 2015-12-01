/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2001-2006 Refractions Research Inc.
 * Copyright 2010 Nicklas Avén
 * Copyright 2012 Paul Ramsey
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <string.h>
#include <stdlib.h>

#include "measures.h"
#include "rtgeom_log.h"



/*------------------------------------------------------------------------------------------------------------
Initializing functions
The functions starting the distance-calculation processses
--------------------------------------------------------------------------------------------------------------*/

RTGEOM *
rtgeom_closest_line(const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distanceline(rt1, rt2, rt1->srid, DIST_MIN);
}

RTGEOM *
rtgeom_furthest_line(const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distanceline(rt1, rt2, rt1->srid, DIST_MAX);
}

RTGEOM *
rtgeom_closest_point(const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distancepoint(rt1, rt2, rt1->srid, DIST_MIN);  
}

RTGEOM *
rtgeom_furthest_point(const RTGEOM *rt1, const RTGEOM *rt2)
{
  return rt_dist2d_distancepoint(rt1, rt2, rt1->srid, DIST_MAX);  
}


void
rt_dist2d_distpts_init(DISTPTS *dl, int mode)
{
	dl->twisted = -1;
	dl->p1.x = dl->p1.y = 0.0;
	dl->p2.x = dl->p2.y = 0.0;
	dl->mode = mode;
	dl->tolerance = 0.0;
	if ( mode == DIST_MIN )
		dl->distance = FLT_MAX;
	else
		dl->distance = -1 * FLT_MAX;
}

/**
Function initializing shortestline and longestline calculations.
*/
RTGEOM *
rt_dist2d_distanceline(const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode)
{
	double x1,x2,y1,y2;

	double initdistance = ( mode == DIST_MIN ? FLT_MAX : -1.0);
	DISTPTS thedl;
	RTPOINT *rtpoints[2];
	RTGEOM *result;

	thedl.mode = mode;
	thedl.distance = initdistance;
	thedl.tolerance = 0.0;

	RTDEBUG(2, "rt_dist2d_distanceline is called");

	if (!rt_dist2d_comp( rt1,rt2,&thedl))
	{
		/*should never get here. all cases ought to be error handled earlier*/
		rterror("Some unspecified error.");
		result = (RTGEOM *)rtcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}

	/*if thedl.distance is unchanged there where only empty geometries input*/
	if (thedl.distance == initdistance)
	{
		RTDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (RTGEOM *)rtcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x1=thedl.p1.x;
		y1=thedl.p1.y;
		x2=thedl.p2.x;
		y2=thedl.p2.y;

		rtpoints[0] = rtpoint_make2d(srid, x1, y1);
		rtpoints[1] = rtpoint_make2d(srid, x2, y2);

		result = (RTGEOM *)rtline_from_ptarray(srid, 2, rtpoints);
	}
	return result;
}

/**
Function initializing closestpoint calculations.
*/
RTGEOM *
rt_dist2d_distancepoint(const RTGEOM *rt1, const RTGEOM *rt2,int srid,int mode)
{
	double x,y;
	DISTPTS thedl;
	double initdistance = FLT_MAX;
	RTGEOM *result;

	thedl.mode = mode;
	thedl.distance= initdistance;
	thedl.tolerance = 0;

	RTDEBUG(2, "rt_dist2d_distancepoint is called");

	if (!rt_dist2d_comp( rt1,rt2,&thedl))
	{
		/*should never get here. all cases ought to be error handled earlier*/
		rterror("Some unspecified error.");
		result = (RTGEOM *)rtcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	if (thedl.distance == initdistance)
	{
		RTDEBUG(3, "didn't find geometries to measure between, returning null");
		result = (RTGEOM *)rtcollection_construct_empty(COLLECTIONTYPE, srid, 0, 0);
	}
	else
	{
		x=thedl.p1.x;
		y=thedl.p1.y;
		result = (RTGEOM *)rtpoint_make2d(srid, x, y);
	}
	return result;
}


/**
Function initialazing max distance calculation
*/
double
rtgeom_maxdistance2d(const RTGEOM *rt1, const RTGEOM *rt2)
{
	RTDEBUG(2, "rtgeom_maxdistance2d is called");

	return rtgeom_maxdistance2d_tolerance( rt1, rt2, 0.0 );
}

/**
Function handling max distance calculations and dfyllywithin calculations.
The difference is just the tolerance.
*/
double
rtgeom_maxdistance2d_tolerance(const RTGEOM *rt1, const RTGEOM *rt2, double tolerance)
{
	/*double thedist;*/
	DISTPTS thedl;
	RTDEBUG(2, "rtgeom_maxdistance2d_tolerance is called");
	thedl.mode = DIST_MAX;
	thedl.distance= -1;
	thedl.tolerance = tolerance;
	if (rt_dist2d_comp( rt1,rt2,&thedl))
	{
		return thedl.distance;
	}
	/*should never get here. all cases ought to be error handled earlier*/
	rterror("Some unspecified error.");
	return -1;
}

/**
	Function initialazing min distance calculation
*/
double
rtgeom_mindistance2d(const RTGEOM *rt1, const RTGEOM *rt2)
{
	RTDEBUG(2, "rtgeom_mindistance2d is called");
	return rtgeom_mindistance2d_tolerance( rt1, rt2, 0.0 );
}

/**
	Function handling min distance calculations and dwithin calculations.
	The difference is just the tolerance.
*/
double
rtgeom_mindistance2d_tolerance(const RTGEOM *rt1, const RTGEOM *rt2, double tolerance)
{
	DISTPTS thedl;
	RTDEBUG(2, "rtgeom_mindistance2d_tolerance is called");
	thedl.mode = DIST_MIN;
	thedl.distance= FLT_MAX;
	thedl.tolerance = tolerance;
	if (rt_dist2d_comp( rt1,rt2,&thedl))
	{
		return thedl.distance;
	}
	/*should never get here. all cases ought to be error handled earlier*/
	rterror("Some unspecified error.");
	return FLT_MAX;
}


/*------------------------------------------------------------------------------------------------------------
End of Initializing functions
--------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------------------------------
Preprocessing functions
Functions preparing geometries for distance-calculations
--------------------------------------------------------------------------------------------------------------*/

/**
	This function just deserializes geometries
	Bboxes is not checked here since it is the subgeometries
	bboxes we will use anyway.
*/
int
rt_dist2d_comp(const RTGEOM *rt1,const RTGEOM *rt2, DISTPTS *dl)
{
	RTDEBUG(2, "rt_dist2d_comp is called");

	return rt_dist2d_recursive(rt1, rt2, dl);
}

static int
rt_dist2d_is_collection(const RTGEOM *g)
{

	switch (g->type)
	{
	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	case MULTICURVETYPE:
	case MULTISURFACETYPE:
	case COMPOUNDTYPE:
	case POLYHEDRALSURFACETYPE:
		return RT_TRUE;
		break;

	default:
		return RT_FALSE;
	}
}

/**
This is a recursive function delivering every possible combinatin of subgeometries
*/
int rt_dist2d_recursive(const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS *dl)
{
	int i, j;
	int n1=1;
	int n2=1;
	RTGEOM *g1 = NULL;
	RTGEOM *g2 = NULL;
	RTCOLLECTION *c1 = NULL;
	RTCOLLECTION *c2 = NULL;

	RTDEBUGF(2, "rt_dist2d_comp is called with type1=%d, type2=%d", rtg1->type, rtg2->type);

	if (rt_dist2d_is_collection(rtg1))
	{
		RTDEBUG(3, "First geometry is collection");
		c1 = rtgeom_as_rtcollection(rtg1);
		n1 = c1->ngeoms;
	}
	if (rt_dist2d_is_collection(rtg2))
	{
		RTDEBUG(3, "Second geometry is collection");
		c2 = rtgeom_as_rtcollection(rtg2);
		n2 = c2->ngeoms;
	}

	for ( i = 0; i < n1; i++ )
	{

		if (rt_dist2d_is_collection(rtg1))
		{
			g1 = c1->geoms[i];
		}
		else
		{
			g1 = (RTGEOM*)rtg1;
		}

		if (rtgeom_is_empty(g1)) return RT_TRUE;

		if (rt_dist2d_is_collection(g1))
		{
			RTDEBUG(3, "Found collection inside first geometry collection, recursing");
			if (!rt_dist2d_recursive(g1, rtg2, dl)) return RT_FALSE;
			continue;
		}
		for ( j = 0; j < n2; j++ )
		{
			if (rt_dist2d_is_collection(rtg2))
			{
				g2 = c2->geoms[j];
			}
			else
			{
				g2 = (RTGEOM*)rtg2;
			}
			if (rt_dist2d_is_collection(g2))
			{
				RTDEBUG(3, "Found collection inside second geometry collection, recursing");
				if (!rt_dist2d_recursive(g1, g2, dl)) return RT_FALSE;
				continue;
			}

			if ( ! g1->bbox )
			{
				rtgeom_add_bbox(g1);
			}
			if ( ! g2->bbox )
			{
				rtgeom_add_bbox(g2);
			}

			/*If one of geometries is empty, return. True here only means continue searching. False would have stoped the process*/
			if (rtgeom_is_empty(g1)||rtgeom_is_empty(g2)) return RT_TRUE;

			if ( (dl->mode != DIST_MAX) && 
				 (! rt_dist2d_check_overlap(g1, g2)) && 
			     (g1->type == LINETYPE || g1->type == POLYGONTYPE) && 
			     (g2->type == LINETYPE || g2->type == POLYGONTYPE) )	
			{
				if (!rt_dist2d_distribute_fast(g1, g2, dl)) return RT_FALSE;
			}
			else
			{
				if (!rt_dist2d_distribute_bruteforce(g1, g2, dl)) return RT_FALSE;
				if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/				
			}
		}
	}
	return RT_TRUE;
}


int
rt_dist2d_distribute_bruteforce(const RTGEOM *rtg1,const RTGEOM *rtg2, DISTPTS *dl)
{

	int	t1 = rtg1->type;
	int	t2 = rtg2->type;

	switch ( t1 )
	{
		case POINTTYPE:
		{
			dl->twisted = 1;
			switch ( t2 )
			{
				case POINTTYPE:
					return rt_dist2d_point_point((RTPOINT *)rtg1, (RTPOINT *)rtg2, dl);
				case LINETYPE:
					return rt_dist2d_point_line((RTPOINT *)rtg1, (RTLINE *)rtg2, dl);
				case POLYGONTYPE:
					return rt_dist2d_point_poly((RTPOINT *)rtg1, (RTPOLY *)rtg2, dl);
				case CIRCSTRINGTYPE:
					return rt_dist2d_point_circstring((RTPOINT *)rtg1, (RTCIRCSTRING *)rtg2, dl);
				case CURVEPOLYTYPE:
					return rt_dist2d_point_curvepoly((RTPOINT *)rtg1, (RTCURVEPOLY *)rtg2, dl);
				default:
					rterror("Unsupported geometry type: %s", rttype_name(t2));
			}
		}
		case LINETYPE:
		{
			dl->twisted = 1;
			switch ( t2 )
			{
				case POINTTYPE:
					dl->twisted=(-1);
					return rt_dist2d_point_line((RTPOINT *)rtg2, (RTLINE *)rtg1, dl);
				case LINETYPE:
					return rt_dist2d_line_line((RTLINE *)rtg1, (RTLINE *)rtg2, dl);
				case POLYGONTYPE:
					return rt_dist2d_line_poly((RTLINE *)rtg1, (RTPOLY *)rtg2, dl);
				case CIRCSTRINGTYPE:
					return rt_dist2d_line_circstring((RTLINE *)rtg1, (RTCIRCSTRING *)rtg2, dl);
				case CURVEPOLYTYPE:
					return rt_dist2d_line_curvepoly((RTLINE *)rtg1, (RTCURVEPOLY *)rtg2, dl);
				default:
					rterror("Unsupported geometry type: %s", rttype_name(t2));
			}
		}
		case CIRCSTRINGTYPE:
		{
			dl->twisted = 1;
			switch ( t2 )
			{
				case POINTTYPE:
					dl->twisted = -1;
					return rt_dist2d_point_circstring((RTPOINT *)rtg2, (RTCIRCSTRING *)rtg1, dl);
				case LINETYPE:
					dl->twisted = -1;
					return rt_dist2d_line_circstring((RTLINE *)rtg2, (RTCIRCSTRING *)rtg1, dl);
				case POLYGONTYPE:
					return rt_dist2d_circstring_poly((RTCIRCSTRING *)rtg1, (RTPOLY *)rtg2, dl);
				case CIRCSTRINGTYPE:
					return rt_dist2d_circstring_circstring((RTCIRCSTRING *)rtg1, (RTCIRCSTRING *)rtg2, dl);
				case CURVEPOLYTYPE:
					return rt_dist2d_circstring_curvepoly((RTCIRCSTRING *)rtg1, (RTCURVEPOLY *)rtg2, dl);
				default:
					rterror("Unsupported geometry type: %s", rttype_name(t2));
			}			
		}
		case POLYGONTYPE:
		{
			dl->twisted = -1;
			switch ( t2 )
			{
				case POINTTYPE:
					return rt_dist2d_point_poly((RTPOINT *)rtg2, (RTPOLY *)rtg1, dl);
				case LINETYPE:
					return rt_dist2d_line_poly((RTLINE *)rtg2, (RTPOLY *)rtg1, dl);
				case CIRCSTRINGTYPE:
					return rt_dist2d_circstring_poly((RTCIRCSTRING *)rtg2, (RTPOLY *)rtg1, dl);
				case POLYGONTYPE:
					dl->twisted = 1;
					return rt_dist2d_poly_poly((RTPOLY *)rtg1, (RTPOLY *)rtg2, dl);
				case CURVEPOLYTYPE:
					dl->twisted = 1;
					return rt_dist2d_poly_curvepoly((RTPOLY *)rtg1, (RTCURVEPOLY *)rtg2, dl);
				default:
					rterror("Unsupported geometry type: %s", rttype_name(t2));
			}
		}
		case CURVEPOLYTYPE:
		{
			dl->twisted = (-1);
			switch ( t2 )
			{
				case POINTTYPE:
					return rt_dist2d_point_curvepoly((RTPOINT *)rtg2, (RTCURVEPOLY *)rtg1, dl);
				case LINETYPE:
					return rt_dist2d_line_curvepoly((RTLINE *)rtg2, (RTCURVEPOLY *)rtg1, dl);
				case POLYGONTYPE:
					return rt_dist2d_poly_curvepoly((RTPOLY *)rtg2, (RTCURVEPOLY *)rtg1, dl);
				case CIRCSTRINGTYPE:
					return rt_dist2d_circstring_curvepoly((RTCIRCSTRING *)rtg2, (RTCURVEPOLY *)rtg1, dl);
				case CURVEPOLYTYPE:
					dl->twisted = 1;
					return rt_dist2d_curvepoly_curvepoly((RTCURVEPOLY *)rtg1, (RTCURVEPOLY *)rtg2, dl);
				default:
					rterror("Unsupported geometry type: %s", rttype_name(t2));
			}			
		}
		default:
		{
			rterror("Unsupported geometry type: %s", rttype_name(t1));
		}
	}

	/*You shouldn't being able to get here*/
	rterror("unspecified error in function rt_dist2d_distribute_bruteforce");
	return RT_FALSE;
}




/**

We have to check for overlapping bboxes
*/
int
rt_dist2d_check_overlap(RTGEOM *rtg1,RTGEOM *rtg2)
{
	RTDEBUG(2, "rt_dist2d_check_overlap is called");
	if ( ! rtg1->bbox )
		rtgeom_calculate_gbox(rtg1, rtg1->bbox);
	if ( ! rtg2->bbox )
		rtgeom_calculate_gbox(rtg2, rtg2->bbox);

	/*Check if the geometries intersect.
	*/
	if ((rtg1->bbox->xmax<rtg2->bbox->xmin||rtg1->bbox->xmin>rtg2->bbox->xmax||rtg1->bbox->ymax<rtg2->bbox->ymin||rtg1->bbox->ymin>rtg2->bbox->ymax))
	{
		RTDEBUG(3, "geometries bboxes did not overlap");
		return RT_FALSE;
	}
	RTDEBUG(3, "geometries bboxes overlap");
	return RT_TRUE;
}

/**

Here the geometries are distributed for the new faster distance-calculations
*/
int
rt_dist2d_distribute_fast(RTGEOM *rtg1, RTGEOM *rtg2, DISTPTS *dl)
{
	POINTARRAY *pa1, *pa2;
	int	type1 = rtg1->type;
	int	type2 = rtg2->type;

	RTDEBUGF(2, "rt_dist2d_distribute_fast is called with typ1=%d, type2=%d", rtg1->type, rtg2->type);

	switch (type1)
	{
	case LINETYPE:
		pa1 = ((RTLINE *)rtg1)->points;
		break;
	case POLYGONTYPE:
		pa1 = ((RTPOLY *)rtg1)->rings[0];
		break;
	default:
		rterror("Unsupported geometry1 type: %s", rttype_name(type1));
		return RT_FALSE;
	}
	switch (type2)
	{
	case LINETYPE:
		pa2 = ((RTLINE *)rtg2)->points;
		break;
	case POLYGONTYPE:
		pa2 = ((RTPOLY *)rtg2)->rings[0];
		break;
	default:
		rterror("Unsupported geometry2 type: %s", rttype_name(type1));
		return RT_FALSE;
	}
	dl->twisted=1;
	return rt_dist2d_fast_ptarray_ptarray(pa1, pa2, dl, rtg1->bbox, rtg2->bbox);
}

/*------------------------------------------------------------------------------------------------------------
End of Preprocessing functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Brute force functions
The old way of calculating distances, now used for:
1)	distances to points (because there shouldn't be anything to gain by the new way of doing it)
2)	distances when subgeometries geometries bboxes overlaps
--------------------------------------------------------------------------------------------------------------*/

/**

point to point calculation
*/
int
rt_dist2d_point_point(RTPOINT *point1, RTPOINT *point2, DISTPTS *dl)
{
	const POINT2D *p1, *p2;

	p1 = getPoint2d_cp(point1->point, 0);
	p2 = getPoint2d_cp(point2->point, 0);

	return rt_dist2d_pt_pt(p1, p2, dl);
}
/**

point to line calculation
*/
int
rt_dist2d_point_line(RTPOINT *point, RTLINE *line, DISTPTS *dl)
{
	const POINT2D *p;
	RTDEBUG(2, "rt_dist2d_point_line is called");
	p = getPoint2d_cp(point->point, 0);
	return rt_dist2d_pt_ptarray(p, line->points, dl);
}

int
rt_dist2d_point_circstring(RTPOINT *point, RTCIRCSTRING *circ, DISTPTS *dl)
{
	const POINT2D *p;
	p = getPoint2d_cp(point->point, 0);
	return rt_dist2d_pt_ptarrayarc(p, circ->points, dl);
}

/**
 * 1. see if pt in outer boundary. if no, then treat the outer ring like a line
 * 2. if in the boundary, test to see if its in a hole.
 *    if so, then return dist to hole, else return 0 (point in polygon)
 */
int
rt_dist2d_point_poly(RTPOINT *point, RTPOLY *poly, DISTPTS *dl)
{
	const POINT2D *p;
	int i;

	RTDEBUG(2, "rt_dist2d_point_poly called");

	p = getPoint2d_cp(point->point, 0);

	if (dl->mode == DIST_MAX)
	{
		RTDEBUG(3, "looking for maxdistance");
		return rt_dist2d_pt_ptarray(p, poly->rings[0], dl);
	}
	/* Return distance to outer ring if not inside it */
	if ( ptarray_contains_point(poly->rings[0], p) == RT_OUTSIDE )	
	{
		RTDEBUG(3, "first point not inside outer-ring");
		return rt_dist2d_pt_ptarray(p, poly->rings[0], dl);
	}

	/*
	 * Inside the outer ring.
	 * Scan though each of the inner rings looking to
	 * see if its inside.  If not, distance==0.
	 * Otherwise, distance = pt to ring distance
	 */
	for ( i = 1;  i < poly->nrings; i++)
	{
		/* Inside a hole. Distance = pt -> ring */
		if ( ptarray_contains_point(poly->rings[i], p) != RT_OUTSIDE )
		{
			RTDEBUG(3, " inside an hole");
			return rt_dist2d_pt_ptarray(p, poly->rings[i], dl);
		}
	}

	RTDEBUG(3, " inside the polygon");
	if (dl->mode == DIST_MIN)
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = p->x;
		dl->p1.y = dl->p2.y = p->y;
	}
	return RT_TRUE; /* Is inside the polygon */
}

int
rt_dist2d_point_curvepoly(RTPOINT *point, RTCURVEPOLY *poly, DISTPTS *dl)
{
	const POINT2D *p;
	int i;

	p = getPoint2d_cp(point->point, 0);

	if (dl->mode == DIST_MAX)
		rterror("rt_dist2d_point_curvepoly cannot calculate max distance");

	/* Return distance to outer ring if not inside it */
	if ( rtgeom_contains_point(poly->rings[0], p) == RT_OUTSIDE )	
	{
		return rt_dist2d_recursive((RTGEOM*)point, poly->rings[0], dl);
	}

	/*
	 * Inside the outer ring.
	 * Scan though each of the inner rings looking to
	 * see if its inside.  If not, distance==0.
	 * Otherwise, distance = pt to ring distance
	 */
	for ( i = 1;  i < poly->nrings; i++)
	{
		/* Inside a hole. Distance = pt -> ring */
		if ( rtgeom_contains_point(poly->rings[i], p) != RT_OUTSIDE )
		{
			RTDEBUG(3, " inside a hole");
			return rt_dist2d_recursive((RTGEOM*)point, poly->rings[i], dl);
		}
	}

	RTDEBUG(3, " inside the polygon");
	if (dl->mode == DIST_MIN)
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = p->x;
		dl->p1.y = dl->p2.y = p->y;
	}

	return RT_TRUE; /* Is inside the polygon */
}

/**

line to line calculation
*/
int
rt_dist2d_line_line(RTLINE *line1, RTLINE *line2, DISTPTS *dl)
{
	POINTARRAY *pa1 = line1->points;
	POINTARRAY *pa2 = line2->points;
	RTDEBUG(2, "rt_dist2d_line_line is called");
	return rt_dist2d_ptarray_ptarray(pa1, pa2, dl);
}

int
rt_dist2d_line_circstring(RTLINE *line1, RTCIRCSTRING *line2, DISTPTS *dl)
{
	return rt_dist2d_ptarray_ptarrayarc(line1->points, line2->points, dl);
}

/**
 * line to polygon calculation
 * Brute force.
 * Test line-ring distance against each ring.
 * If there's an intersection (distance==0) then return 0 (crosses boundary).
 * Otherwise, test to see if any point is inside outer rings of polygon,
 * but not in inner rings.
 * If so, return 0  (line inside polygon),
 * otherwise return min distance to a ring (could be outside
 * polygon or inside a hole)
 */
int
rt_dist2d_line_poly(RTLINE *line, RTPOLY *poly, DISTPTS *dl)
{
	const POINT2D *pt;
	int i;

	RTDEBUGF(2, "rt_dist2d_line_poly called (%d rings)", poly->nrings);

	pt = getPoint2d_cp(line->points, 0);
	if ( ptarray_contains_point(poly->rings[0], pt) == RT_OUTSIDE )
	{
		return rt_dist2d_ptarray_ptarray(line->points, poly->rings[0], dl);
	}

	for (i=1; i<poly->nrings; i++)
	{
		if (!rt_dist2d_ptarray_ptarray(line->points, poly->rings[i], dl)) return RT_FALSE;

		RTDEBUGF(3, " distance from ring %d: %f, mindist: %f",
		         i, dl->distance, dl->tolerance);
		/* just a check if  the answer is already given */
		if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; 
	}

	/*
	 * No intersection, have to check if a point is
	 * inside polygon
	 */
	pt = getPoint2d_cp(line->points, 0);

	/*
	 * Outside outer ring, so min distance to a ring
	 * is the actual min distance

	if ( ! pt_in_ring_2d(&pt, poly->rings[0]) )
	{
		return ;
	} */

	/*
	 * Its in the outer ring.
	 * Have to check if its inside a hole
	 */
	for (i=1; i<poly->nrings; i++)
	{
		if ( ptarray_contains_point(poly->rings[i], pt) != RT_OUTSIDE )
		{
			/*
			 * Its inside a hole, then the actual
			 * distance is the min ring distance
			 */
			return RT_TRUE;
		}
	}
	if (dl->mode == DIST_MIN)
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = pt->x;
		dl->p1.y = dl->p2.y = pt->y;
	}
	return RT_TRUE; /* Not in hole, so inside polygon */
}

int
rt_dist2d_line_curvepoly(RTLINE *line, RTCURVEPOLY *poly, DISTPTS *dl)
{
	const POINT2D *pt = getPoint2d_cp(line->points, 0);
	int i;

	if ( rtgeom_contains_point(poly->rings[0], pt) == RT_OUTSIDE )
	{
		return rt_dist2d_recursive((RTGEOM*)line, poly->rings[0], dl);
	}

	for ( i = 1; i < poly->nrings; i++ )
	{
		if ( ! rt_dist2d_recursive((RTGEOM*)line, poly->rings[i], dl) )
			return RT_FALSE;

		if ( dl->distance<=dl->tolerance && dl->mode == DIST_MIN ) 
			return RT_TRUE; 
	}

	for ( i=1; i < poly->nrings; i++ )
	{
		if ( rtgeom_contains_point(poly->rings[i],pt) != RT_OUTSIDE )
		{
			/* Its inside a hole, then the actual */
			return RT_TRUE;
		}	
	}

	if (dl->mode == DIST_MIN)
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = pt->x;
		dl->p1.y = dl->p2.y = pt->y;
	}

	return RT_TRUE; /* Not in hole, so inside polygon */
}

/**
Function handling polygon to polygon calculation
1	if we are looking for maxdistance, just check the outer rings.
2	check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
3	check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1
4	check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2
5	If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.
 */
int
rt_dist2d_poly_poly(RTPOLY *poly1, RTPOLY *poly2, DISTPTS *dl)
{

	const POINT2D *pt;
	int i;

	RTDEBUG(2, "rt_dist2d_poly_poly called");

	/*1	if we are looking for maxdistance, just check the outer rings.*/
	if (dl->mode == DIST_MAX)
	{
		return rt_dist2d_ptarray_ptarray(poly1->rings[0], poly2->rings[0], dl);
	}


	/* 2	check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
	here it would be possible to handle the information about wich one is inside wich one and only search for the smaller ones in the bigger ones holes.*/
	pt = getPoint2d_cp(poly1->rings[0], 0);
	if ( ptarray_contains_point(poly2->rings[0], pt) == RT_OUTSIDE )
	{
		pt = getPoint2d_cp(poly2->rings[0], 0);
		if ( ptarray_contains_point(poly1->rings[0], pt) == RT_OUTSIDE )
		{
			return rt_dist2d_ptarray_ptarray(poly1->rings[0], poly2->rings[0], dl);
		}
	}

	/*3	check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1*/
	pt = getPoint2d_cp(poly2->rings[0], 0);
	for (i=1; i<poly1->nrings; i++)
	{
		/* Inside a hole */
		if ( ptarray_contains_point(poly1->rings[i], pt) != RT_OUTSIDE )
		{
			return rt_dist2d_ptarray_ptarray(poly1->rings[i], poly2->rings[0], dl);
		}
	}

	/*4	check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2*/
	pt = getPoint2d_cp(poly1->rings[0], 0);
	for (i=1; i<poly2->nrings; i++)
	{
		/* Inside a hole */
		if ( ptarray_contains_point(poly2->rings[i], pt) != RT_OUTSIDE )
		{
			return rt_dist2d_ptarray_ptarray(poly1->rings[0], poly2->rings[i], dl);
		}
	}


	/*5	If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.*/
	pt = getPoint2d_cp(poly1->rings[0], 0);
	if ( ptarray_contains_point(poly2->rings[0], pt) != RT_OUTSIDE )
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = pt->x;
		dl->p1.y = dl->p2.y = pt->y;
		return RT_TRUE;
	}

	pt = getPoint2d_cp(poly2->rings[0], 0);
	if ( ptarray_contains_point(poly1->rings[0], pt) != RT_OUTSIDE )
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = pt->x;
		dl->p1.y = dl->p2.y = pt->y;
		return RT_TRUE;
	}


	rterror("Unspecified error in function rt_dist2d_poly_poly");
	return RT_FALSE;
}

int
rt_dist2d_poly_curvepoly(RTPOLY *poly1, RTCURVEPOLY *curvepoly2, DISTPTS *dl)
{
	RTCURVEPOLY *curvepoly1 = rtcurvepoly_construct_from_rtpoly(poly1);
	int rv = rt_dist2d_curvepoly_curvepoly(curvepoly1, curvepoly2, dl);
	rtgeom_free((RTGEOM*)curvepoly1);
	return rv;
}

int
rt_dist2d_circstring_poly(RTCIRCSTRING *circ, RTPOLY *poly, DISTPTS *dl)
{
	RTCURVEPOLY *curvepoly = rtcurvepoly_construct_from_rtpoly(poly);
	int rv = rt_dist2d_line_curvepoly((RTLINE*)circ, curvepoly, dl);
	rtgeom_free((RTGEOM*)curvepoly);
	return rv;
}


int
rt_dist2d_circstring_curvepoly(RTCIRCSTRING *circ, RTCURVEPOLY *poly, DISTPTS *dl)
{
	return rt_dist2d_line_curvepoly((RTLINE*)circ, poly, dl);
}

int
rt_dist2d_circstring_circstring(RTCIRCSTRING *line1, RTCIRCSTRING *line2, DISTPTS *dl)
{
	return rt_dist2d_ptarrayarc_ptarrayarc(line1->points, line2->points, dl);
}

static const POINT2D *
rt_curvering_getfirstpoint2d_cp(RTGEOM *geom)
{
	switch( geom->type )
	{
		case LINETYPE:
			return getPoint2d_cp(((RTLINE*)geom)->points, 0);
		case CIRCSTRINGTYPE:
			return getPoint2d_cp(((RTCIRCSTRING*)geom)->points, 0);
		case COMPOUNDTYPE:
		{
			RTCOMPOUND *comp = (RTCOMPOUND*)geom;
			RTLINE *line = (RTLINE*)(comp->geoms[0]);
			return getPoint2d_cp(line->points, 0);			
		}
		default:
			rterror("rt_curvering_getfirstpoint2d_cp: unknown type");
	}
	return NULL;
}

int
rt_dist2d_curvepoly_curvepoly(RTCURVEPOLY *poly1, RTCURVEPOLY *poly2, DISTPTS *dl)
{
	const POINT2D *pt;
	int i;

	RTDEBUG(2, "rt_dist2d_curvepoly_curvepoly called");

	/*1	if we are looking for maxdistance, just check the outer rings.*/
	if (dl->mode == DIST_MAX)
	{
		return rt_dist2d_recursive(poly1->rings[0],	poly2->rings[0], dl);
	}


	/* 2	check if poly1 has first point outside poly2 and vice versa, if so, just check outer rings
	here it would be possible to handle the information about wich one is inside wich one and only search for the smaller ones in the bigger ones holes.*/
	pt = rt_curvering_getfirstpoint2d_cp(poly1->rings[0]);
	if ( rtgeom_contains_point(poly2->rings[0], pt) == RT_OUTSIDE )
	{
		pt = rt_curvering_getfirstpoint2d_cp(poly2->rings[0]);
		if ( rtgeom_contains_point(poly1->rings[0], pt) == RT_OUTSIDE )
		{
			return rt_dist2d_recursive(poly1->rings[0], poly2->rings[0], dl);
		}
	}

	/*3	check if first point of poly2 is in a hole of poly1. If so check outer ring of poly2 against that hole of poly1*/
	pt = rt_curvering_getfirstpoint2d_cp(poly2->rings[0]);
	for (i = 1; i < poly1->nrings; i++)
	{
		/* Inside a hole */
		if ( rtgeom_contains_point(poly1->rings[i], pt) != RT_OUTSIDE )
		{
			return rt_dist2d_recursive(poly1->rings[i], poly2->rings[0], dl);
		}
	}

	/*4	check if first point of poly1 is in a hole of poly2. If so check outer ring of poly1 against that hole of poly2*/
	pt = rt_curvering_getfirstpoint2d_cp(poly1->rings[0]);
	for (i = 1; i < poly2->nrings; i++)
	{
		/* Inside a hole */
		if ( rtgeom_contains_point(poly2->rings[i], pt) != RT_OUTSIDE )
		{
			return rt_dist2d_recursive(poly1->rings[0],	poly2->rings[i], dl);
		}
	}


	/*5	If we have come all the way here we know that the first point of one of them is inside the other ones outer ring and not in holes so we check wich one is inside.*/
	pt = rt_curvering_getfirstpoint2d_cp(poly1->rings[0]);
	if ( rtgeom_contains_point(poly2->rings[0], pt) != RT_OUTSIDE )
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = pt->x;
		dl->p1.y = dl->p2.y = pt->y;
		return RT_TRUE;
	}

	pt = rt_curvering_getfirstpoint2d_cp(poly2->rings[0]);
	if ( rtgeom_contains_point(poly1->rings[0], pt) != RT_OUTSIDE )
	{
		dl->distance = 0.0;
		dl->p1.x = dl->p2.x = pt->x;
		dl->p1.y = dl->p2.y = pt->y;
		return RT_TRUE;
	}

	rterror("Unspecified error in function rt_dist2d_curvepoly_curvepoly");
	return RT_FALSE;
}



/**
 * search all the segments of pointarray to see which one is closest to p1
 * Returns minimum distance between point and pointarray
 */
int
rt_dist2d_pt_ptarray(const POINT2D *p, POINTARRAY *pa,DISTPTS *dl)
{
	int t;
	const POINT2D *start, *end;
	int twist = dl->twisted;

	RTDEBUG(2, "rt_dist2d_pt_ptarray is called");

	start = getPoint2d_cp(pa, 0);

	if ( !rt_dist2d_pt_pt(p, start, dl) ) return RT_FALSE;

	for (t=1; t<pa->npoints; t++)
	{
		dl->twisted=twist;
		end = getPoint2d_cp(pa, t);
		if (!rt_dist2d_pt_seg(p, start, end, dl)) return RT_FALSE;

		if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
		start = end;
	}

	return RT_TRUE;
}

/**
* Search all the arcs of pointarray to see which one is closest to p1
* Returns minimum distance between point and arc pointarray.
*/
int
rt_dist2d_pt_ptarrayarc(const POINT2D *p, const POINTARRAY *pa, DISTPTS *dl)
{
	int t;
	const POINT2D *A1;
	const POINT2D *A2;
	const POINT2D *A3;
	int twist = dl->twisted;

	RTDEBUG(2, "rt_dist2d_pt_ptarrayarc is called");

	if ( pa->npoints % 2 == 0 || pa->npoints < 3 )
	{
		rterror("rt_dist2d_pt_ptarrayarc called with non-arc input");
		return RT_FALSE;
	}

	if (dl->mode == DIST_MAX)
	{
		rterror("rt_dist2d_pt_ptarrayarc does not currently support DIST_MAX mode");
		return RT_FALSE;
	}

	A1 = getPoint2d_cp(pa, 0);

	if ( ! rt_dist2d_pt_pt(p, A1, dl) ) 
		return RT_FALSE;

	for ( t=1; t<pa->npoints; t += 2 )
	{
		dl->twisted = twist;
		A2 = getPoint2d_cp(pa, t);
		A3 = getPoint2d_cp(pa, t+1);
		
		if ( rt_dist2d_pt_arc(p, A1, A2, A3, dl) == RT_FALSE ) 
			return RT_FALSE;

		if ( dl->distance <= dl->tolerance && dl->mode == DIST_MIN ) 
			return RT_TRUE; /*just a check if  the answer is already given*/
			
		A1 = A3;
	}

	return RT_TRUE;
}




/**
* test each segment of l1 against each segment of l2.
*/
int
rt_dist2d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2,DISTPTS *dl)
{
	int t,u;
	const POINT2D	*start, *end;
	const POINT2D	*start2, *end2;
	int twist = dl->twisted;

	RTDEBUGF(2, "rt_dist2d_ptarray_ptarray called (points: %d-%d)",l1->npoints, l2->npoints);

	if (dl->mode == DIST_MAX)/*If we are searching for maxdistance we go straight to point-point calculation since the maxdistance have to be between two vertexes*/
	{
		for (t=0; t<l1->npoints; t++) /*for each segment in L1 */
		{
			start = getPoint2d_cp(l1, t);
			for (u=0; u<l2->npoints; u++) /*for each segment in L2 */
			{
				start2 = getPoint2d_cp(l2, u);
				rt_dist2d_pt_pt(start, start2, dl);
				RTDEBUGF(4, "maxdist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
				RTDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f",
				         t, u, dl->distance, dl->tolerance);
			}
		}
	}
	else
	{
		start = getPoint2d_cp(l1, 0);
		for (t=1; t<l1->npoints; t++) /*for each segment in L1 */
		{
			end = getPoint2d_cp(l1, t);
			start2 = getPoint2d_cp(l2, 0);
			for (u=1; u<l2->npoints; u++) /*for each segment in L2 */
			{
				end2 = getPoint2d_cp(l2, u);
				dl->twisted=twist;
				rt_dist2d_seg_seg(start, end, start2, end2, dl);
				RTDEBUGF(4, "mindist_ptarray_ptarray; seg %i * seg %i, dist = %g\n",t,u,dl->distance);
				RTDEBUGF(3, " seg%d-seg%d dist: %f, mindist: %f",
				         t, u, dl->distance, dl->tolerance);
				if (dl->distance<=dl->tolerance && dl->mode == DIST_MIN) return RT_TRUE; /*just a check if  the answer is already given*/
				start2 = end2;
			}
			start = end;
		}
	}
	return RT_TRUE;
}

/**
* Test each segment of pa against each arc of pb for distance.
*/
int
rt_dist2d_ptarray_ptarrayarc(const POINTARRAY *pa, const POINTARRAY *pb, DISTPTS *dl)
{
	int t, u;
	const POINT2D *A1;
	const POINT2D *A2;
	const POINT2D *B1;
	const POINT2D *B2;
	const POINT2D *B3;
	int twist = dl->twisted;

	RTDEBUGF(2, "rt_dist2d_ptarray_ptarrayarc called (points: %d-%d)",pa->npoints, pb->npoints);

	if ( pb->npoints % 2 == 0 || pb->npoints < 3 )
	{
		rterror("rt_dist2d_ptarray_ptarrayarc called with non-arc input");
		return RT_FALSE;
	}

	if ( dl->mode == DIST_MAX )
	{
		rterror("rt_dist2d_ptarray_ptarrayarc does not currently support DIST_MAX mode");
		return RT_FALSE;
	}
	else
	{
		A1 = getPoint2d_cp(pa, 0);
		for ( t=1; t < pa->npoints; t++ ) /* For each segment in pa */
		{
			A2 = getPoint2d_cp(pa, t);
			B1 = getPoint2d_cp(pb, 0);
			for ( u=1; u < pb->npoints; u += 2 ) /* For each arc in pb */
			{
				B2 = getPoint2d_cp(pb, u);
				B3 = getPoint2d_cp(pb, u+1);
				dl->twisted = twist;

				rt_dist2d_seg_arc(A1, A2, B1, B2, B3, dl);

				/* If we've found a distance within tolerance, we're done */
				if ( dl->distance <= dl->tolerance && dl->mode == DIST_MIN ) 
					return RT_TRUE; 

				B1 = B3;
			}
			A1 = A2;
		}
	}
	return RT_TRUE;
}

/**
* Test each arc of pa against each arc of pb for distance.
*/
int
rt_dist2d_ptarrayarc_ptarrayarc(const POINTARRAY *pa, const POINTARRAY *pb, DISTPTS *dl)
{
	int t, u;
	const POINT2D *A1;
	const POINT2D *A2;
	const POINT2D *A3;
	const POINT2D *B1;
	const POINT2D *B2;
	const POINT2D *B3;
	int twist = dl->twisted;

	RTDEBUGF(2, "rt_dist2d_ptarrayarc_ptarrayarc called (points: %d-%d)",pa->npoints, pb->npoints);

	if (dl->mode == DIST_MAX)
	{
		rterror("rt_dist2d_ptarrayarc_ptarrayarc does not currently support DIST_MAX mode");
		return RT_FALSE;
	}
	else
	{
		A1 = getPoint2d_cp(pa, 0);
		for ( t=1; t < pa->npoints; t += 2 ) /* For each segment in pa */
		{
			A2 = getPoint2d_cp(pa, t);
			A3 = getPoint2d_cp(pa, t+1);
			B1 = getPoint2d_cp(pb, 0);
			for ( u=1; u < pb->npoints; u += 2 ) /* For each arc in pb */
			{
				B2 = getPoint2d_cp(pb, u);
				B3 = getPoint2d_cp(pb, u+1);
				dl->twisted = twist;

				rt_dist2d_arc_arc(A1, A2, A3, B1, B2, B3, dl);

				/* If we've found a distance within tolerance, we're done */
				if ( dl->distance <= dl->tolerance && dl->mode == DIST_MIN ) 
					return RT_TRUE; 

				B1 = B3;
			}
			A1 = A3;
		}
	}
	return RT_TRUE;
}

/**
* Calculate the shortest distance between an arc and an edge.
* Line/circle approach from http://stackoverflow.com/questions/1073336/circle-line-collision-detection 
*/
int 
rt_dist2d_seg_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *B1, const POINT2D *B2, const POINT2D *B3, DISTPTS *dl)
{
	POINT2D C; /* center of arc circle */
	double radius_C; /* radius of arc circle */
	POINT2D D; /* point on A closest to C */
	double dist_C_D; /* distance from C to D */
	int pt_in_arc, pt_in_seg;
	DISTPTS dltmp;
	
	/* Bail out on crazy modes */
	if ( dl->mode < 0 )
		rterror("rt_dist2d_seg_arc does not support maxdistance mode");

	/* What if the "arc" is a point? */
	if ( rt_arc_is_pt(B1, B2, B3) )
		return rt_dist2d_pt_seg(B1, A1, A2, dl);

	/* Calculate center and radius of the circle. */
	radius_C = rt_arc_center(B1, B2, B3, &C);

	/* This "arc" is actually a line (B2 is colinear with B1,B3) */
	if ( radius_C < 0.0 )
		return rt_dist2d_seg_seg(A1, A2, B1, B3, dl);

	/* Calculate distance between the line and circle center */
	rt_dist2d_distpts_init(&dltmp, DIST_MIN);
	if ( rt_dist2d_pt_seg(&C, A1, A2, &dltmp) == RT_FALSE )
		rterror("rt_dist2d_pt_seg failed in rt_dist2d_seg_arc");

	D = dltmp.p1;
	dist_C_D = dltmp.distance;
	
	/* Line intersects circle, maybe arc intersects edge? */
	/* If so, that's the closest point. */
	/* If not, the closest point is one of the end points of A */
	if ( dist_C_D < radius_C )
	{
		double length_A; /* length of the segment A */
		POINT2D E, F; /* points of interection of edge A and circle(B) */
		double dist_D_EF; /* distance from D to E or F (same distance both ways) */

		dist_D_EF = sqrt(radius_C*radius_C - dist_C_D*dist_C_D);
		length_A = sqrt((A2->x-A1->x)*(A2->x-A1->x)+(A2->y-A1->y)*(A2->y-A1->y));

		/* Point of intersection E */
		E.x = D.x - (A2->x-A1->x) * dist_D_EF / length_A;
		E.y = D.y - (A2->y-A1->y) * dist_D_EF / length_A;
		/* Point of intersection F */
		F.x = D.x + (A2->x-A1->x) * dist_D_EF / length_A;
		F.y = D.y + (A2->y-A1->y) * dist_D_EF / length_A;


		/* If E is within A and within B then it's an interesction point */
		pt_in_arc = rt_pt_in_arc(&E, B1, B2, B3);
		pt_in_seg = rt_pt_in_seg(&E, A1, A2);
		
		if ( pt_in_arc && pt_in_seg )
		{
			dl->distance = 0.0;
			dl->p1 = E;
			dl->p2 = E;
			return RT_TRUE;
		}
		
		/* If F is within A and within B then it's an interesction point */
		pt_in_arc = rt_pt_in_arc(&F, B1, B2, B3);
		pt_in_seg = rt_pt_in_seg(&F, A1, A2);
		
		if ( pt_in_arc && pt_in_seg )
		{
			dl->distance = 0.0;
			dl->p1 = F;
			dl->p2 = F;
			return RT_TRUE;
		}
	}
	
	/* Line grazes circle, maybe arc intersects edge? */
	/* If so, grazing point is the closest point. */
	/* If not, the closest point is one of the end points of A */
	else if ( dist_C_D == radius_C )
	{		
		/* Closest point D is also the point of grazing */
		pt_in_arc = rt_pt_in_arc(&D, B1, B2, B3);
		pt_in_seg = rt_pt_in_seg(&D, A1, A2);

		/* Is D contained in both A and B? */
		if ( pt_in_arc && pt_in_seg )
		{
			dl->distance = 0.0;
			dl->p1 = D;
			dl->p2 = D;
			return RT_TRUE;
		}
	}
	/* Line misses circle. */
	/* If closest point to A on circle is within B, then that's the closest */
	/* Otherwise, the closest point will be an end point of A */
	else
	{
		POINT2D G; /* Point on circle closest to A */
		G.x = C.x + (D.x-C.x) * radius_C / dist_C_D;
		G.y = C.y + (D.y-C.y) * radius_C / dist_C_D;
		
		pt_in_arc = rt_pt_in_arc(&G, B1, B2, B3);
		pt_in_seg = rt_pt_in_seg(&D, A1, A2);
		
		/* Closest point is on the interior of A and B */
		if ( pt_in_arc && pt_in_seg )
			return rt_dist2d_pt_pt(&D, &G, dl);

	}
	
	/* Now we test the many combinations of end points with either */
	/* arcs or edges. Each previous check determined if the closest */
	/* potential point was within the arc/segment inscribed on the */
	/* line/circle holding the arc/segment. */

	/* Closest point is in the arc, but not in the segment, so */
	/* one of the segment end points must be the closest. */
	if ( pt_in_arc & ! pt_in_seg )
	{
		rt_dist2d_pt_arc(A1, B1, B2, B3, dl);
		rt_dist2d_pt_arc(A2, B1, B2, B3, dl);		
		return RT_TRUE;
	}
	/* or, one of the arc end points is the closest */
	else if  ( pt_in_seg && ! pt_in_arc )
	{
		rt_dist2d_pt_seg(B1, A1, A2, dl);
		rt_dist2d_pt_seg(B3, A1, A2, dl);
		return RT_TRUE;			
	}
	/* Finally, one of the end-point to end-point combos is the closest. */
	else
	{
		rt_dist2d_pt_pt(A1, B1, dl);
		rt_dist2d_pt_pt(A1, B3, dl);
		rt_dist2d_pt_pt(A2, B1, dl);
		rt_dist2d_pt_pt(A2, B3, dl);
		return RT_TRUE;
	}
	
	return RT_FALSE;
}

int
rt_dist2d_pt_arc(const POINT2D* P, const POINT2D* A1, const POINT2D* A2, const POINT2D* A3, DISTPTS* dl)
{
	double radius_A, d;
	POINT2D C; /* center of circle defined by arc A */
	POINT2D X; /* point circle(A) where line from C to P crosses */
	
	if ( dl->mode < 0 )
		rterror("rt_dist2d_pt_arc does not support maxdistance mode");

	/* What if the arc is a point? */
	if ( rt_arc_is_pt(A1, A2, A3) )
		return rt_dist2d_pt_pt(P, A1, dl);

	/* Calculate centers and radii of circles. */
	radius_A = rt_arc_center(A1, A2, A3, &C);
	
	/* This "arc" is actually a line (A2 is colinear with A1,A3) */
	if ( radius_A < 0.0 )
		return rt_dist2d_pt_seg(P, A1, A3, dl);
	
	/* Distance from point to center */	
	d = distance2d_pt_pt(&C, P);
	
	/* X is the point on the circle where the line from P to C crosses */
	X.x = C.x + (P->x - C.x) * radius_A / d;
	X.y = C.y + (P->y - C.y) * radius_A / d;

	/* Is crossing point inside the arc? Or arc is actually circle? */
	if ( p2d_same(A1, A3) || rt_pt_in_arc(&X, A1, A2, A3) )
	{
		rt_dist2d_pt_pt(P, &X, dl);
	}
	else 
	{
		/* Distance is the minimum of the distances to the arc end points */
		rt_dist2d_pt_pt(A1, P, dl);
		rt_dist2d_pt_pt(A3, P, dl);
	}
	return RT_TRUE;
}


int
rt_dist2d_arc_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3, 
                  const POINT2D *B1, const POINT2D *B2, const POINT2D *B3,
                  DISTPTS *dl)
{
	POINT2D CA, CB; /* Center points of arcs A and B */
	double radius_A, radius_B, d; /* Radii of arcs A and B */
	POINT2D P; /* Temporary point P */
	POINT2D D; /* Mid-point between the centers CA and CB */
	int pt_in_arc_A, pt_in_arc_B; /* Test whether potential intersection point is within the arc */
	
	if ( dl->mode != DIST_MIN )
		rterror("rt_dist2d_arc_arc only supports mindistance");
	
	/* TODO: Handle case where arc is closed circle (A1 = A3) */
	
	/* What if one or both of our "arcs" is actually a point? */
	if ( rt_arc_is_pt(B1, B2, B3) && rt_arc_is_pt(A1, A2, A3) )
		return rt_dist2d_pt_pt(B1, A1, dl);
	else if ( rt_arc_is_pt(B1, B2, B3) )
		return rt_dist2d_pt_arc(B1, A1, A2, A3, dl);
	else if ( rt_arc_is_pt(A1, A2, A3) )
		return rt_dist2d_pt_arc(A1, B1, B2, B3, dl);
	
	/* Calculate centers and radii of circles. */
	radius_A = rt_arc_center(A1, A2, A3, &CA);
	radius_B = rt_arc_center(B1, B2, B3, &CB);

	/* Two co-linear arcs?!? That's two segments. */
	if ( radius_A < 0 && radius_B < 0 )
		return rt_dist2d_seg_seg(A1, A3, B1, B3, dl);

	/* A is co-linear, delegate to rt_dist_seg_arc here. */
	if ( radius_A < 0 )
		return rt_dist2d_seg_arc(A1, A3, B1, B2, B3, dl);

	/* B is co-linear, delegate to rt_dist_seg_arc here. */
	if ( radius_B < 0 )
		return rt_dist2d_seg_arc(B1, B3, A1, A2, A3, dl);

	/* Make sure that arc "A" has the bigger radius */
	if ( radius_B > radius_A )
	{
		const POINT2D *tmp;
		tmp = B1; B1 = A1; A1 = tmp;
		tmp = B2; B2 = A2; A2 = tmp;
		tmp = B3; B3 = A3; A3 = tmp;
		P = CB; CB = CA; CA = P;
		d = radius_B; radius_B = radius_A; radius_A = d;
	}
	
	/* Center-center distance */
	d = distance2d_pt_pt(&CA, &CB);

	/* Equal circles. Arcs may intersect at multiple points, or at none! */
	if ( FP_EQUALS(d, 0.0) && FP_EQUALS(radius_A, radius_B) )
	{
		rterror("rt_dist2d_arc_arc can't handle cojoint circles, uh oh");
	}
	
	/* Circles touch at a point. Is that point within the arcs? */
	if ( d == (radius_A + radius_B) )
	{
		D.x = CA.x + (CB.x - CA.x) * radius_A / d;
		D.y = CA.y + (CB.y - CA.y) * radius_A / d;
		
		pt_in_arc_A = rt_pt_in_arc(&D, A1, A2, A3);
		pt_in_arc_B = rt_pt_in_arc(&D, B1, B2, B3);
		
		/* Arcs do touch at D, return it */
		if ( pt_in_arc_A && pt_in_arc_B )
		{
			dl->distance = 0.0;
			dl->p1 = D;
			dl->p2 = D;
			return RT_TRUE;
		}
	}
	/* Disjoint or contained circles don't intersect. Closest point may be on */
	/* the line joining CA to CB. */
	else if ( d > (radius_A + radius_B) /* Disjoint */ || d < (radius_A - radius_B) /* Contained */ )
	{
		POINT2D XA, XB; /* Points where the line from CA to CB cross their circle bounds */
		
		/* Calculate hypothetical nearest points, the places on the */
		/* two circles where the center-center line crosses. If both */
		/* arcs contain their hypothetical points, that's the crossing distance */
		XA.x = CA.x + (CB.x - CA.x) * radius_A / d;
		XA.y = CA.y + (CB.y - CA.y) * radius_A / d;
		XB.x = CB.x + (CA.x - CB.x) * radius_B / d;
		XB.y = CB.y + (CA.y - CB.y) * radius_B / d;
		
		pt_in_arc_A = rt_pt_in_arc(&XA, A1, A2, A3);
		pt_in_arc_B = rt_pt_in_arc(&XB, B1, B2, B3);
		
		/* If the nearest points are both within the arcs, that's our answer */
		/* the shortest distance is at the nearest points */
		if ( pt_in_arc_A && pt_in_arc_B )
		{
			return rt_dist2d_pt_pt(&XA, &XB, dl);
		}
	}
	/* Circles cross at two points, are either of those points in both arcs? */
	/* http://paulbourke.net/geometry/2circle/ */
	else if ( d < (radius_A + radius_B) )
	{
		POINT2D E, F; /* Points where circle(A) and circle(B) cross */
		/* Distance from CA to D */
		double a = (radius_A*radius_A - radius_B*radius_B + d*d) / (2*d);
		/* Distance from D to E or F */
		double h = sqrt(radius_A*radius_A - a*a);
		
		/* Location of D */
		D.x = CA.x + (CB.x - CA.x) * a / d;
		D.y = CA.y + (CB.y - CA.y) * a / d;
		
		/* Start from D and project h units perpendicular to CA-D to get E */
		E.x = D.x + (D.y - CA.y) * h / a;
		E.y = D.y + (D.x - CA.x) * h / a;

		/* Crossing point E contained in arcs? */
		pt_in_arc_A = rt_pt_in_arc(&E, A1, A2, A3);
		pt_in_arc_B = rt_pt_in_arc(&E, B1, B2, B3);

		if ( pt_in_arc_A && pt_in_arc_B ) 
		{
			dl->p1 = dl->p2 = E;
			dl->distance = 0.0;
			return RT_TRUE;
		}

		/* Start from D and project h units perpendicular to CA-D to get F */
		F.x = D.x - (D.y - CA.y) * h / a;
		F.y = D.y - (D.x - CA.x) * h / a;
		
		/* Crossing point F contained in arcs? */
		pt_in_arc_A = rt_pt_in_arc(&F, A1, A2, A3);
		pt_in_arc_B = rt_pt_in_arc(&F, B1, B2, B3);

		if ( pt_in_arc_A && pt_in_arc_B ) 
		{
			dl->p1 = dl->p2 = F;
			dl->distance = 0.0;
			return RT_TRUE;
		}
	} 
	else
	{
		rterror("rt_dist2d_arc_arc: arcs neither touch, intersect nor are disjoint! INCONCEIVABLE!");
		return RT_FALSE;
	}

	/* Closest point is in the arc A, but not in the arc B, so */
	/* one of the B end points must be the closest. */
	if ( pt_in_arc_A & ! pt_in_arc_B )
	{
		rt_dist2d_pt_arc(B1, A1, A2, A3, dl);
		rt_dist2d_pt_arc(B3, A1, A2, A3, dl);
		return RT_TRUE;
	}
	/* Closest point is in the arc B, but not in the arc A, so */
	/* one of the A end points must be the closest. */
	else if  ( pt_in_arc_B && ! pt_in_arc_A )
	{
		rt_dist2d_pt_arc(A1, B1, B2, B3, dl);
		rt_dist2d_pt_arc(A3, B1, B2, B3, dl);		
		return RT_TRUE;			
	}
	/* Finally, one of the end-point to end-point combos is the closest. */
	else
	{
		rt_dist2d_pt_pt(A1, B1, dl);
		rt_dist2d_pt_pt(A1, B3, dl);
		rt_dist2d_pt_pt(A2, B1, dl);
		rt_dist2d_pt_pt(A2, B3, dl);
		return RT_TRUE;
	}	

	return RT_TRUE;
}

/**
Finds the shortest distance between two segments.
This function is changed so it is not doing any comparasion of distance
but just sending every possible combination further to rt_dist2d_pt_seg
*/
int
rt_dist2d_seg_seg(const POINT2D *A, const POINT2D *B, const POINT2D *C, const POINT2D *D, DISTPTS *dl)
{
	double	s_top, s_bot,s;
	double	r_top, r_bot,r;

	RTDEBUGF(2, "rt_dist2d_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
	         A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);

	/*A and B are the same point */
	if (  ( A->x == B->x) && (A->y == B->y) )
	{
		return rt_dist2d_pt_seg(A,C,D,dl);
	}
	/*U and V are the same point */

	if (  ( C->x == D->x) && (C->y == D->y) )
	{
		dl->twisted= ((dl->twisted) * (-1));
		return rt_dist2d_pt_seg(D,A,B,dl);
	}
	/* AB and CD are line segments */
	/* from comp.graphics.algo

	Solving the above for r and s yields
				(Ay-Cy)(Dx-Cx)-(Ax-Cx)(Dy-Cy)
	           r = ----------------------------- (eqn 1)
				(Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)

		 	(Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
		s = ----------------------------- (eqn 2)
			(Bx-Ax)(Dy-Cy)-(By-Ay)(Dx-Cx)
	Let P be the position vector of the intersection point, then
		P=A+r(B-A) or
		Px=Ax+r(Bx-Ax)
		Py=Ay+r(By-Ay)
	By examining the values of r & s, you can also determine some other limiting conditions:
		If 0<=r<=1 & 0<=s<=1, intersection exists
		r<0 or r>1 or s<0 or s>1 line segments do not intersect
		If the denominator in eqn 1 is zero, AB & CD are parallel
		If the numerator in eqn 1 is also zero, AB & CD are collinear.

	*/
	r_top = (A->y-C->y)*(D->x-C->x) - (A->x-C->x)*(D->y-C->y);
	r_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x);

	s_top = (A->y-C->y)*(B->x-A->x) - (A->x-C->x)*(B->y-A->y);
	s_bot = (B->x-A->x)*(D->y-C->y) - (B->y-A->y)*(D->x-C->x);

	if  ( (r_bot==0) || (s_bot == 0) )
	{
		if ((rt_dist2d_pt_seg(A,C,D,dl)) && (rt_dist2d_pt_seg(B,C,D,dl)))
		{
			dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
			return ((rt_dist2d_pt_seg(C,A,B,dl)) && (rt_dist2d_pt_seg(D,A,B,dl))); /*if all is successful we return true*/
		}
		else
		{
			return RT_FALSE; /* if any of the calls to rt_dist2d_pt_seg goes wrong we return false*/
		}
	}

	s = s_top/s_bot;
	r=  r_top/r_bot;

	if (((r<0) || (r>1) || (s<0) || (s>1)) || (dl->mode == DIST_MAX))
	{
		if ((rt_dist2d_pt_seg(A,C,D,dl)) && (rt_dist2d_pt_seg(B,C,D,dl)))
		{
			dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
			return ((rt_dist2d_pt_seg(C,A,B,dl)) && (rt_dist2d_pt_seg(D,A,B,dl))); /*if all is successful we return true*/
		}
		else
		{
			return RT_FALSE; /* if any of the calls to rt_dist2d_pt_seg goes wrong we return false*/
		}
	}
	else
	{
		if (dl->mode == DIST_MIN)	/*If there is intersection we identify the intersection point and return it but only if we are looking for mindistance*/
		{
			POINT2D theP;

			if (((A->x==C->x)&&(A->y==C->y))||((A->x==D->x)&&(A->y==D->y)))
			{
				theP.x = A->x;
				theP.y = A->y;
			}
			else if (((B->x==C->x)&&(B->y==C->y))||((B->x==D->x)&&(B->y==D->y)))
			{
				theP.x = B->x;
				theP.y = B->y;
			}
			else
			{
				theP.x = A->x+r*(B->x-A->x);
				theP.y = A->y+r*(B->y-A->y);
			}
			dl->distance=0.0;
			dl->p1=theP;
			dl->p2=theP;
		}
		return RT_TRUE;

	}
	rterror("unspecified error in function rt_dist2d_seg_seg");
	return RT_FALSE; /*If we have come here something is wrong*/
}


/*------------------------------------------------------------------------------------------------------------
End of Brute force functions
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
New faster distance calculations
--------------------------------------------------------------------------------------------------------------*/

/**

The new faster calculation comparing pointarray to another pointarray
the arrays can come from both polygons and linestrings.
The naming is not good but comes from that it compares a
chosen selection of the points not all of them
*/
int
rt_dist2d_fast_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2,DISTPTS *dl, GBOX *box1, GBOX *box2)
{
	/*here we define two lists to hold our calculated "z"-values and the order number in the geometry*/

	double k, thevalue;
	float	deltaX, deltaY, c1m, c2m;
	POINT2D	c1, c2;
	const POINT2D *theP;
	float min1X, max1X, max1Y, min1Y,min2X, max2X, max2Y, min2Y;
	int t;
	int n1 = l1->npoints;
	int n2 = l2->npoints;
	
	LISTSTRUCT *list1, *list2;
	list1 = (LISTSTRUCT*)rtalloc(sizeof(LISTSTRUCT)*n1); 
	list2 = (LISTSTRUCT*)rtalloc(sizeof(LISTSTRUCT)*n2);
	
	RTDEBUG(2, "rt_dist2d_fast_ptarray_ptarray is called");

	max1X = box1->xmax;
	min1X = box1->xmin;
	max1Y = box1->ymax;
	min1Y = box1->ymin;
	max2X = box2->xmax;
	min2X = box2->xmin;
	max2Y = box2->ymax;
	min2Y = box2->ymin;
	/*we want the center of the bboxes, and calculate the slope between the centerpoints*/
	c1.x = min1X + (max1X-min1X)/2;
	c1.y = min1Y + (max1Y-min1Y)/2;
	c2.x = min2X + (max2X-min2X)/2;
	c2.y = min2Y + (max2Y-min2Y)/2;

	deltaX=(c2.x-c1.x);
	deltaY=(c2.y-c1.y);


	/*Here we calculate where the line perpendicular to the center-center line crosses the axes for each vertex
	if the center-center line is vertical the perpendicular line will be horizontal and we find it's crossing the Y-axes with z = y-kx */
	if ((deltaX*deltaX)<(deltaY*deltaY))        /*North or South*/
	{
		k = -deltaX/deltaY;
		for (t=0; t<n1; t++) /*for each segment in L1 */
		{
			theP = getPoint2d_cp(l1, t);
			thevalue = theP->y - (k * theP->x);
			list1[t].themeasure=thevalue;
			list1[t].pnr=t;

		}
		for (t=0; t<n2; t++) /*for each segment in L2*/
		{
			theP = getPoint2d_cp(l2, t);
			thevalue = theP->y - (k * theP->x);
			list2[t].themeasure=thevalue;
			list2[t].pnr=t;

		}
		c1m = c1.y-(k*c1.x);
		c2m = c2.y-(k*c2.x);
	}


	/*if the center-center line is horizontal the perpendicular line will be vertical. To eliminate problems with deviding by zero we are here mirroring the coordinate-system
	 and we find it's crossing the X-axes with z = x-(1/k)y */
	else        /*West or East*/
	{
		k = -deltaY/deltaX;
		for (t=0; t<n1; t++) /*for each segment in L1 */
		{
			theP = getPoint2d_cp(l1, t);
			thevalue = theP->x - (k * theP->y);
			list1[t].themeasure=thevalue;
			list1[t].pnr=t;
			/* rtnotice("l1 %d, measure=%f",t,thevalue ); */
		}
		for (t=0; t<n2; t++) /*for each segment in L2*/
		{
			theP = getPoint2d_cp(l2, t);
			thevalue = theP->x - (k * theP->y);
			list2[t].themeasure=thevalue;
			list2[t].pnr=t;
			/* rtnotice("l2 %d, measure=%f",t,thevalue ); */
		}
		c1m = c1.x-(k*c1.y);
		c2m = c2.x-(k*c2.y);
	}

	/*we sort our lists by the calculated values*/
	qsort(list1, n1, sizeof(LISTSTRUCT), struct_cmp_by_measure);
	qsort(list2, n2, sizeof(LISTSTRUCT), struct_cmp_by_measure);

	if (c1m < c2m)
	{
		if (!rt_dist2d_pre_seg_seg(l1,l2,list1,list2,k,dl)) 
		{
			rtfree(list1);
			rtfree(list2);
			return RT_FALSE;
		}
	}
	else
	{
		dl->twisted= ((dl->twisted) * (-1));
		if (!rt_dist2d_pre_seg_seg(l2,l1,list2,list1,k,dl)) 
		{
			rtfree(list1);
			rtfree(list2);
			return RT_FALSE;
		}
	}
	rtfree(list1);
	rtfree(list2);	
	return RT_TRUE;
}

int
struct_cmp_by_measure(const void *a, const void *b)
{
	LISTSTRUCT *ia = (LISTSTRUCT*)a;
	LISTSTRUCT *ib = (LISTSTRUCT*)b;
	return ( ia->themeasure>ib->themeasure ) ? 1 : -1;
}

/**
	preparation before rt_dist2d_seg_seg.
*/
int
rt_dist2d_pre_seg_seg(POINTARRAY *l1, POINTARRAY *l2,LISTSTRUCT *list1, LISTSTRUCT *list2,double k, DISTPTS *dl)
{
	const POINT2D *p1, *p2, *p3, *p4, *p01, *p02;
	int pnr1,pnr2,pnr3,pnr4, n1, n2, i, u, r, twist;
	double maxmeasure;
	n1=	l1->npoints;
	n2 = l2->npoints;

	RTDEBUG(2, "rt_dist2d_pre_seg_seg is called");

	p1 = getPoint2d_cp(l1, list1[0].pnr);
	p3 = getPoint2d_cp(l2, list2[0].pnr);
	rt_dist2d_pt_pt(p1, p3, dl);
	maxmeasure = sqrt(dl->distance*dl->distance + (dl->distance*dl->distance*k*k));
	twist = dl->twisted; /*to keep the incomming order between iterations*/
	for (i =(n1-1); i>=0; --i)
	{
		/*we break this iteration when we have checked every
		point closer to our perpendicular "checkline" than
		our shortest found distance*/
		if (((list2[0].themeasure-list1[i].themeasure)) > maxmeasure) break;
		for (r=-1; r<=1; r +=2) /*because we are not iterating in the original pointorder we have to check the segment before and after every point*/
		{
			pnr1 = list1[i].pnr;
			p1 = getPoint2d_cp(l1, pnr1);
			if (pnr1+r<0)
			{
				p01 = getPoint2d_cp(l1, (n1-1));
				if (( p1->x == p01->x) && (p1->y == p01->y)) pnr2 = (n1-1);
				else pnr2 = pnr1; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
			}

			else if (pnr1+r>(n1-1))
			{
				p01 = getPoint2d_cp(l1, 0);
				if (( p1->x == p01->x) && (p1->y == p01->y)) pnr2 = 0;
				else pnr2 = pnr1; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
			}
			else pnr2 = pnr1+r;


			p2 = getPoint2d_cp(l1, pnr2);
			for (u=0; u<n2; ++u)
			{
				if (((list2[u].themeasure-list1[i].themeasure)) >= maxmeasure) break;
				pnr3 = list2[u].pnr;
				p3 = getPoint2d_cp(l2, pnr3);
				if (pnr3==0)
				{
					p02 = getPoint2d_cp(l2, (n2-1));
					if (( p3->x == p02->x) && (p3->y == p02->y)) pnr4 = (n2-1);
					else pnr4 = pnr3; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
				}
				else pnr4 = pnr3-1;

				p4 = getPoint2d_cp(l2, pnr4);
				dl->twisted=twist;
				if (!rt_dist2d_selected_seg_seg(p1, p2, p3, p4, dl)) return RT_FALSE;

				if (pnr3>=(n2-1))
				{
					p02 = getPoint2d_cp(l2, 0);
					if (( p3->x == p02->x) && (p3->y == p02->y)) pnr4 = 0;
					else pnr4 = pnr3; /* if it is a line and the last and first point is not the same we avoid the edge between start and end this way*/
				}

				else pnr4 = pnr3+1;

				p4 = getPoint2d_cp(l2, pnr4);
				dl->twisted=twist; /*we reset the "twist" for each iteration*/
				if (!rt_dist2d_selected_seg_seg(p1, p2, p3, p4, dl)) return RT_FALSE;

				maxmeasure = sqrt(dl->distance*dl->distance + (dl->distance*dl->distance*k*k));/*here we "translate" the found mindistance so it can be compared to our "z"-values*/
			}
		}
	}

	return RT_TRUE;
}


/**
	This is the same function as rt_dist2d_seg_seg but
	without any calculations to determine intersection since we
	already know they do not intersect
*/
int
rt_dist2d_selected_seg_seg(const POINT2D *A, const POINT2D *B, const POINT2D *C, const POINT2D *D, DISTPTS *dl)
{
	RTDEBUGF(2, "rt_dist2d_selected_seg_seg [%g,%g]->[%g,%g] by [%g,%g]->[%g,%g]",
	         A->x,A->y,B->x,B->y, C->x,C->y, D->x, D->y);

	/*A and B are the same point */
	if (  ( A->x == B->x) && (A->y == B->y) )
	{
		return rt_dist2d_pt_seg(A,C,D,dl);
	}
	/*U and V are the same point */

	if (  ( C->x == D->x) && (C->y == D->y) )
	{
		dl->twisted= ((dl->twisted) * (-1));
		return rt_dist2d_pt_seg(D,A,B,dl);
	}

	if ((rt_dist2d_pt_seg(A,C,D,dl)) && (rt_dist2d_pt_seg(B,C,D,dl)))
	{
		dl->twisted= ((dl->twisted) * (-1));  /*here we change the order of inputted geometrys and that we  notice by changing sign on dl->twisted*/
		return ((rt_dist2d_pt_seg(C,A,B,dl)) && (rt_dist2d_pt_seg(D,A,B,dl))); /*if all is successful we return true*/
	}
	else
	{
		return RT_FALSE; /* if any of the calls to rt_dist2d_pt_seg goes wrong we return false*/
	}
}

/*------------------------------------------------------------------------------------------------------------
End of New faster distance calculations
--------------------------------------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------------------------------------
Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/

/**
rt_dist2d_comp from p to line A->B
This one is now sending every occation to rt_dist2d_pt_pt
Before it was handling occations where r was between 0 and 1 internally
and just returning the distance without identifying the points.
To get this points it was nessecary to change and it also showed to be about 10%faster.
*/
int
rt_dist2d_pt_seg(const POINT2D *p, const POINT2D *A, const POINT2D *B, DISTPTS *dl)
{
	POINT2D c;
	double	r;
	/*if start==end, then use pt distance */
	if (  ( A->x == B->x) && (A->y == B->y) )
	{
		return rt_dist2d_pt_pt(p,A,dl);
	}
	/*
	 * otherwise, we use comp.graphics.algorithms
	 * Frequently Asked Questions method
	 *
	 *  (1)        AC dot AB
	 *         r = ---------
	 *              ||AB||^2
	 *	r has the following meaning:
	 *	r=0 P = A
	 *	r=1 P = B
	 *	r<0 P is on the backward extension of AB
	 *	r>1 P is on the forward extension of AB
	 *	0<r<1 P is interior to AB
	 */

	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	/*This is for finding the maxdistance.
	the maxdistance have to be between two vertexes,
	compared to mindistance which can be between
	tvo vertexes vertex.*/
	if (dl->mode == DIST_MAX)
	{
		if (r>=0.5)
		{
			return rt_dist2d_pt_pt(p,A,dl);
		}
		if (r<0.5)
		{
			return rt_dist2d_pt_pt(p,B,dl);
		}
	}

	if (r<0)	/*If p projected on the line is outside point A*/
	{
		return rt_dist2d_pt_pt(p,A,dl);
	}
	if (r>=1)	/*If p projected on the line is outside point B or on point B*/
	{
		return rt_dist2d_pt_pt(p,B,dl);
	}
	
	/*If the point p is on the segment this is a more robust way to find out that*/
	if (( ((A->y-p->y)*(B->x-A->x)==(A->x-p->x)*(B->y-A->y) ) ) && (dl->mode ==  DIST_MIN))
	{
		dl->distance = 0.0;
		dl->p1 = *p;
		dl->p2 = *p;
	}
	
	/*If the projection of point p on the segment is between A and B
	then we find that "point on segment" and send it to rt_dist2d_pt_pt*/
	c.x=A->x + r * (B->x-A->x);
	c.y=A->y + r * (B->y-A->y);

	return rt_dist2d_pt_pt(p,&c,dl);
}


/**

Compares incomming points and
stores the points closest to each other
or most far away from each other
depending on dl->mode (max or min)
*/
int
rt_dist2d_pt_pt(const POINT2D *thep1, const POINT2D *thep2, DISTPTS *dl)
{
	double hside = thep2->x - thep1->x;
	double vside = thep2->y - thep1->y;
	double dist = sqrt ( hside*hside + vside*vside );

	if (((dl->distance - dist)*(dl->mode))>0) /*multiplication with mode to handle mindistance (mode=1)  and maxdistance (mode = (-1)*/
	{
		dl->distance = dist;

		if (dl->twisted>0)	/*To get the points in right order. twisted is updated between 1 and (-1) every time the order is changed earlier in the chain*/
		{
			dl->p1 = *thep1;
			dl->p2 = *thep2;
		}
		else
		{
			dl->p1 = *thep2;
			dl->p2 = *thep1;
		}
	}
	return RT_TRUE;
}




/*------------------------------------------------------------------------------------------------------------
End of Functions in common for Brute force and new calculation
--------------------------------------------------------------------------------------------------------------*/


/**
The old function nessecary for ptarray_segmentize2d in ptarray.c
*/
double
distance2d_pt_pt(const POINT2D *p1, const POINT2D *p2)
{
	double hside = p2->x - p1->x;
	double vside = p2->y - p1->y;

	return sqrt ( hside*hside + vside*vside );

}

double
distance2d_sqr_pt_pt(const POINT2D *p1, const POINT2D *p2)
{
	double hside = p2->x - p1->x;
	double vside = p2->y - p1->y;

	return  hside*hside + vside*vside;

}


/**

The old function nessecary for ptarray_segmentize2d in ptarray.c
*/
double
distance2d_pt_seg(const POINT2D *p, const POINT2D *A, const POINT2D *B)
{
	double	r,s;

	/*if start==end, then use pt distance */
	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance2d_pt_pt(p,A);

	/*
	 * otherwise, we use comp.graphics.algorithms
	 * Frequently Asked Questions method
	 *
	 *  (1)     	      AC dot AB
	        *         r = ---------
	        *               ||AB||^2
	 *	r has the following meaning:
	 *	r=0 P = A
	 *	r=1 P = B
	 *	r<0 P is on the backward extension of AB
	 *	r>1 P is on the forward extension of AB
	 *	0<r<1 P is interior to AB
	 */

	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	if (r<0) return distance2d_pt_pt(p,A);
	if (r>1) return distance2d_pt_pt(p,B);


	/*
	 * (2)
	 *	     (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
	 *	s = -----------------------------
	 *	             	L^2
	 *
	 *	Then the distance from C to P = |s|*L.
	 *
	 */

	s = ( (A->y-p->y)*(B->x-A->x)- (A->x-p->x)*(B->y-A->y) ) /
	    ( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	return FP_ABS(s) * sqrt(
	           (B->x-A->x)*(B->x-A->x) + (B->y-A->y)*(B->y-A->y)
	       );
}

/* return distance squared, useful to avoid sqrt calculations */
double
distance2d_sqr_pt_seg(const POINT2D *p, const POINT2D *A, const POINT2D *B)
{
	double	r,s;

	if (  ( A->x == B->x) && (A->y == B->y) )
		return distance2d_sqr_pt_pt(p,A);

	r = ( (p->x-A->x) * (B->x-A->x) + (p->y-A->y) * (B->y-A->y) )/( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	if (r<0) return distance2d_sqr_pt_pt(p,A);
	if (r>1) return distance2d_sqr_pt_pt(p,B);


	/*
	 * (2)
	 *	     (Ay-Cy)(Bx-Ax)-(Ax-Cx)(By-Ay)
	 *	s = -----------------------------
	 *	             	L^2
	 *
	 *	Then the distance from C to P = |s|*L.
	 *
	 */

	s = ( (A->y-p->y)*(B->x-A->x)- (A->x-p->x)*(B->y-A->y) ) /
	    ( (B->x-A->x)*(B->x-A->x) +(B->y-A->y)*(B->y-A->y) );

	return s * s * ( (B->x-A->x)*(B->x-A->x) + (B->y-A->y)*(B->y-A->y) );
}



/**
 * Compute the azimuth of segment AB in radians.
 * Return 0 on exception (same point), 1 otherwise.
 */
int
azimuth_pt_pt(const POINT2D *A, const POINT2D *B, double *d)
{
	if ( A->x == B->x )
	{
		if ( A->y < B->y ) *d=0.0;
		else if ( A->y > B->y ) *d=M_PI;
		else return 0;
		return 1;
	}

	if ( A->y == B->y )
	{
		if ( A->x < B->x ) *d=M_PI/2;
		else if ( A->x > B->x ) *d=M_PI+(M_PI/2);
		else return 0;
		return 1;
	}

	if ( A->x < B->x )
	{
		if ( A->y < B->y )
		{
			*d=atan(fabs(A->x - B->x) / fabs(A->y - B->y) );
		}
		else /* ( A->y > B->y )  - equality case handled above */
		{
			*d=atan(fabs(A->y - B->y) / fabs(A->x - B->x) )
			   + (M_PI/2);
		}
	}

	else /* ( A->x > B->x ) - equality case handled above */
	{
		if ( A->y > B->y )
		{
			*d=atan(fabs(A->x - B->x) / fabs(A->y - B->y) )
			   + M_PI;
		}
		else /* ( A->y < B->y )  - equality case handled above */
		{
			*d=atan(fabs(A->y - B->y) / fabs(A->x - B->x) )
			   + (M_PI+(M_PI/2));
		}
	}

	return 1;
}


