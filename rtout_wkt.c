/**********************************************************************
 *
 * rttopo - topology library
 *
 * Copyright (C) 2009 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "librtgeom_internal.h"
#include "rtgeom_log.h"
#include "stringbuffer.h"

static void rtgeom_to_wkt_sb(const RTGEOM *geom, stringbuffer_t *sb, int precision, uint8_t variant);


/*
* ISO format uses both Z and M qualifiers.
* Extended format only uses an M qualifier for 3DM variants, where it is not
* clear what the third dimension represents.
* SFSQL format never has more than two dimensions, so no qualifiers.
*/
static void dimension_qualifiers_to_wkt_sb(const RTGEOM *geom, stringbuffer_t *sb, uint8_t variant)
{

	/* Extended RTWKT: POINTM(0 0 0) */
#if 0
	if ( (variant & RTWKT_EXTENDED) && ! (variant & RTWKT_IS_CHILD) && RTFLAGS_GET_M(geom->flags) && (!RTFLAGS_GET_Z(geom->flags)) )
#else
	if ( (variant & RTWKT_EXTENDED) && RTFLAGS_GET_M(geom->flags) && (!RTFLAGS_GET_Z(geom->flags)) )
#endif
	{
		stringbuffer_append(sb, "M"); /* "M" */
		return;
	}

	/* ISO RTWKT: POINT ZM (0 0 0 0) */
	if ( (variant & RTWKT_ISO) && (RTFLAGS_NDIMS(geom->flags) > 2) )
	{
		stringbuffer_append(sb, " ");
		if ( RTFLAGS_GET_Z(geom->flags) )
			stringbuffer_append(sb, "Z");
		if ( RTFLAGS_GET_M(geom->flags) )
			stringbuffer_append(sb, "M");
		stringbuffer_append(sb, " ");
	}
}

/*
* Write an empty token out, padding with a space if
* necessary. 
*/
static void empty_to_wkt_sb(stringbuffer_t *sb)
{
	if ( ! strchr(" ,(", stringbuffer_lastchar(sb)) ) /* "EMPTY" */
	{ 
		stringbuffer_append(sb, " "); 
	}
	stringbuffer_append(sb, "EMPTY"); 
}

/*
* Point array is a list of coordinates. Depending on output mode,
* we may suppress some dimensions. ISO and Extended formats include
* all dimensions. Standard OGC output only includes X/Y coordinates.
*/
static void ptarray_to_wkt_sb(const RTPOINTARRAY *ptarray, stringbuffer_t *sb, int precision, uint8_t variant)
{
	/* OGC only includes X/Y */
	int dimensions = 2;
	int i, j;

	/* ISO and extended formats include all dimensions */
	if ( variant & ( RTWKT_ISO | RTWKT_EXTENDED ) )
		dimensions = RTFLAGS_NDIMS(ptarray->flags);

	/* Opening paren? */
	if ( ! (variant & RTWKT_NO_PARENS) )
		stringbuffer_append(sb, "(");

	/* Digits and commas */
	for (i = 0; i < ptarray->npoints; i++)
	{
		double *dbl_ptr = (double*)getPoint_internal(ptarray, i);

		/* Commas before ever coord but the first */
		if ( i > 0 )
			stringbuffer_append(sb, ",");

		for (j = 0; j < dimensions; j++)
		{
			/* Spaces before every ordinate but the first */
			if ( j > 0 )
				stringbuffer_append(sb, " ");
			stringbuffer_aprintf(sb, "%.*g", precision, dbl_ptr[j]);
		}
	}

	/* Closing paren? */
	if ( ! (variant & RTWKT_NO_PARENS) )
		stringbuffer_append(sb, ")");
}

