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
#include "librtgeom_internal.h"
#include "rtgeom_log.h"


#define CHECK_RTGEOM_ZM 1

void
rtcollection_release(RTCOLLECTION *rtcollection)
{
	rtgeom_release(rtcollection_as_rtgeom(rtcollection));
}


RTCOLLECTION *
rtcollection_construct(uint8_t type, int srid, RTGBOX *bbox,
                       uint32_t ngeoms, RTGEOM **geoms)
{
	RTCOLLECTION *ret;
	int hasz, hasm;
#ifdef CHECK_RTGEOM_ZM
	char zm;
	uint32_t i;
#endif

	RTDEBUGF(2, "rtcollection_construct called with %d, %d, %p, %d, %p.", type, srid, bbox, ngeoms, geoms);

	if( ! rttype_is_collection(type) )
		rterror("Non-collection type specified in collection constructor!");

	hasz = 0;
	hasm = 0;
	if ( ngeoms > 0 )
	{
		hasz = FLAGS_GET_Z(geoms[0]->flags);
		hasm = FLAGS_GET_M(geoms[0]->flags);
#ifdef CHECK_RTGEOM_ZM
		zm = FLAGS_GET_ZM(geoms[0]->flags);

		RTDEBUGF(3, "rtcollection_construct type[0]=%d", geoms[0]->type);

		for (i=1; i<ngeoms; i++)
		{
			RTDEBUGF(3, "rtcollection_construct type=[%d]=%d", i, geoms[i]->type);

			if ( zm != FLAGS_GET_ZM(geoms[i]->flags) )
				rterror("rtcollection_construct: mixed dimension geometries: %d/%d", zm, FLAGS_GET_ZM(geoms[i]->flags));
		}
#endif
	}


	ret = rtalloc(sizeof(RTCOLLECTION));
	ret->type = type;
	ret->flags = gflags(hasz,hasm,0);
	FLAGS_SET_BBOX(ret->flags, bbox?1:0);
	ret->srid = srid;
	ret->ngeoms = ngeoms;
	ret->maxgeoms = ngeoms;
	ret->geoms = geoms;
	ret->bbox = bbox;

	return ret;
}

RTCOLLECTION *
rtcollection_construct_empty(uint8_t type, int srid, char hasz, char hasm)
{
	RTCOLLECTION *ret;
	if( ! rttype_is_collection(type) )
		rterror("Non-collection type specified in collection constructor!");

	ret = rtalloc(sizeof(RTCOLLECTION));
	ret->type = type;
	ret->flags = gflags(hasz,hasm,0);
	ret->srid = srid;
	ret->ngeoms = 0;
	ret->maxgeoms = 1; /* Allocate room for sub-members, just in case. */
	ret->geoms = rtalloc(ret->maxgeoms * sizeof(RTGEOM*));
	ret->bbox = NULL;

	return ret;
}

RTGEOM *
rtcollection_getsubgeom(RTCOLLECTION *col, int gnum)
{
	return (RTGEOM *)col->geoms[gnum];
}

/**
 * @brief Clone #RTCOLLECTION object. #RTPOINTARRAY are not copied.
 * 			Bbox is cloned if present in input.
 */
RTCOLLECTION *
rtcollection_clone(const RTCOLLECTION *g)
{
	uint32_t i;
	RTCOLLECTION *ret = rtalloc(sizeof(RTCOLLECTION));
	memcpy(ret, g, sizeof(RTCOLLECTION));
	if ( g->ngeoms > 0 )
	{
		ret->geoms = rtalloc(sizeof(RTGEOM *)*g->ngeoms);
		for (i=0; i<g->ngeoms; i++)
		{
			ret->geoms[i] = rtgeom_clone(g->geoms[i]);
		}
		if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	}
	else
	{
		ret->bbox = NULL; /* empty collection */
		ret->geoms = NULL;
	}
	return ret;
}

/**
* @brief Deep clone #RTCOLLECTION object. #RTPOINTARRAY are copied.
*/
RTCOLLECTION *
rtcollection_clone_deep(const RTCOLLECTION *g)
{
	uint32_t i;
	RTCOLLECTION *ret = rtalloc(sizeof(RTCOLLECTION));
	memcpy(ret, g, sizeof(RTCOLLECTION));
	if ( g->ngeoms > 0 )
	{
		ret->geoms = rtalloc(sizeof(RTGEOM *)*g->ngeoms);
		for (i=0; i<g->ngeoms; i++)
		{
			ret->geoms[i] = rtgeom_clone_deep(g->geoms[i]);
		}
		if ( g->bbox ) ret->bbox = gbox_copy(g->bbox);
	}
	else
	{
		ret->bbox = NULL; /* empty collection */
		ret->geoms = NULL;
	}
	return ret;
}

