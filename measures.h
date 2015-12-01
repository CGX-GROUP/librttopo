
/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2010 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "librtgeom_internal.h"

/* for the measure functions*/
#define DIST_MAX		-1
#define DIST_MIN		1

/** 
* Structure used in distance-calculations
*/
typedef struct
{
	double distance;	/*the distance between p1 and p2*/
	POINT2D p1;
	POINT2D p2;
	int mode;	/*the direction of looking, if thedir = -1 then we look for maxdistance and if it is 1 then we look for mindistance*/
	int twisted; /*To preserve the order of incoming points to match the first and secon point in shortest and longest line*/
	double tolerance; /*the tolerance for dwithin and dfullywithin*/
} DISTPTS;

typedef struct
{
	double themeasure;	/*a value calculated to compare distances*/
	int pnr;	/*pointnumber. the ordernumber of the point*/
} LISTSTRUCT;


/*
* Preprocessing functions
*/
int rt_dist2d_comp(const RTGEOM *rt1, const RTGEOM *rt2, DISTPTS *dl);
int rt_dist2d_distribute_bruteforce(const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS *dl);
int rt_dist2d_recursive(const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS *dl);
int rt_dist2d_check_overlap(RTGEOM *rtg1, RTGEOM *rtg2);
int rt_dist2d_distribute_fast(RTGEOM *rtg1, RTGEOM *rtg2, DISTPTS *dl);

/*
* Brute force functions
*/
int rt_dist2d_pt_ptarray(const POINT2D *p, POINTARRAY *pa, DISTPTS *dl);
int rt_dist2d_pt_ptarrayarc(const POINT2D *p, const POINTARRAY *pa, DISTPTS *dl);
int rt_dist2d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2, DISTPTS *dl);
int rt_dist2d_ptarray_ptarrayarc(const POINTARRAY *pa, const POINTARRAY *pb, DISTPTS *dl);
int rt_dist2d_ptarrayarc_ptarrayarc(const POINTARRAY *pa, const POINTARRAY *pb, DISTPTS *dl);
int rt_dist2d_ptarray_poly(POINTARRAY *pa, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_point_point(RTPOINT *point1, RTPOINT *point2, DISTPTS *dl);
int rt_dist2d_point_line(RTPOINT *point, RTLINE *line, DISTPTS *dl);
int rt_dist2d_point_circstring(RTPOINT *point, RTCIRCSTRING *circ, DISTPTS *dl);
int rt_dist2d_point_poly(RTPOINT *point, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_point_curvepoly(RTPOINT *point, RTCURVEPOLY *poly, DISTPTS *dl);
int rt_dist2d_line_line(RTLINE *line1, RTLINE *line2, DISTPTS *dl);
int rt_dist2d_line_circstring(RTLINE *line1, RTCIRCSTRING *line2, DISTPTS *dl);
int rt_dist2d_line_poly(RTLINE *line, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_line_curvepoly(RTLINE *line, RTCURVEPOLY *poly, DISTPTS *dl);
int rt_dist2d_circstring_circstring(RTCIRCSTRING *line1, RTCIRCSTRING *line2, DISTPTS *dl);
int rt_dist2d_circstring_poly(RTCIRCSTRING *circ, RTPOLY *poly, DISTPTS *dl);
int rt_dist2d_circstring_curvepoly(RTCIRCSTRING *circ, RTCURVEPOLY *poly, DISTPTS *dl);
int rt_dist2d_poly_poly(RTPOLY *poly1, RTPOLY *poly2, DISTPTS *dl);
int rt_dist2d_poly_curvepoly(RTPOLY *poly1, RTCURVEPOLY *curvepoly2, DISTPTS *dl);
int rt_dist2d_curvepoly_curvepoly(RTCURVEPOLY *poly1, RTCURVEPOLY *poly2, DISTPTS *dl);

/*
* New faster distance calculations
*/
int rt_dist2d_pre_seg_seg(POINTARRAY *l1, POINTARRAY *l2,LISTSTRUCT *list1, LISTSTRUCT *list2,double k, DISTPTS *dl);
int rt_dist2d_selected_seg_seg(const POINT2D *A, const POINT2D *B, const POINT2D *C, const POINT2D *D, DISTPTS *dl);
int struct_cmp_by_measure(const void *a, const void *b);
int rt_dist2d_fast_ptarray_ptarray(POINTARRAY *l1,POINTARRAY *l2, DISTPTS *dl,  GBOX *box1, GBOX *box2);

/*
* Distance calculation primitives. 
*/
int rt_dist2d_pt_pt  (const POINT2D *P,  const POINT2D *Q,  DISTPTS *dl);
int rt_dist2d_pt_seg (const POINT2D *P,  const POINT2D *A1, const POINT2D *A2, DISTPTS *dl);
int rt_dist2d_pt_arc (const POINT2D *P,  const POINT2D *A1, const POINT2D *A2, const POINT2D *A3, DISTPTS *dl);
int rt_dist2d_seg_seg(const POINT2D *A1, const POINT2D *A2, const POINT2D *B1, const POINT2D *B2, DISTPTS *dl);
int rt_dist2d_seg_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *B1, const POINT2D *B2, const POINT2D *B3, DISTPTS *dl);
int rt_dist2d_arc_arc(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3, const POINT2D *B1, const POINT2D *B2, const POINT2D* B3, DISTPTS *dl);
void rt_dist2d_distpts_init(DISTPTS *dl, int mode);

/*
* Length primitives
*/
double rt_arc_length(const POINT2D *A1, const POINT2D *A2, const POINT2D *A3);

/*
* Geometry returning functions
*/
RTGEOM* rt_dist2d_distancepoint(const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode);
RTGEOM* rt_dist2d_distanceline(const RTGEOM *rt1, const RTGEOM *rt2, int srid, int mode);