/*
* A four-dimensional point will have different outputs depending on variant.
*   ISO: POINT ZM (0 0 0 0)
*   Extended: POINT(0 0 0 0)
*   OGC: POINT(0 0)
* A three-dimensional m-point will have different outputs too.
*   ISO: POINT M (0 0 0)
*   Extended: POINTM(0 0 0)
*   OGC: POINT(0 0)
*/
static void rtpoint_to_wkt_sb(const RTPOINT *pt, stringbuffer_t *sb, int precision, uint8_t variant)
{
	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "POINT"); /* "POINT" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)pt, sb, variant);
	}

	if ( rtpoint_is_empty(pt) )
	{
		empty_to_wkt_sb(sb);
		return;
	}

	ptarray_to_wkt_sb(pt->point, sb, precision, variant);
}

/*
* LINESTRING(0 0 0, 1 1 1)
*/
static void rtline_to_wkt_sb(const RTLINE *line, stringbuffer_t *sb, int precision, uint8_t variant)
{
	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "LINESTRING"); /* "LINESTRING" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)line, sb, variant);
	}
	if ( rtline_is_empty(line) )
	{  
		empty_to_wkt_sb(sb);
		return;
	}

	ptarray_to_wkt_sb(line->points, sb, precision, variant);
}

/*
* POLYGON(0 0 1, 1 0 1, 1 1 1, 0 1 1, 0 0 1)
*/
static void rtpoly_to_wkt_sb(const RTPOLY *poly, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;
	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "POLYGON"); /* "POLYGON" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)poly, sb, variant);
	}
	if ( rtpoly_is_empty(poly) )
	{
		empty_to_wkt_sb(sb);
		return;
	}

	stringbuffer_append(sb, "(");
	for ( i = 0; i < poly->nrings; i++ )
	{
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		ptarray_to_wkt_sb(poly->rings[i], sb, precision, variant);
	}
	stringbuffer_append(sb, ")");
}

/*
* CIRCULARSTRING
*/
static void rtcircstring_to_wkt_sb(const RTCIRCSTRING *circ, stringbuffer_t *sb, int precision, uint8_t variant)
{
	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "CIRCULARSTRING"); /* "CIRCULARSTRING" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)circ, sb, variant);
	}
	if ( rtcircstring_is_empty(circ) )
	{
		empty_to_wkt_sb(sb);
		return;
	}
	ptarray_to_wkt_sb(circ->points, sb, precision, variant);
}


/*
* Multi-points do not wrap their sub-members in parens, unlike other multi-geometries.
*   MULTPOINT(0 0, 1 1) instead of MULTIPOINT((0 0),(1 1))
*/
static void rtmpoint_to_wkt_sb(const RTMPOINT *mpoint, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;
	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "MULTIPOINT"); /* "MULTIPOINT" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)mpoint, sb, variant);
	}
	if ( mpoint->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}
	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
	for ( i = 0; i < mpoint->ngeoms; i++ )
	{
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		/* We don't want type strings or parens on our subgeoms */
		rtpoint_to_wkt_sb(mpoint->geoms[i], sb, precision, variant | RTWKT_NO_PARENS | RTWKT_NO_TYPE );
	}
	stringbuffer_append(sb, ")");
}

/*
* MULTILINESTRING
*/
static void rtmline_to_wkt_sb(const RTMLINE *mline, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "MULTILINESTRING"); /* "MULTILINESTRING" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)mline, sb, variant);
	}
	if ( mline->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}

	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
	for ( i = 0; i < mline->ngeoms; i++ )
	{
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		/* We don't want type strings on our subgeoms */
		rtline_to_wkt_sb(mline->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
	}
	stringbuffer_append(sb, ")");
}

/*
* MULTIPOLYGON
*/
static void rtmpoly_to_wkt_sb(const RTMPOLY *mpoly, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "MULTIPOLYGON"); /* "MULTIPOLYGON" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)mpoly, sb, variant);
	}
	if ( mpoly->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}

	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
	for ( i = 0; i < mpoly->ngeoms; i++ )
	{
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		/* We don't want type strings on our subgeoms */
		rtpoly_to_wkt_sb(mpoly->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
	}
	stringbuffer_append(sb, ")");
}