/**
 * Ensure the collection can hold up at least ngeoms
 */
void rtcollection_reserve(RTCOLLECTION *col, int ngeoms)
{
	if ( ngeoms <= col->maxgeoms ) return;

	/* Allocate more space if we need it */
	do { col->maxgeoms *= 2; } while ( col->maxgeoms < ngeoms );
	col->geoms = rtrealloc(col->geoms, sizeof(RTGEOM*) * col->maxgeoms);
}

/**
* Appends geom to the collection managed by col. Does not copy or
* clone, simply takes a reference on the passed geom.
*/
RTCOLLECTION* rtcollection_add_rtgeom(RTCOLLECTION *col, const RTGEOM *geom)
{
	if ( col == NULL || geom == NULL ) return NULL;

	if ( col->geoms == NULL && (col->ngeoms || col->maxgeoms) ) {
		rterror("Collection is in inconsistent state. Null memory but non-zero collection counts.");
		return NULL;
	}

	/* Check type compatibility */
	if ( ! rtcollection_allows_subtype(col->type, geom->type) ) {
		rterror("%s cannot contain %s element", rttype_name(col->type), rttype_name(geom->type));
		return NULL;
	}

	/* In case this is a truly empty, make some initial space  */
	if ( col->geoms == NULL )
	{
		col->maxgeoms = 2;
		col->ngeoms = 0;
		col->geoms = rtalloc(col->maxgeoms * sizeof(RTGEOM*));
	}

	/* Allocate more space if we need it */
	rtcollection_reserve(col, col->ngeoms + 1);

#if PARANOIA_LEVEL > 1
	/* See http://trac.osgeo.org/postgis/ticket/2933 */
	/* Make sure we don't already have a reference to this geom */
	{
	int i = 0;
	for ( i = 0; i < col->ngeoms; i++ )
	{
		if ( col->geoms[i] == geom )
		{
			RTDEBUGF(4, "Found duplicate geometry in collection %p == %p", col->geoms[i], geom);
			return col;
		}
	}
	}
#endif

	col->geoms[col->ngeoms] = (RTGEOM*)geom;
	col->ngeoms++;
	return col;
}


RTCOLLECTION *
rtcollection_segmentize2d(RTCOLLECTION *col, double dist)
{
	uint32_t i;
	RTGEOM **newgeoms;

	if ( ! col->ngeoms ) return rtcollection_clone(col);

	newgeoms = rtalloc(sizeof(RTGEOM *)*col->ngeoms);
	for (i=0; i<col->ngeoms; i++)
	{
		newgeoms[i] = rtgeom_segmentize2d(col->geoms[i], dist);
		if ( ! newgeoms[i] ) {
			while (i--) rtgeom_free(newgeoms[i]);
			rtfree(newgeoms);
			return NULL;
		}
	}

	return rtcollection_construct(col->type, col->srid, NULL, col->ngeoms, newgeoms);
}

/** @brief check for same geometry composition
 *
 */
char
rtcollection_same(const RTCOLLECTION *c1, const RTCOLLECTION *c2)
{
	uint32_t i;

	RTDEBUG(2, "rtcollection_same called");

	if ( c1->type != c2->type ) return RT_FALSE;
	if ( c1->ngeoms != c2->ngeoms ) return RT_FALSE;

	for ( i = 0; i < c1->ngeoms; i++ )
	{
		if ( ! rtgeom_same(c1->geoms[i], c2->geoms[i]) )
			return RT_FALSE;
	}

	/* Former method allowed out-of-order equality between collections

		hit = rtalloc(sizeof(uint32_t)*c1->ngeoms);
		memset(hit, 0, sizeof(uint32_t)*c1->ngeoms);

		for (i=0; i<c1->ngeoms; i++)
		{
			char found=0;
			for (j=0; j<c2->ngeoms; j++)
			{
				if ( hit[j] ) continue;
				if ( rtgeom_same(c1->geoms[i], c2->geoms[j]) )
				{
					hit[j] = 1;
					found=1;
					break;
				}
			}
			if ( ! found ) return RT_FALSE;
		}
	*/

	return RT_TRUE;
}

int rtcollection_ngeoms(const RTCOLLECTION *col)
{
	int i;
	int ngeoms = 0;

	if ( ! col )
	{
		rterror("Null input geometry.");
		return 0;
	}

	for ( i = 0; i < col->ngeoms; i++ )
	{
		if ( col->geoms[i])
		{
			switch (col->geoms[i]->type)
			{
			case RTPOINTTYPE:
			case RTLINETYPE:
			case RTCIRCSTRINGTYPE:
			case RTPOLYGONTYPE:
				ngeoms += 1;
				break;
			case RTMULTIPOINTTYPE:
			case RTMULTILINETYPE:
			case RTMULTICURVETYPE:
			case RTMULTIPOLYGONTYPE:
				ngeoms += col->ngeoms;
				break;
			case RTCOLLECTIONTYPE:
				ngeoms += rtcollection_ngeoms((RTCOLLECTION*)col->geoms[i]);
				break;
			}
		}
	}
	return ngeoms;
}

