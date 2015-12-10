/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2011-2012 Sandro Santilli <strk@keybit.net>
 * Copyright (C) 2011 Paul Ramsey <pramsey@cleverelephant.ca>
 * Copyright (C) 2007-2008 Mark Cave-Ayland
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef _LIBRTGEOM_INTERNAL_H
#define _LIBRTGEOM_INTERNAL_H 1

#include "rttopo_config.h"
#include "librtgeom.h"

#include "rtgeom_log.h"
#include "proj_api.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

#if defined(PJ_VERSION) && PJ_VERSION >= 490
/* Enable new geodesic functions */
#define PROJ_GEODESIC 1
#else
/* Use the old (pre-2.2) geodesic functions */
#define PROJ_GEODESIC 0
#endif


#include <float.h>

#include "librtgeom.h"

/**
* Floating point comparators.
*/
#define FP_TOLERANCE 1e-12
#define FP_IS_ZERO(A) (fabs(A) <= FP_TOLERANCE)
#define FP_MAX(A, B) (((A) > (B)) ? (A) : (B))
#define FP_MIN(A, B) (((A) < (B)) ? (A) : (B))
#define FP_ABS(a)   ((a) <	(0) ? -(a) : (a))
#define FP_EQUALS(A, B) (fabs((A)-(B)) <= FP_TOLERANCE)
#define FP_NEQUALS(A, B) (fabs((A)-(B)) > FP_TOLERANCE)
#define FP_LT(A, B) (((A) + FP_TOLERANCE) < (B))
#define FP_LTEQ(A, B) (((A) - FP_TOLERANCE) <= (B))
#define FP_GT(A, B) (((A) - FP_TOLERANCE) > (B))
#define FP_GTEQ(A, B) (((A) + FP_TOLERANCE) >= (B))
#define FP_CONTAINS_TOP(A, X, B) (FP_LT(A, X) && FP_LTEQ(X, B))
#define FP_CONTAINS_BOTTOM(A, X, B) (FP_LTEQ(A, X) && FP_LT(X, B))
#define FP_CONTAINS_INCL(A, X, B) (FP_LTEQ(A, X) && FP_LTEQ(X, B))
#define FP_CONTAINS_EXCL(A, X, B) (FP_LT(A, X) && FP_LT(X, B))
#define FP_CONTAINS(A, X, B) FP_CONTAINS_EXCL(A, X, B)


/*
* this will change to NaN when I figure out how to
* get NaN in a platform-independent way
*/
#define NO_VALUE 0.0
#define NO_Z_VALUE NO_VALUE
#define NO_M_VALUE NO_VALUE


/**
* Well-Known Text (RTWKT) Output Variant Types
*/
#define RTWKT_NO_TYPE 0x08 /* Internal use only */
#define RTWKT_NO_PARENS 0x10 /* Internal use only */
#define RTWKT_IS_CHILD 0x20 /* Internal use only */

/**
* Well-Known Binary (RTWKB) Output Variant Types
*/

#define RTWKB_DOUBLE_SIZE 8 /* Internal use only */
#define RTWKB_INT_SIZE 4 /* Internal use only */
#define RTWKB_BYTE_SIZE 1 /* Internal use only */

/**
* Well-Known Binary (RTWKB) Geometry Types 
*/
#define RTWKB_POINT_TYPE 1
#define RTWKB_LINESTRING_TYPE 2
#define RTWKB_POLYGON_TYPE 3
#define RTWKB_MULTIPOINT_TYPE 4
#define RTWKB_MULTILINESTRING_TYPE 5
#define RTWKB_MULTIPOLYGON_TYPE 6
#define RTWKB_GEOMETRYCOLLECTION_TYPE 7
#define RTWKB_CIRCULARSTRING_TYPE 8
#define RTWKB_COMPOUNDCURVE_TYPE 9
#define RTWKB_CURVEPOLYGON_TYPE 10
#define RTWKB_MULTICURVE_TYPE 11
#define RTWKB_MULTISURFACE_TYPE 12
#define RTWKB_CURVE_TYPE 13 /* from ISO draft, not sure is real */
#define RTWKB_SURFACE_TYPE 14 /* from ISO draft, not sure is real */
#define RTWKB_POLYHEDRALSURFACE_TYPE 15
#define RTWKB_TIN_TYPE 16
#define RTWKB_TRIANGLE_TYPE 17