/*
* Compound curves provide type information for their curved sub-geometries
* but not their linestring sub-geometries.
*   COMPOUNDCURVE((0 0, 1 1), CURVESTRING(1 1, 2 2, 3 3))
*/
static void rtcompound_to_wkt_sb(const RTCOMPOUND *comp, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "COMPOUNDCURVE"); /* "COMPOUNDCURVE" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)comp, sb, variant);
	}
	if ( comp->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}

	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
	for ( i = 0; i < comp->ngeoms; i++ )
	{
		int type = comp->geoms[i]->type;
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		/* Linestring subgeoms don't get type identifiers */
		if ( type == RTLINETYPE )
		{
			rtline_to_wkt_sb((RTLINE*)comp->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
		}
		/* But circstring subgeoms *do* get type identifiers */
		else if ( type == RTCIRCSTRINGTYPE )
		{
			rtcircstring_to_wkt_sb((RTCIRCSTRING*)comp->geoms[i], sb, precision, variant );
		}
		else
		{
			rterror("rtcompound_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(type));
		}
	}
	stringbuffer_append(sb, ")");
}

/*
* Curve polygons provide type information for their curved rings
* but not their linestring rings.
*   CURVEPOLYGON((0 0, 1 1, 0 1, 0 0), CURVESTRING(0 0, 1 1, 0 1, 0.5 1, 0 0))
*/
static void rtcurvepoly_to_wkt_sb(const RTCURVEPOLY *cpoly, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "CURVEPOLYGON"); /* "CURVEPOLYGON" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)cpoly, sb, variant);
	}
	if ( cpoly->nrings < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}
	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
	for ( i = 0; i < cpoly->nrings; i++ )
	{
		int type = cpoly->rings[i]->type;
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		switch (type)
		{
		case RTLINETYPE:
			/* Linestring subgeoms don't get type identifiers */
			rtline_to_wkt_sb((RTLINE*)cpoly->rings[i], sb, precision, variant | RTWKT_NO_TYPE );
			break;
		case RTCIRCSTRINGTYPE:
			/* But circstring subgeoms *do* get type identifiers */
			rtcircstring_to_wkt_sb((RTCIRCSTRING*)cpoly->rings[i], sb, precision, variant );
			break;
		case RTCOMPOUNDTYPE:
			/* And compoundcurve subgeoms *do* get type identifiers */
			rtcompound_to_wkt_sb((RTCOMPOUND*)cpoly->rings[i], sb, precision, variant );
			break;
		default:
			rterror("rtcurvepoly_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(type));
		}
	}
	stringbuffer_append(sb, ")");
}


/*
* Multi-curves provide type information for their curved sub-geometries
* but not their linear sub-geometries.
*   MULTICURVE((0 0, 1 1), CURVESTRING(0 0, 1 1, 2 2))
*/
static void rtmcurve_to_wkt_sb(const RTMCURVE *mcurv, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "MULTICURVE"); /* "MULTICURVE" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)mcurv, sb, variant);
	}
	if ( mcurv->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}
	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
	for ( i = 0; i < mcurv->ngeoms; i++ )
	{
		int type = mcurv->geoms[i]->type;
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		switch (type)
		{
		case RTLINETYPE:
			/* Linestring subgeoms don't get type identifiers */
			rtline_to_wkt_sb((RTLINE*)mcurv->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
			break;
		case RTCIRCSTRINGTYPE:
			/* But circstring subgeoms *do* get type identifiers */
			rtcircstring_to_wkt_sb((RTCIRCSTRING*)mcurv->geoms[i], sb, precision, variant );
			break;
		case RTCOMPOUNDTYPE:
			/* And compoundcurve subgeoms *do* get type identifiers */
			rtcompound_to_wkt_sb((RTCOMPOUND*)mcurv->geoms[i], sb, precision, variant );
			break;
		default:
			rterror("rtmcurve_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(type));
		}
	}
	stringbuffer_append(sb, ")");
}