void rtcollection_free(RTCOLLECTION *col)
{
	int i;
	if ( ! col ) return;
	
	if ( col->bbox )
	{
		rtfree(col->bbox);
	}
	for ( i = 0; i < col->ngeoms; i++ )
	{
		RTDEBUGF(4,"freeing geom[%d]", i);
		if ( col->geoms && col->geoms[i] )
			rtgeom_free(col->geoms[i]);
	}
	if ( col->geoms )
	{
		rtfree(col->geoms);
	}
	rtfree(col);
}


/**
* Takes a potentially heterogeneous collection and returns a homogeneous
* collection consisting only of the specified type.
*/
RTCOLLECTION* rtcollection_extract(RTCOLLECTION *col, int type)
{
	int i = 0;
	RTGEOM **geomlist;
	RTCOLLECTION *outcol;
	int geomlistsize = 16;
	int geomlistlen = 0;
	uint8_t outtype;

	if ( ! col ) return NULL;

	switch (type)
	{
	case RTPOINTTYPE:
		outtype = RTMULTIPOINTTYPE;
		break;
	case RTLINETYPE:
		outtype = RTMULTILINETYPE;
		break;
	case RTPOLYGONTYPE:
		outtype = RTMULTIPOLYGONTYPE;
		break;
	default:
		rterror("Only POLYGON, LINESTRING and POINT are supported by rtcollection_extract. %s requested.", rttype_name(type));
		return NULL;
	}

	geomlist = rtalloc(sizeof(RTGEOM*) * geomlistsize);

	/* Process each sub-geometry */
	for ( i = 0; i < col->ngeoms; i++ )
	{
		int subtype = col->geoms[i]->type;
		/* Don't bother adding empty sub-geometries */
		if ( rtgeom_is_empty(col->geoms[i]) )
		{
			continue;
		}
		/* Copy our sub-types into the output list */
		if ( subtype == type )
		{
			/* We've over-run our buffer, double the memory segment */
			if ( geomlistlen == geomlistsize )
			{
				geomlistsize *= 2;
				geomlist = rtrealloc(geomlist, sizeof(RTGEOM*) * geomlistsize);
			}
			geomlist[geomlistlen] = rtgeom_clone(col->geoms[i]);
			geomlistlen++;
		}
		/* Recurse into sub-collections */
		if ( rttype_is_collection( subtype ) )
		{
			int j = 0;
			RTCOLLECTION *tmpcol = rtcollection_extract((RTCOLLECTION*)col->geoms[i], type);
			for ( j = 0; j < tmpcol->ngeoms; j++ )
			{
				/* We've over-run our buffer, double the memory segment */
				if ( geomlistlen == geomlistsize )
				{
					geomlistsize *= 2;
					geomlist = rtrealloc(geomlist, sizeof(RTGEOM*) * geomlistsize);
				}
				geomlist[geomlistlen] = tmpcol->geoms[j];
				geomlistlen++;
			}
			rtfree(tmpcol);
		}
	}

	if ( geomlistlen > 0 )
	{
		RTGBOX gbox;
		outcol = rtcollection_construct(outtype, col->srid, NULL, geomlistlen, geomlist);
		rtgeom_calculate_gbox((RTGEOM *) outcol, &gbox);
		outcol->bbox = gbox_copy(&gbox);
	}
	else
	{
		rtfree(geomlist);
		outcol = rtcollection_construct_empty(outtype, col->srid, FLAGS_GET_Z(col->flags), FLAGS_GET_M(col->flags));
	}

	return outcol;
}

RTGEOM*
rtcollection_remove_repeated_points(const RTCOLLECTION *coll, double tolerance)
{
	uint32_t i;
	RTGEOM **newgeoms;

	newgeoms = rtalloc(sizeof(RTGEOM *)*coll->ngeoms);
	for (i=0; i<coll->ngeoms; i++)
	{
		newgeoms[i] = rtgeom_remove_repeated_points(coll->geoms[i], tolerance);
	}

	return (RTGEOM*)rtcollection_construct(coll->type,
	                                       coll->srid, coll->bbox ? gbox_copy(coll->bbox) : NULL,
	                                       coll->ngeoms, newgeoms);
}


