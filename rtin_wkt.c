/**********************************************************************
 *
 * rttopo - topology library
 *
 * Copyright (C) 2010 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdlib.h>
#include <ctype.h> /* for isspace */

#include "rtin_wkt.h"
#include "rtin_wkt_parse.h"
#include "rtgeom_log.h"


/*
* Error messages for failures in the parser. 
*/
const char *parser_error_messages[] =
{
	"",
	"geometry requires more points",
	"geometry must have an odd number of points",
	"geometry contains non-closed rings",
	"can not mix dimensionality in a geometry",
	"parse error - invalid geometry",
	"invalid WKB type",
	"incontinuous compound curve",
	"triangle must have exactly 4 points",
	"geometry has too many points",
	"parse error - invalid geometry"
};

#define SET_PARSER_ERROR(errno) { \
		global_parser_result.message = parser_error_messages[(errno)]; \
		global_parser_result.errcode = (errno); \
		global_parser_result.errlocation = wkt_yylloc.last_column; \
	}
		
/**
* Read the SRID number from an SRID=<> string
*/
int wkt_lexer_read_srid(char *str)
{
	char *c = str;
	long i = 0;
	int srid;

	if( ! str ) return SRID_UNKNOWN;
	c += 5; /* Advance past "SRID=" */
	i = strtol(c, NULL, 10);
	srid = clamp_srid((int)i);
	/* TODO: warn on explicit UNKNOWN srid ? */
	return srid;
}

static uint8_t wkt_dimensionality(char *dimensionality)
{
	int i = 0;
	uint8_t flags = 0;
	
	if( ! dimensionality ) 
		return flags;
	
	/* If there's an explicit dimensionality, we use that */
	for( i = 0; i < strlen(dimensionality); i++ )
	{
		if( (dimensionality[i] == 'Z') || (dimensionality[i] == 'z') )
			FLAGS_SET_Z(flags,1);
		else if( (dimensionality[i] == 'M') || (dimensionality[i] == 'm') )
			FLAGS_SET_M(flags,1);
		/* only a space is accepted in between */
		else if( ! isspace(dimensionality[i]) ) break;
	}
	return flags;
}


/**
* Force the dimensionality of a geometry to match the dimensionality
* of a set of flags (usually derived from a ZM WKT tag).
*/
static int wkt_parser_set_dims(RTGEOM *geom, uint8_t flags)
{
	int hasz = FLAGS_GET_Z(flags);
	int hasm = FLAGS_GET_M(flags);
	int i = 0;
	
	/* Error on junk */
	if( ! geom ) 
		return RT_FAILURE;

	FLAGS_SET_Z(geom->flags, hasz);
	FLAGS_SET_M(geom->flags, hasm);
	
	switch( geom->type )
	{
		case RTPOINTTYPE:
		{
			RTPOINT *pt = (RTPOINT*)geom;
			if ( pt->point )
			{
				FLAGS_SET_Z(pt->point->flags, hasz);
				FLAGS_SET_M(pt->point->flags, hasm);
			}
			break;
		}
		case RTTRIANGLETYPE:
		case RTCIRCSTRINGTYPE:
		case RTLINETYPE:
		{
			RTLINE *ln = (RTLINE*)geom;
			if ( ln->points )
			{
				FLAGS_SET_Z(ln->points->flags, hasz);
				FLAGS_SET_M(ln->points->flags, hasm);
			}
			break;
		}
		case RTPOLYGONTYPE:
		{
			RTPOLY *poly = (RTPOLY*)geom;
			for ( i = 0; i < poly->nrings; i++ )
			{
				if( poly->rings[i] )
				{
					FLAGS_SET_Z(poly->rings[i]->flags, hasz);
					FLAGS_SET_M(poly->rings[i]->flags, hasm);
				}
			}
			break;
		}
		case RTCURVEPOLYTYPE:
		{
			RTCURVEPOLY *poly = (RTCURVEPOLY*)geom;
			for ( i = 0; i < poly->nrings; i++ )
				wkt_parser_set_dims(poly->rings[i], flags);
			break;
		}
		default: 
		{
			if ( rttype_is_collection(geom->type) )
			{
				RTCOLLECTION *col = (RTCOLLECTION*)geom;
				for ( i = 0; i < col->ngeoms; i++ )
					wkt_parser_set_dims(col->geoms[i], flags);			
				return RT_SUCCESS;
			}
			else
			{
				RTDEBUGF(2,"Unknown geometry type: %d", geom->type);
				return RT_FAILURE;
			}
		}
	}

	return RT_SUCCESS;				
}