/**
* Macro for reading the size from the GSERIALIZED size attribute.
* Cribbed from PgSQL, top 30 bits are size. Use VARSIZE() when working
* internally with PgSQL.
*/
#define SIZE_GET(varsize) (((varsize) >> 2) & 0x3FFFFFFF)
#define SIZE_SET(varsize, size) (((varsize) & 0x00000003)|(((size) & 0x3FFFFFFF) << 2 ))

/**
* Tolerance used to determine equality.
*/
#define EPSILON_SQLMM 1e-8

/*
 * Export functions
 */
#define OUT_MAX_DOUBLE 1E15
#define OUT_SHOW_DIGS_DOUBLE 20
#define OUT_MAX_DOUBLE_PRECISION 15
#define OUT_MAX_DIGS_DOUBLE (OUT_SHOW_DIGS_DOUBLE + 2) /* +2 mean add dot and sign */


/**
* Constants for point-in-polygon return values
*/
#define RT_INSIDE 1
#define RT_BOUNDARY 0
#define RT_OUTSIDE -1

/*
* Internal prototypes
*/

/* Machine endianness */
#define XDR 0 /* big endian */
#define NDR 1 /* little endian */
extern char getMachineEndian(RTCTX *ctx);


/*
* Force dims
*/
RTGEOM* rtgeom_force_dims(RTCTX *ctx, const RTGEOM *rtgeom, int hasz, int hasm);
RTPOINT* rtpoint_force_dims(RTCTX *ctx, const RTPOINT *rtpoint, int hasz, int hasm);
RTLINE* rtline_force_dims(RTCTX *ctx, const RTLINE *rtline, int hasz, int hasm);
RTPOLY* rtpoly_force_dims(RTCTX *ctx, const RTPOLY *rtpoly, int hasz, int hasm);
RTCOLLECTION* rtcollection_force_dims(RTCTX *ctx, const RTCOLLECTION *rtcol, int hasz, int hasm);
RTPOINTARRAY* ptarray_force_dims(RTCTX *ctx, const RTPOINTARRAY *pa, int hasz, int hasm);

/**
 * Swap ordinate values o1 and o2 on a given RTPOINTARRAY
 *
 * Ordinates semantic is: 0=x 1=y 2=z 3=m
 */
void ptarray_swap_ordinates(RTCTX *ctx, RTPOINTARRAY *pa, RTORD o1, RTORD o2);

/*
* Is Empty?
*/
int rtpoly_is_empty(RTCTX *ctx, const RTPOLY *poly);
int rtcollection_is_empty(RTCTX *ctx, const RTCOLLECTION *col);
int rtcircstring_is_empty(RTCTX *ctx, const RTCIRCSTRING *circ);
int rttriangle_is_empty(RTCTX *ctx, const RTTRIANGLE *triangle);
int rtline_is_empty(RTCTX *ctx, const RTLINE *line);
int rtpoint_is_empty(RTCTX *ctx, const RTPOINT *point);

/*
* Number of vertices?
*/
int rtline_count_vertices(RTCTX *ctx, RTLINE *line);
int rtpoly_count_vertices(RTCTX *ctx, RTPOLY *poly);
int rtcollection_count_vertices(RTCTX *ctx, RTCOLLECTION *col);

/*
* Read from byte buffer
*/
extern uint32_t rt_get_uint32_t(RTCTX *ctx, const uint8_t *loc);
extern int32_t rt_get_int32_t(RTCTX *ctx, const uint8_t *loc);

