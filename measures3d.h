/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2011 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _MEASURES3D_H
#define _MEASURES3D_H 1
#include <float.h>
#include "measures.h"

#define DOT(u,v)   ((u).x * (v).x + (u).y * (v).y + (u).z * (v).z)
#define VECTORLENGTH(v)   sqrt(((v).x * (v).x) + ((v).y * (v).y) + ((v).z * (v).z))


/**

Structure used in distance-calculations
*/
typedef struct
{
	double distance;	/*the distance between p1 and p2*/
	POINT3DZ p1;
	POINT3DZ p2;
	int mode;	/*the direction of looking, if thedir = -1 then we look for 3dmaxdistance and if it is 1 then we look for 3dmindistance*/
	int twisted; /*To preserve the order of incoming points to match the first and second point in 3dshortest and 3dlongest line*/
	double tolerance; /*the tolerance for 3ddwithin and 3ddfullywithin*/
} DISTPTS3D;

typedef struct
{
	double	x,y,z;  
}
VECTOR3D; 

typedef struct
{
	POINT3DZ		pop;  /*Point On Plane*/
	VECTOR3D	pv;  /*Perpendicular normal vector*/
}
PLANE3D; 


/*
Geometry returning functions
*/
RTGEOM * rt_dist3d_distancepoint(const RTGEOM *rt1, const RTGEOM *rt2,int srid,int mode);
RTGEOM * rt_dist3d_distanceline(const RTGEOM *rt1, const RTGEOM *rt2,int srid,int mode);

/*
Preprocessing functions
*/
int rt_dist3d_distribute_bruteforce(const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS3D *dl);
int rt_dist3d_recursive(const RTGEOM *rtg1,const RTGEOM *rtg2, DISTPTS3D *dl);
int rt_dist3d_distribute_fast(const RTGEOM *rtg1, const RTGEOM *rtg2, DISTPTS3D *dl);

/*
Brute force functions
*/
int rt_dist3d_pt_ptarray(POINT3DZ *p, POINTARRAY *pa, DISTPTS3D *dl);
int rt_dist3d_point_point(RTPOINT *point1, RTPOINT *point2, DISTPTS3D *dl);
int rt_dist3d_point_line(RTPOINT *point, RTLINE *line, DISTPTS3D *dl);
int rt_dist3d_line_line(RTLINE *line1,RTLINE *line2 , DISTPTS3D *dl);
int rt_dist3d_point_poly(RTPOINT *point, RTPOLY *poly, DISTPTS3D *dl);
int rt_dist3d_line_poly(RTLINE *line, RTPOLY *poly, DISTPTS3D *dl);
int rt_dist3d_poly_poly(RTPOLY *poly1, RTPOLY *poly2, DISTPTS3D *dl);
int rt_dist3d_ptarray_ptarray(POINTARRAY *l1, POINTARRAY *l2,DISTPTS3D *dl);
int rt_dist3d_seg_seg(POINT3DZ *A, POINT3DZ *B, POINT3DZ *C, POINT3DZ *D, DISTPTS3D *dl);
int rt_dist3d_pt_pt(POINT3DZ *p1, POINT3DZ *p2, DISTPTS3D *dl);
int rt_dist3d_pt_seg(POINT3DZ *p, POINT3DZ *A, POINT3DZ *B, DISTPTS3D *dl);
int rt_dist3d_pt_poly(POINT3DZ *p, RTPOLY *poly, PLANE3D *plane,POINT3DZ *projp,  DISTPTS3D *dl);
int rt_dist3d_ptarray_poly(POINTARRAY *pa, RTPOLY *poly, PLANE3D *plane, DISTPTS3D *dl);



double project_point_on_plane(POINT3DZ *p,  PLANE3D *pl, POINT3DZ *p0);
int define_plane(POINTARRAY *pa, PLANE3D *pl);
int pt_in_ring_3d(const POINT3DZ *p, const POINTARRAY *ring,PLANE3D *plane);

/*
Helper functions
*/


#endif /* !defined _MEASURES3D_H  */