/**
* Read the dimensionality from a flag, if provided. Then check that the
* dimensionality matches that of the pointarray. If the dimension counts
* match, ensure the pointarray is using the right "Z" or "M".
*/
static int wkt_pointarray_dimensionality(POINTARRAY *pa, uint8_t flags)
{	
	int hasz = FLAGS_GET_Z(flags);
	int hasm = FLAGS_GET_M(flags);
	int ndims = 2 + hasz + hasm;

	/* No dimensionality or array means we go with what we have */
	if( ! (flags && pa) )
		return RT_TRUE;
		
	RTDEBUGF(5,"dimensionality ndims == %d", ndims);
	RTDEBUGF(5,"FLAGS_NDIMS(pa->flags) == %d", FLAGS_NDIMS(pa->flags));
	
	/* 
	* ndims > 2 implies that the flags have something useful to add,
	* that there is a 'Z' or an 'M' or both.
	*/
	if( ndims > 2 )
	{
		/* Mismatch implies a problem */
		if ( FLAGS_NDIMS(pa->flags) != ndims )
			return RT_FALSE;
		/* Match means use the explicit dimensionality */
		else
		{
			FLAGS_SET_Z(pa->flags, hasz);
			FLAGS_SET_M(pa->flags, hasm);
		}
	}

	return RT_TRUE;
}



/**
* Build a 2d coordinate.
*/
POINT wkt_parser_coord_2(double c1, double c2)
{
	POINT p;
	p.flags = 0;
	p.x = c1;
	p.y = c2;
	p.z = p.m = 0.0;
	FLAGS_SET_Z(p.flags, 0);
	FLAGS_SET_M(p.flags, 0);
	return p;
}

/**
* Note, if this is an XYM coordinate we'll have to fix it later when we build
* the object itself and have access to the dimensionality token.
*/
POINT wkt_parser_coord_3(double c1, double c2, double c3)
{
		POINT p;
		p.flags = 0;
		p.x = c1;
		p.y = c2;
		p.z = c3;
		p.m = 0;
		FLAGS_SET_Z(p.flags, 1);
		FLAGS_SET_M(p.flags, 0);
		return p;
}

/**
*/
POINT wkt_parser_coord_4(double c1, double c2, double c3, double c4)
{
	POINT p;
	p.flags = 0;
	p.x = c1;
	p.y = c2;
	p.z = c3;
	p.m = c4;
	FLAGS_SET_Z(p.flags, 1);
	FLAGS_SET_M(p.flags, 1);
	return p;
}

POINTARRAY* wkt_parser_ptarray_add_coord(POINTARRAY *pa, POINT p)
{
	POINT4D pt;
	RTDEBUG(4,"entered");
	
	/* Error on trouble */
	if( ! pa ) 
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;	
	}
	
	/* Check that the coordinate has the same dimesionality as the array */
	if( FLAGS_NDIMS(p.flags) != FLAGS_NDIMS(pa->flags) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
	
	/* While parsing the point arrays, XYM and XMZ points are both treated as XYZ */
	pt.x = p.x;
	pt.y = p.y;
	if( FLAGS_GET_Z(pa->flags) )
		pt.z = p.z;
	if( FLAGS_GET_M(pa->flags) )
		pt.m = p.m;
	/* If the destination is XYM, we'll write the third coordinate to m */
	if( FLAGS_GET_M(pa->flags) && ! FLAGS_GET_Z(pa->flags) )
		pt.m = p.z;
		
	ptarray_append_point(pa, &pt, RT_TRUE); /* Allow duplicate points in array */
	return pa;
}