/*
* DP simplification
*/

/**
 * @param minpts minimun number of points to retain, if possible.
 */
RTPOINTARRAY* ptarray_simplify(RTCTX *ctx, RTPOINTARRAY *inpts, double epsilon, unsigned int minpts);
RTLINE* rtline_simplify(RTCTX *ctx, const RTLINE *iline, double dist, int preserve_collapsed);
RTPOLY* rtpoly_simplify(RTCTX *ctx, const RTPOLY *ipoly, double dist, int preserve_collapsed);
RTCOLLECTION* rtcollection_simplify(RTCTX *ctx, const RTCOLLECTION *igeom, double dist, int preserve_collapsed);

/*
* Computational geometry
*/
int signum(RTCTX *ctx, double n);

/*
* The possible ways a pair of segments can interact. Returned by rt_segment_intersects 
*/
enum RTCG_SEGMENT_INTERSECTION_TYPE {
    SEG_ERROR = -1,
    SEG_NO_INTERSECTION = 0,
    SEG_COLINEAR = 1,
    SEG_CROSS_LEFT = 2,
    SEG_CROSS_RIGHT = 3,
    SEG_TOUCH_LEFT = 4,
    SEG_TOUCH_RIGHT = 5
};

/*
* Do the segments intersect? How?
*/
int rt_segment_intersects(RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2, const RTPOINT2D *q1, const RTPOINT2D *q2);

/*
* Get/Set an enumeratoed ordinate. (x,y,z,m)
*/
double rtpoint_get_ordinate(RTCTX *ctx, const RTPOINT4D *p, char ordinate);
void rtpoint_set_ordinate(RTCTX *ctx, RTPOINT4D *p, char ordinate, double value);

/* 
* Generate an interpolated coordinate p given an interpolation value and ordinate to apply it to
*/
int point_interpolate(RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2, RTPOINT4D *p, int hasz, int hasm, char ordinate, double interpolation_value);


/**
* Clip a line based on the from/to range of one of its ordinates. Use for m- and z- clipping 
*/
RTCOLLECTION *rtline_clip_to_ordinate_range(RTCTX *ctx, const RTLINE *line, char ordinate, double from, double to);

/**
* Clip a multi-line based on the from/to range of one of its ordinates. Use for m- and z- clipping 
*/
RTCOLLECTION *rtmline_clip_to_ordinate_range(RTCTX *ctx, const RTMLINE *mline, char ordinate, double from, double to);

/**
* Clip a multi-point based on the from/to range of one of its ordinates. Use for m- and z- clipping 
*/
RTCOLLECTION *rtmpoint_clip_to_ordinate_range(RTCTX *ctx, const RTMPOINT *mpoint, char ordinate, double from, double to);

/**
* Clip a point based on the from/to range of one of its ordinates. Use for m- and z- clipping 
*/
RTCOLLECTION *rtpoint_clip_to_ordinate_range(RTCTX *ctx, const RTPOINT *mpoint, char ordinate, double from, double to);

/*
* Geohash
*/
int rtgeom_geohash_precision(RTCTX *ctx, RTGBOX bbox, RTGBOX *bounds);
char *geohash_point(RTCTX *ctx, double longitude, double latitude, int precision);
void decode_geohash_bbox(RTCTX *ctx, char *geohash, double *lat, double *lon, int precision);

/*
* Point comparisons
*/
int p4d_same(RTCTX *ctx, const RTPOINT4D *p1, const RTPOINT4D *p2);
int p3d_same(RTCTX *ctx, const POINT3D *p1, const POINT3D *p2);
int p2d_same(RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2);

/*
* Area calculations
*/
double rtpoly_area(RTCTX *ctx, const RTPOLY *poly);
double rtcurvepoly_area(RTCTX *ctx, const RTCURVEPOLY *curvepoly);
double rttriangle_area(RTCTX *ctx, const RTTRIANGLE *triangle);