/*
* Multi-surfaces provide type information for their curved sub-geometries
* but not their linear sub-geometries.
*   MULTISURFACE(((0 0, 1 1, 1 0, 0 0)), CURVEPOLYGON(CURVESTRING(0 0, 1 1, 2 2, 0 1, 0 0)))
*/
static void rtmsurface_to_wkt_sb(const RTMSURFACE *msurf, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "MULTISURFACE"); /* "MULTISURFACE" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)msurf, sb, variant);
	}
	if ( msurf->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}
	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */
	for ( i = 0; i < msurf->ngeoms; i++ )
	{
		int type = msurf->geoms[i]->type;
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		switch (type)
		{
		case RTPOLYGONTYPE:
			/* Linestring subgeoms don't get type identifiers */
			rtpoly_to_wkt_sb((RTPOLY*)msurf->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
			break;
		case RTCURVEPOLYTYPE:
			/* But circstring subgeoms *do* get type identifiers */
			rtcurvepoly_to_wkt_sb((RTCURVEPOLY*)msurf->geoms[i], sb, precision, variant);
			break;
		default:
			rterror("rtmsurface_to_wkt_sb: Unknown type received %d - %s", type, rttype_name(type));
		}
	}
	stringbuffer_append(sb, ")");
}

/*
* Geometry collections provide type information for all their curved sub-geometries
* but not their linear sub-geometries.
*   GEOMETRYCOLLECTION(POLYGON((0 0, 1 1, 1 0, 0 0)), CURVEPOLYGON(CURVESTRING(0 0, 1 1, 2 2, 0 1, 0 0)))
*/
static void rtcollection_to_wkt_sb(const RTCOLLECTION *collection, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "GEOMETRYCOLLECTION"); /* "GEOMETRYCOLLECTION" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)collection, sb, variant);
	}
	if ( collection->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}
	stringbuffer_append(sb, "(");
	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are children */
	for ( i = 0; i < collection->ngeoms; i++ )
	{
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		rtgeom_to_wkt_sb((RTGEOM*)collection->geoms[i], sb, precision, variant );
	}
	stringbuffer_append(sb, ")");
}

/*
* TRIANGLE 
*/
static void rttriangle_to_wkt_sb(const RTTRIANGLE *tri, stringbuffer_t *sb, int precision, uint8_t variant)
{
	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "TRIANGLE"); /* "TRIANGLE" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)tri, sb, variant);
	}
	if ( rttriangle_is_empty(tri) )
	{  
		empty_to_wkt_sb(sb);
		return;
	}

	stringbuffer_append(sb, "("); /* Triangles have extraneous brackets */
	ptarray_to_wkt_sb(tri->points, sb, precision, variant);
	stringbuffer_append(sb, ")"); 
}

/*
* TIN
*/
static void rttin_to_wkt_sb(const RTTIN *tin, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "TIN"); /* "TIN" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)tin, sb, variant);
	}
	if ( tin->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}

	stringbuffer_append(sb, "(");
	for ( i = 0; i < tin->ngeoms; i++ )
	{
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		/* We don't want type strings on our subgeoms */
		rttriangle_to_wkt_sb(tin->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
	}
	stringbuffer_append(sb, ")");
}

/*
* POLYHEDRALSURFACE
*/
static void rtpsurface_to_wkt_sb(const RTPSURFACE *psurf, stringbuffer_t *sb, int precision, uint8_t variant)
{
	int i = 0;

	if ( ! (variant & RTWKT_NO_TYPE) )
	{
		stringbuffer_append(sb, "POLYHEDRALSURFACE"); /* "POLYHEDRALSURFACE" */
		dimension_qualifiers_to_wkt_sb((RTGEOM*)psurf, sb, variant);
	}
	if ( psurf->ngeoms < 1 )
	{
		empty_to_wkt_sb(sb);
		return;
	}

	variant = variant | RTWKT_IS_CHILD; /* Inform the sub-geometries they are childre */

	stringbuffer_append(sb, "(");
	for ( i = 0; i < psurf->ngeoms; i++ )
	{
		if ( i > 0 )
			stringbuffer_append(sb, ",");
		/* We don't want type strings on our subgeoms */
		rtpoly_to_wkt_sb(psurf->geoms[i], sb, precision, variant | RTWKT_NO_TYPE );
	}
	stringbuffer_append(sb, ")");	
}