/**
* Start a point array from the first coordinate.
*/
POINTARRAY* wkt_parser_ptarray_new(POINT p)
{
	int ndims = FLAGS_NDIMS(p.flags);
	POINTARRAY *pa = ptarray_construct_empty((ndims>2), (ndims>3), 4);
	RTDEBUG(4,"entered");
	if ( ! pa )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	return wkt_parser_ptarray_add_coord(pa, p);
}

/**
* Create a new point. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT.
*/
RTGEOM* wkt_parser_point_new(POINTARRAY *pa, char *dimensionality)
{
	uint8_t flags = wkt_dimensionality(dimensionality);
	RTDEBUG(4,"entered");
	
	/* No pointarray means it is empty */
	if( ! pa )
		return rtpoint_as_rtgeom(rtpoint_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == RT_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Only one point allowed in our point array! */	
	if( pa->npoints != 1 )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_LESSPOINTS);
		return NULL;
	}		

	return rtpoint_as_rtgeom(rtpoint_construct(SRID_UNKNOWN, NULL, pa));
}


/**
* Create a new linestring. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT. Check for numpoints >= 2 if
* requested.
*/
RTGEOM* wkt_parser_linestring_new(POINTARRAY *pa, char *dimensionality)
{
	uint8_t flags = wkt_dimensionality(dimensionality);
	RTDEBUG(4,"entered");

	/* No pointarray means it is empty */
	if( ! pa )
		return rtline_as_rtgeom(rtline_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == RT_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
	
	/* Apply check for not enough points, if requested. */	
	if( (global_parser_result.parser_check_flags & RT_PARSER_CHECK_MINPOINTS) && (pa->npoints < 2) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}

	return rtline_as_rtgeom(rtline_construct(SRID_UNKNOWN, NULL, pa));
}

/**
* Create a new circularstring. Null point array implies empty. Null dimensionality
* implies no specified dimensionality in the WKT. 
* Circular strings are just like linestrings, except with slighty different
* validity rules (minpoint == 3, numpoints % 2 == 1). 
*/
RTGEOM* wkt_parser_circularstring_new(POINTARRAY *pa, char *dimensionality)
{
	uint8_t flags = wkt_dimensionality(dimensionality);
	RTDEBUG(4,"entered");

	/* No pointarray means it is empty */
	if( ! pa )
		return rtcircstring_as_rtgeom(rtcircstring_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == RT_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
	
	/* Apply check for not enough points, if requested. */	
	if( (global_parser_result.parser_check_flags & RT_PARSER_CHECK_MINPOINTS) && (pa->npoints < 3) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}	

	/* Apply check for odd number of points, if requested. */	
	if( (global_parser_result.parser_check_flags & RT_PARSER_CHECK_ODD) && ((pa->npoints % 2) == 0) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_ODDPOINTS);
		return NULL;
	}
	
	return rtcircstring_as_rtgeom(rtcircstring_construct(SRID_UNKNOWN, NULL, pa));	
}

RTGEOM* wkt_parser_triangle_new(POINTARRAY *pa, char *dimensionality)
{
	uint8_t flags = wkt_dimensionality(dimensionality);
	RTDEBUG(4,"entered");

	/* No pointarray means it is empty */
	if( ! pa )
		return rttriangle_as_rtgeom(rttriangle_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions is not consistent, we have a problem. */
	if( wkt_pointarray_dimensionality(pa, flags) == RT_FALSE )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Triangles need four points. */	
	if( (pa->npoints != 4) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_TRIANGLEPOINTS);
		return NULL;
	}	
	
	/* Triangles need closure. */	
	if( ! ptarray_is_closed(pa) )
	{
		ptarray_free(pa);
		SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
		return NULL;
	}	

	return rttriangle_as_rtgeom(rttriangle_construct(SRID_UNKNOWN, NULL, pa));
}

RTGEOM* wkt_parser_polygon_new(POINTARRAY *pa, char dimcheck)
{
	RTPOLY *poly = NULL;
	RTDEBUG(4,"entered");
	
	/* No pointarray is a problem */
	if( ! pa )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;	
	}

	poly = rtpoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(pa->flags), FLAGS_GET_M(pa->flags));
	
	/* Error out if we can't build this polygon. */
	if( ! poly )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	
	wkt_parser_polygon_add_ring(rtpoly_as_rtgeom(poly), pa, dimcheck);	
	return rtpoly_as_rtgeom(poly);
}