/**
* Pull a #RTGBOX from the header of a #GSERIALIZED, if one is available. If
* it is not, return RT_FAILURE.
*/
extern int gserialized_read_gbox_p(RTCTX *ctx, const GSERIALIZED *g, RTGBOX *gbox);

/*
* Length calculations
*/
double rtcompound_length(RTCTX *ctx, const RTCOMPOUND *comp);
double rtcompound_length_2d(RTCTX *ctx, const RTCOMPOUND *comp);
double rtline_length(RTCTX *ctx, const RTLINE *line);
double rtline_length_2d(RTCTX *ctx, const RTLINE *line);
double rtcircstring_length(RTCTX *ctx, const RTCIRCSTRING *circ);
double rtcircstring_length_2d(RTCTX *ctx, const RTCIRCSTRING *circ);
double rtpoly_perimeter(RTCTX *ctx, const RTPOLY *poly);
double rtpoly_perimeter_2d(RTCTX *ctx, const RTPOLY *poly);
double rtcurvepoly_perimeter(RTCTX *ctx, const RTCURVEPOLY *poly);
double rtcurvepoly_perimeter_2d(RTCTX *ctx, const RTCURVEPOLY *poly);
double rttriangle_perimeter(RTCTX *ctx, const RTTRIANGLE *triangle);
double rttriangle_perimeter_2d(RTCTX *ctx, const RTTRIANGLE *triangle);

/*
* Segmentization
*/
RTLINE *rtcircstring_stroke(RTCTX *ctx, const RTCIRCSTRING *icurve, uint32_t perQuad);
RTLINE *rtcompound_stroke(RTCTX *ctx, const RTCOMPOUND *icompound, uint32_t perQuad);
RTPOLY *rtcurvepoly_stroke(RTCTX *ctx, const RTCURVEPOLY *curvepoly, uint32_t perQuad);

/*
* Affine
*/
void ptarray_affine(RTCTX *ctx, RTPOINTARRAY *pa, const AFFINE *affine);

/*
* Scale
*/
void ptarray_scale(RTCTX *ctx, RTPOINTARRAY *pa, const RTPOINT4D *factor);

/*
* PointArray
*/
int ptarray_has_z(RTCTX *ctx, const RTPOINTARRAY *pa);
int ptarray_has_m(RTCTX *ctx, const RTPOINTARRAY *pa);
double ptarray_signed_area(RTCTX *ctx, const RTPOINTARRAY *pa);

/*
* Clone support
*/
RTLINE *rtline_clone(RTCTX *ctx, const RTLINE *rtgeom);
RTPOLY *rtpoly_clone(RTCTX *ctx, const RTPOLY *rtgeom);
RTTRIANGLE *rttriangle_clone(RTCTX *ctx, const RTTRIANGLE *rtgeom);
RTCOLLECTION *rtcollection_clone(RTCTX *ctx, const RTCOLLECTION *rtgeom);
RTCIRCSTRING *rtcircstring_clone(RTCTX *ctx, const RTCIRCSTRING *curve);
RTPOINTARRAY *ptarray_clone(RTCTX *ctx, const RTPOINTARRAY *ptarray);
RTGBOX *box2d_clone(RTCTX *ctx, const RTGBOX *rtgeom);
RTLINE *rtline_clone_deep(RTCTX *ctx, const RTLINE *rtgeom);
RTPOLY *rtpoly_clone_deep(RTCTX *ctx, const RTPOLY *rtgeom);
RTCOLLECTION *rtcollection_clone_deep(RTCTX *ctx, const RTCOLLECTION *rtgeom);
RTGBOX *gbox_clone(RTCTX *ctx, const RTGBOX *gbox);

/*
* Startpoint
*/
int rtpoly_startpoint(RTCTX *ctx, const RTPOLY* rtpoly, RTPOINT4D* pt);
int ptarray_startpoint(RTCTX *ctx, const RTPOINTARRAY* pa, RTPOINT4D* pt);
int rtcollection_startpoint(RTCTX *ctx, const RTCOLLECTION* col, RTPOINT4D* pt);