/*
* Generic GEOMETRY
*/
static void rtgeom_to_wkt_sb(const RTGEOM *geom, stringbuffer_t *sb, int precision, uint8_t variant)
{
	RTDEBUGF(4, "rtgeom_to_wkt_sb: type %s, hasz %d, hasm %d",
		rttype_name(geom->type), (geom->type),
		RTFLAGS_GET_Z(geom->flags)?1:0, RTFLAGS_GET_M(geom->flags)?1:0);

	switch (geom->type)
	{
	case RTPOINTTYPE:
		rtpoint_to_wkt_sb((RTPOINT*)geom, sb, precision, variant);
		break;
	case RTLINETYPE:
		rtline_to_wkt_sb((RTLINE*)geom, sb, precision, variant);
		break;
	case RTPOLYGONTYPE:
		rtpoly_to_wkt_sb((RTPOLY*)geom, sb, precision, variant);
		break;
	case RTMULTIPOINTTYPE:
		rtmpoint_to_wkt_sb((RTMPOINT*)geom, sb, precision, variant);
		break;
	case RTMULTILINETYPE:
		rtmline_to_wkt_sb((RTMLINE*)geom, sb, precision, variant);
		break;
	case RTMULTIPOLYGONTYPE:
		rtmpoly_to_wkt_sb((RTMPOLY*)geom, sb, precision, variant);
		break;
	case RTCOLLECTIONTYPE:
		rtcollection_to_wkt_sb((RTCOLLECTION*)geom, sb, precision, variant);
		break;
	case RTCIRCSTRINGTYPE:
		rtcircstring_to_wkt_sb((RTCIRCSTRING*)geom, sb, precision, variant);
		break;
	case RTCOMPOUNDTYPE:
		rtcompound_to_wkt_sb((RTCOMPOUND*)geom, sb, precision, variant);
		break;
	case RTCURVEPOLYTYPE:
		rtcurvepoly_to_wkt_sb((RTCURVEPOLY*)geom, sb, precision, variant);
		break;
	case RTMULTICURVETYPE:
		rtmcurve_to_wkt_sb((RTMCURVE*)geom, sb, precision, variant);
		break;
	case RTMULTISURFACETYPE:
		rtmsurface_to_wkt_sb((RTMSURFACE*)geom, sb, precision, variant);
		break;
	case RTTRIANGLETYPE:
		rttriangle_to_wkt_sb((RTTRIANGLE*)geom, sb, precision, variant);
		break;
	case RTTINTYPE:
		rttin_to_wkt_sb((RTTIN*)geom, sb, precision, variant);
		break;
	case RTPOLYHEDRALSURFACETYPE:
		rtpsurface_to_wkt_sb((RTPSURFACE*)geom, sb, precision, variant);
		break;
	default:
		rterror("rtgeom_to_wkt_sb: Type %d - %s unsupported.",
		        geom->type, rttype_name(geom->type));
	}
}

/**
* RTWKT emitter function. Allocates a new *char and fills it with the RTWKT
* representation. If size_out is not NULL, it will be set to the size of the
* allocated *char.
*
* @param variant Bitmasked value, accepts one of RTWKT_ISO, RTWKT_SFSQL, RTWKT_EXTENDED.
* @param precision Number of significant digits in the output doubles.
* @param size_out If supplied, will return the size of the returned string,
* including the null terminator.
*/
char* rtgeom_to_wkt(const RTGEOM *geom, uint8_t variant, int precision, size_t *size_out)
{
	stringbuffer_t *sb;
	char *str = NULL;
	if ( geom == NULL )
		return NULL;
	sb = stringbuffer_create();
	/* Extended mode starts with an "SRID=" section for geoms that have one */
	if ( (variant & RTWKT_EXTENDED) && rtgeom_has_srid(geom) )
	{
		stringbuffer_aprintf(sb, "SRID=%d;", geom->srid);
	}
	rtgeom_to_wkt_sb(geom, sb, precision, variant);
	if ( stringbuffer_getstring(sb) == NULL )
	{
		rterror("Uh oh");
		return NULL;
	}
	str = stringbuffer_getstringcopy(sb);
	if ( size_out )
		*size_out = stringbuffer_getlength(sb) + 1;
	stringbuffer_destroy(sb);
	return str;
}