RTGEOM* wkt_parser_polygon_add_ring(RTGEOM *poly, POINTARRAY *pa, char dimcheck)
{
	RTDEBUG(4,"entered");

	/* Bad inputs are a problem */
	if( ! (pa && poly) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;	
	}

	/* Rings must agree on dimensionality */
	if( FLAGS_NDIMS(poly->flags) != FLAGS_NDIMS(pa->flags) )
	{
		ptarray_free(pa);
		rtgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}

	/* Apply check for minimum number of points, if requested. */	
	if( (global_parser_result.parser_check_flags & RT_PARSER_CHECK_MINPOINTS) && (pa->npoints < 4) )
	{
		ptarray_free(pa);
		rtgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
		return NULL;
	}
	
	/* Apply check for not closed rings, if requested. */	
	if( (global_parser_result.parser_check_flags & RT_PARSER_CHECK_CLOSURE) && 
	    ! (dimcheck == 'Z' ? ptarray_is_closed_z(pa) : ptarray_is_closed_2d(pa)) )
	{
		ptarray_free(pa);
		rtgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
		return NULL;
	}

	/* If something goes wrong adding a ring, error out. */
	if ( RT_FAILURE == rtpoly_add_ring(rtgeom_as_rtpoly(poly), pa) )
	{
		ptarray_free(pa);
		rtgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;	
	}
	return poly;
}

RTGEOM* wkt_parser_polygon_finalize(RTGEOM *poly, char *dimensionality)
{
	uint8_t flags = wkt_dimensionality(dimensionality);
	int flagdims = FLAGS_NDIMS(flags);
	RTDEBUG(4,"entered");
	
	/* Null input implies empty return */
	if( ! poly )
		return rtpoly_as_rtgeom(rtpoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	/* If the number of dimensions are not consistent, we have a problem. */
	if( flagdims > 2 )
	{
		if ( flagdims != FLAGS_NDIMS(poly->flags) )
		{
			rtgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
			return NULL;
		}
	
		/* Harmonize the flags in the sub-components with the wkt flags */
		if( RT_FAILURE == wkt_parser_set_dims(poly, flags) )
		{
			rtgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}
	}
	
	return poly;
}

RTGEOM* wkt_parser_curvepolygon_new(RTGEOM *ring) 
{
	RTGEOM *poly;	
	RTDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! ring )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	
	/* Construct poly and add the ring. */
	poly = rtcurvepoly_as_rtgeom(rtcurvepoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(ring->flags), FLAGS_GET_M(ring->flags)));
	/* Return the result. */
	return wkt_parser_curvepolygon_add_ring(poly,ring);
}

RTGEOM* wkt_parser_curvepolygon_add_ring(RTGEOM *poly, RTGEOM *ring)
{
	RTDEBUG(4,"entered");

	/* Toss error on null input */
	if( ! (ring && poly) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		RTDEBUG(4,"inputs are null");
		return NULL;
	}
	
	/* All the elements must agree on dimensionality */
	if( FLAGS_NDIMS(poly->flags) != FLAGS_NDIMS(ring->flags) )
	{
		RTDEBUG(4,"dimensionality does not match");
		rtgeom_free(ring);
		rtgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
	
	/* Apply check for minimum number of points, if requested. */	
	if( (global_parser_result.parser_check_flags & RT_PARSER_CHECK_MINPOINTS) )
	{
		int vertices_needed = 3;

		if ( ring->type == RTLINETYPE )
			vertices_needed = 4;
					
		if (rtgeom_count_vertices(ring) < vertices_needed)
		{
			RTDEBUG(4,"number of points is incorrect");
			rtgeom_free(ring);
			rtgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_MOREPOINTS);
			return NULL;
		}		
	}
	
	/* Apply check for not closed rings, if requested. */	
	if( (global_parser_result.parser_check_flags & RT_PARSER_CHECK_CLOSURE) )
	{
		int is_closed = 1;
		RTDEBUG(4,"checking ring closure");
		switch ( ring->type )
		{
			case RTLINETYPE:
			is_closed = rtline_is_closed(rtgeom_as_rtline(ring));
			break;
			
			case RTCIRCSTRINGTYPE:
			is_closed = rtcircstring_is_closed(rtgeom_as_rtcircstring(ring));
			break;
			
			case RTCOMPOUNDTYPE:
			is_closed = rtcompound_is_closed(rtgeom_as_rtcompound(ring));
			break;
		}
		if ( ! is_closed )
		{
			RTDEBUG(4,"ring is not closed");
			rtgeom_free(ring);
			rtgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_UNCLOSED);
			return NULL;
		}
	}
		
	if( RT_FAILURE == rtcurvepoly_add_ring(rtgeom_as_rtcurvepoly(poly), ring) )
	{
		RTDEBUG(4,"failed to add ring");
		rtgeom_free(ring);
		rtgeom_free(poly);
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	
	return poly;
}