/*
 * Write into *ret the coordinates of the closest point on
 * segment A-B to the reference input point R
 */
void closest_point_on_segment(RTCTX *ctx, const RTPOINT4D *R, const RTPOINT4D *A, const RTPOINT4D *B, RTPOINT4D *ret);

/* 
* Repeated points
*/
RTPOINTARRAY *ptarray_remove_repeated_points_minpoints(RTCTX *ctx, const RTPOINTARRAY *in, double tolerance, int minpoints);
RTPOINTARRAY *ptarray_remove_repeated_points(RTCTX *ctx, const RTPOINTARRAY *in, double tolerance);
RTGEOM* rtmpoint_remove_repeated_points(RTCTX *ctx, const RTMPOINT *in, double tolerance);
RTGEOM* rtline_remove_repeated_points(RTCTX *ctx, const RTLINE *in, double tolerance);
RTGEOM* rtcollection_remove_repeated_points(RTCTX *ctx, const RTCOLLECTION *in, double tolerance);
RTGEOM* rtpoly_remove_repeated_points(RTCTX *ctx, const RTPOLY *in, double tolerance);

/*
* Closure test
*/
int rtline_is_closed(RTCTX *ctx, const RTLINE *line);
int rtpoly_is_closed(RTCTX *ctx, const RTPOLY *poly);
int rtcircstring_is_closed(RTCTX *ctx, const RTCIRCSTRING *curve);
int rtcompound_is_closed(RTCTX *ctx, const RTCOMPOUND *curve);
int rtpsurface_is_closed(RTCTX *ctx, const RTPSURFACE *psurface);
int rttin_is_closed(RTCTX *ctx, const RTTIN *tin);

/**
* Snap to grid
*/

/**
* Snap-to-grid Support
*/
typedef struct gridspec_t
{
	double ipx;
	double ipy;
	double ipz;
	double ipm;
	double xsize;
	double ysize;
	double zsize;
	double msize;
}
gridspec;

RTGEOM* rtgeom_grid(RTCTX *ctx, const RTGEOM *rtgeom, const gridspec *grid);
RTCOLLECTION* rtcollection_grid(RTCTX *ctx, const RTCOLLECTION *coll, const gridspec *grid);
RTPOINT* rtpoint_grid(RTCTX *ctx, const RTPOINT *point, const gridspec *grid);
RTPOLY* rtpoly_grid(RTCTX *ctx, const RTPOLY *poly, const gridspec *grid);
RTLINE* rtline_grid(RTCTX *ctx, const RTLINE *line, const gridspec *grid);
RTCIRCSTRING* rtcircstring_grid(RTCTX *ctx, const RTCIRCSTRING *line, const gridspec *grid);
RTPOINTARRAY* ptarray_grid(RTCTX *ctx, const RTPOINTARRAY *pa, const gridspec *grid);

/*
* What side of the line formed by p1 and p2 does q fall? 
* Returns -1 for left and 1 for right and 0 for co-linearity
*/
int rt_segment_side(RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2, const RTPOINT2D *q);
int rt_arc_side(RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3, const RTPOINT2D *Q);
int rt_arc_calculate_gbox_cartesian_2d(RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3, RTGBOX *gbox);
double rt_arc_center(RTCTX *ctx, const RTPOINT2D *p1, const RTPOINT2D *p2, const RTPOINT2D *p3, RTPOINT2D *result);
int rt_pt_in_seg(RTCTX *ctx, const RTPOINT2D *P, const RTPOINT2D *A1, const RTPOINT2D *A2);
int rt_pt_in_arc(RTCTX *ctx, const RTPOINT2D *P, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3);
int rt_arc_is_pt(RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3);
double rt_seg_length(RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2);
double rt_arc_length(RTCTX *ctx, const RTPOINT2D *A1, const RTPOINT2D *A2, const RTPOINT2D *A3);
int pt_in_ring_2d(RTCTX *ctx, const RTPOINT2D *p, const RTPOINTARRAY *ring);
int ptarray_contains_point(RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt);
int ptarrayarc_contains_point(RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt);
int ptarray_contains_point_partial(RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt, int check_closed, int *winding_number);
int ptarrayarc_contains_point_partial(RTCTX *ctx, const RTPOINTARRAY *pa, const RTPOINT2D *pt, int check_closed, int *winding_number);
int rtcompound_contains_point(RTCTX *ctx, const RTCOMPOUND *comp, const RTPOINT2D *pt);
int rtgeom_contains_point(RTCTX *ctx, const RTGEOM *geom, const RTPOINT2D *pt);