RTCOLLECTION*
rtcollection_force_dims(const RTCOLLECTION *col, int hasz, int hasm)
{
	RTCOLLECTION *colout;
	
	/* Return 2D empty */
	if( rtcollection_is_empty(col) )
	{
		colout = rtcollection_construct_empty(col->type, col->srid, hasz, hasm);
	}
	else
	{
		int i;
		RTGEOM **geoms = NULL;
		geoms = rtalloc(sizeof(RTGEOM*) * col->ngeoms);
		for( i = 0; i < col->ngeoms; i++ )
		{
			geoms[i] = rtgeom_force_dims(col->geoms[i], hasz, hasm);
		}
		colout = rtcollection_construct(col->type, col->srid, NULL, col->ngeoms, geoms);
	}
	return colout;
}

int rtcollection_is_empty(const RTCOLLECTION *col)
{
	int i;
	if ( (col->ngeoms == 0) || (!col->geoms) )
		return RT_TRUE;
	for( i = 0; i < col->ngeoms; i++ )
	{
		if ( ! rtgeom_is_empty(col->geoms[i]) ) return RT_FALSE;
	}
	return RT_TRUE;
}


int rtcollection_count_vertices(RTCOLLECTION *col)
{
	int i = 0;
	int v = 0; /* vertices */
	assert(col);
	for ( i = 0; i < col->ngeoms; i++ )
	{
		v += rtgeom_count_vertices(col->geoms[i]);
	}
	return v;
}

RTCOLLECTION* rtcollection_simplify(const RTCOLLECTION *igeom, double dist, int preserve_collapsed)
{
 	int i;
	RTCOLLECTION *out = rtcollection_construct_empty(igeom->type, igeom->srid, FLAGS_GET_Z(igeom->flags), FLAGS_GET_M(igeom->flags));

	if( rtcollection_is_empty(igeom) )
		return out; /* should we return NULL instead ? */

	for( i = 0; i < igeom->ngeoms; i++ )
	{
		RTGEOM *ngeom = rtgeom_simplify(igeom->geoms[i], dist, preserve_collapsed);
		if ( ngeom ) out = rtcollection_add_rtgeom(out, ngeom);
	}

	return out;
}

int rtcollection_allows_subtype(int collectiontype, int subtype)
{
	if ( collectiontype == RTCOLLECTIONTYPE )
		return RT_TRUE;
	if ( collectiontype == RTMULTIPOINTTYPE &&
	        subtype == RTPOINTTYPE )
		return RT_TRUE;
	if ( collectiontype == RTMULTILINETYPE &&
	        subtype == RTLINETYPE )
		return RT_TRUE;
	if ( collectiontype == RTMULTIPOLYGONTYPE &&
	        subtype == RTPOLYGONTYPE )
		return RT_TRUE;
	if ( collectiontype == RTCOMPOUNDTYPE &&
	        (subtype == RTLINETYPE || subtype == RTCIRCSTRINGTYPE) )
		return RT_TRUE;
	if ( collectiontype == RTCURVEPOLYTYPE &&
	        (subtype == RTCIRCSTRINGTYPE || subtype == RTLINETYPE || subtype == RTCOMPOUNDTYPE) )
		return RT_TRUE;
	if ( collectiontype == RTMULTICURVETYPE &&
	        (subtype == RTCIRCSTRINGTYPE || subtype == RTLINETYPE || subtype == RTCOMPOUNDTYPE) )
		return RT_TRUE;
	if ( collectiontype == RTMULTISURFACETYPE &&
	        (subtype == RTPOLYGONTYPE || subtype == RTCURVEPOLYTYPE) )
		return RT_TRUE;
	if ( collectiontype == RTPOLYHEDRALSURFACETYPE &&
	        subtype == RTPOLYGONTYPE )
		return RT_TRUE;
	if ( collectiontype == RTTINTYPE &&
	        subtype == RTTRIANGLETYPE )
		return RT_TRUE;

	/* Must be a bad combination! */
	return RT_FALSE;
}

int
rtcollection_startpoint(const RTCOLLECTION* col, RTPOINT4D* pt)
{
	if ( col->ngeoms < 1 )
		return RT_FAILURE;
		
	return rtgeom_startpoint(col->geoms[0], pt);
}


RTCOLLECTION* rtcollection_grid(const RTCOLLECTION *coll, const gridspec *grid)
{
	uint32_t i;
	RTCOLLECTION *newcoll;
	
	newcoll = rtcollection_construct_empty(coll->type, coll->srid, rtgeom_has_z((RTGEOM*)coll), rtgeom_has_m((RTGEOM*)coll));

	for (i=0; i<coll->ngeoms; i++)
	{
		RTGEOM *g = rtgeom_grid(coll->geoms[i], grid);
		if ( g ) 
			rtcollection_add_rtgeom(newcoll, g);
	}

	return newcoll;
}