RTGEOM* wkt_parser_curvepolygon_finalize(RTGEOM *poly, char *dimensionality)
{
	uint8_t flags = wkt_dimensionality(dimensionality);
	int flagdims = FLAGS_NDIMS(flags);
	RTDEBUG(4,"entered");
	
	/* Null input implies empty return */
	if( ! poly )
		return rtcurvepoly_as_rtgeom(rtcurvepoly_construct_empty(SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));

	if ( flagdims > 2 )
	{
		/* If the number of dimensions are not consistent, we have a problem. */
		if( flagdims != FLAGS_NDIMS(poly->flags) )
		{
			rtgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
			return NULL;
		}

		/* Harmonize the flags in the sub-components with the wkt flags */
		if( RT_FAILURE == wkt_parser_set_dims(poly, flags) )
		{
			rtgeom_free(poly);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}
	}
	
	return poly;
}

RTGEOM* wkt_parser_collection_new(RTGEOM *geom) 
{
	RTCOLLECTION *col;
	RTGEOM **geoms;
	static int ngeoms = 1;
	RTDEBUG(4,"entered");
	
	/* Toss error on null geometry input */
	if( ! geom )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	
	/* Create our geometry array */
	geoms = rtalloc(sizeof(RTGEOM*) * ngeoms);
	geoms[0] = geom;
	
	/* Make a new collection */
	col = rtcollection_construct(RTCOLLECTIONTYPE, SRID_UNKNOWN, NULL, ngeoms, geoms);

	/* Return the result. */
	return rtcollection_as_rtgeom(col);
}


RTGEOM* wkt_parser_compound_new(RTGEOM *geom) 
{
	RTCOLLECTION *col;
	RTGEOM **geoms;
	static int ngeoms = 1;
	RTDEBUG(4,"entered");
	
	/* Toss error on null geometry input */
	if( ! geom )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
	
	/* Elements of a compoundcurve cannot be empty, because */
	/* empty things can't join up and form a ring */
	if ( rtgeom_is_empty(geom) )
	{
		rtgeom_free(geom);
		SET_PARSER_ERROR(PARSER_ERROR_INCONTINUOUS);
		return NULL;		
	}
	
	/* Create our geometry array */
	geoms = rtalloc(sizeof(RTGEOM*) * ngeoms);
	geoms[0] = geom;
	
	/* Make a new collection */
	col = rtcollection_construct(RTCOLLECTIONTYPE, SRID_UNKNOWN, NULL, ngeoms, geoms);

	/* Return the result. */
	return rtcollection_as_rtgeom(col);
}


RTGEOM* wkt_parser_compound_add_geom(RTGEOM *col, RTGEOM *geom)
{
	RTDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! (geom && col) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}

	/* All the elements must agree on dimensionality */
	if( FLAGS_NDIMS(col->flags) != FLAGS_NDIMS(geom->flags) )
	{
		rtgeom_free(col);
		rtgeom_free(geom);
		SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
		return NULL;
	}
	
	if( RT_FAILURE == rtcompound_add_rtgeom((RTCOMPOUND*)col, geom) )
	{
		rtgeom_free(col);
		rtgeom_free(geom);
		SET_PARSER_ERROR(PARSER_ERROR_INCONTINUOUS);
		return NULL;
	}
	
	return col;
}