/**
* Split a line by a point and push components to the provided multiline.
* If the point doesn't split the line, push nothing to the container.
* Returns 0 if the point is off the line.
* Returns 1 if the point is on the line boundary (endpoints).
* Return 2 if the point is on the interior of the line (only case in which
* a split happens).
*
* NOTE: the components pushed to the output vector have their SRID stripped 
*/
int rtline_split_by_point_to(RTCTX *ctx, const RTLINE* ln, const RTPOINT* pt, RTMLINE* to);

/** Ensure the collection can hold at least up to ngeoms geometries */
void rtcollection_reserve(RTCTX *ctx, RTCOLLECTION *col, int ngeoms);

/** Check if subtype is allowed in collectiontype */
extern int rtcollection_allows_subtype(RTCTX *ctx, int collectiontype, int subtype);

/** RTGBOX utility functions to figure out coverage/location on the globe */
double gbox_angular_height(RTCTX *ctx, const RTGBOX* gbox);
double gbox_angular_width(RTCTX *ctx, const RTGBOX* gbox);
int gbox_centroid(RTCTX *ctx, const RTGBOX* gbox, RTPOINT2D* out);

/* Utilities */
extern void trim_trailing_zeros(RTCTX *ctx, char *num);

extern uint8_t RTMULTITYPE[RTNUMTYPES];

extern rtinterrupt_callback *_rtgeom_interrupt_callback;
extern int _rtgeom_interrupt_requested;
#define RT_ON_INTERRUPT(x) { \
  if ( _rtgeom_interrupt_callback ) { \
    (*_rtgeom_interrupt_callback)(); \
  } \
  if ( _rtgeom_interrupt_requested ) { \
    _rtgeom_interrupt_requested = 0; \
    rtnotice(ctx, "librtgeom code interrupted"); \
    x; \
  } \
}

int ptarray_npoints_in_rect(RTCTX *ctx, const RTPOINTARRAY *pa, const RTGBOX *gbox);
int gbox_contains_point2d(RTCTX *ctx, const RTGBOX *g, const RTPOINT2D *p);
int rtpoly_contains_point(RTCTX *ctx, const RTPOLY *poly, const RTPOINT2D *pt);

/*******************************************************************************
 * PROJ4-dependent extra functions on RTGEOM
 ******************************************************************************/

/**
 * Get a projection from a string representation
 *
 * Eg: "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
 */
projPJ rtproj_from_string(RTCTX *ctx, const char* txt);

/**
 * Transform (reproject) a geometry in-place.
 * @param geom the geometry to transform
 * @param inpj the input (or current, or source) projection
 * @param outpj the output (or destination) projection
 */
int rtgeom_transform(RTCTX *ctx, RTGEOM *geom, projPJ inpj, projPJ outpj) ;
int ptarray_transform(RTCTX *ctx, RTPOINTARRAY *geom, projPJ inpj, projPJ outpj) ;
int point4d_transform(RTCTX *ctx, RTPOINT4D *pt, projPJ srcpj, projPJ dstpj) ;

#endif /* _LIBRTGEOM_INTERNAL_H */