RTGEOM* wkt_parser_collection_add_geom(RTGEOM *col, RTGEOM *geom)
{
	RTDEBUG(4,"entered");

	/* Toss error on null geometry input */
	if( ! (geom && col) )
	{
		SET_PARSER_ERROR(PARSER_ERROR_OTHER);
		return NULL;
	}
			
	return rtcollection_as_rtgeom(rtcollection_add_rtgeom(rtgeom_as_rtcollection(col), geom));
}

RTGEOM* wkt_parser_collection_finalize(int rttype, RTGEOM *geom, char *dimensionality) 
{
	uint8_t flags = wkt_dimensionality(dimensionality);
	int flagdims = FLAGS_NDIMS(flags);
	
	/* No geometry means it is empty */
	if( ! geom )
	{
		return rtcollection_as_rtgeom(rtcollection_construct_empty(rttype, SRID_UNKNOWN, FLAGS_GET_Z(flags), FLAGS_GET_M(flags)));
	}

	/* There are 'Z' or 'M' tokens in the signature */
	if ( flagdims > 2 )
	{
		RTCOLLECTION *col = rtgeom_as_rtcollection(geom);
		int i;
		
		for ( i = 0 ; i < col->ngeoms; i++ )
		{
			RTGEOM *subgeom = col->geoms[i];
			if ( FLAGS_NDIMS(flags) != FLAGS_NDIMS(subgeom->flags) &&
				 ! rtgeom_is_empty(subgeom) )
			{
				rtgeom_free(geom);
				SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
				return NULL;
			}
			
			if ( rttype == RTCOLLECTIONTYPE &&
			   ( (FLAGS_GET_Z(flags) != FLAGS_GET_Z(subgeom->flags)) ||
			     (FLAGS_GET_M(flags) != FLAGS_GET_M(subgeom->flags)) ) &&
				! rtgeom_is_empty(subgeom) )
			{
				rtgeom_free(geom);
				SET_PARSER_ERROR(PARSER_ERROR_MIXDIMS);
				return NULL;
			}
		}
		
		/* Harmonize the collection dimensionality */
		if( RT_FAILURE == wkt_parser_set_dims(geom, flags) )
		{
			rtgeom_free(geom);
			SET_PARSER_ERROR(PARSER_ERROR_OTHER);
			return NULL;
		}
	}
		
	/* Set the collection type */
	geom->type = rttype;
			
	return geom;
}

void wkt_parser_geometry_new(RTGEOM *geom, int srid)
{
	RTDEBUG(4,"entered");
	RTDEBUGF(4,"geom %p",geom);
	RTDEBUGF(4,"srid %d",srid);

	if ( geom == NULL ) 
	{
		rterror("Parsed geometry is null!");
		return;
	}
		
	if ( srid != SRID_UNKNOWN && srid < SRID_MAXIMUM )
		rtgeom_set_srid(geom, srid);
	else
		rtgeom_set_srid(geom, SRID_UNKNOWN);
	
	global_parser_result.geom = geom;
}

void rtgeom_parser_result_init(RTGEOM_PARSER_RESULT *parser_result)
{
	memset(parser_result, 0, sizeof(RTGEOM_PARSER_RESULT));
}


void rtgeom_parser_result_free(RTGEOM_PARSER_RESULT *parser_result)
{
	if ( parser_result->geom )
	{
		rtgeom_free(parser_result->geom);
		parser_result->geom = 0;
	}
	if ( parser_result->serialized_rtgeom )
	{
		rtfree(parser_result->serialized_rtgeom );
		parser_result->serialized_rtgeom = 0;
	}
	/* We don't free parser_result->message because
	   it is a const *char */
}

/*
* Public function used for easy access to the parser.
*/
RTGEOM *rtgeom_from_wkt(const char *wkt, const char check)
{
	RTGEOM_PARSER_RESULT r;

	if( RT_FAILURE == rtgeom_parse_wkt(&r, (char*)wkt, check) )
	{
		rterror(r.message);
		return NULL;
	}
	
	return r.geom;	
}


