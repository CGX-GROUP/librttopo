/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * 
 * Copyright 2006 Corporacion Autonoma Regional de Santander 
 *                Eduin Carrillo <yecarrillo@cas.gov.co>
 * Copyright 2010 Paul Ramsey <pramsey@cleverelephant.ca>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "librtgeom_internal.h"
#include "stringbuffer.h"

static int rtgeom_to_kml2_sb(const RTGEOM *geom, int precision, const char *prefix, stringbuffer_t *sb);
static int rtpoint_to_kml2_sb(const RTPOINT *point, int precision, const char *prefix, stringbuffer_t *sb);
static int rtline_to_kml2_sb(const RTLINE *line, int precision, const char *prefix, stringbuffer_t *sb);
static int rtpoly_to_kml2_sb(const RTPOLY *poly, int precision, const char *prefix, stringbuffer_t *sb);
static int rtcollection_to_kml2_sb(const RTCOLLECTION *col, int precision, const char *prefix, stringbuffer_t *sb);
static int ptarray_to_kml2_sb(const POINTARRAY *pa, int precision, stringbuffer_t *sb);

/*
* KML 2.2.0
*/

/* takes a GEOMETRY and returns a KML representation */
char*
rtgeom_to_kml2(const RTGEOM *geom, int precision, const char *prefix)
{
	stringbuffer_t *sb;
	int rv;
	char *kml;

	/* Can't do anything with empty */
	if( rtgeom_is_empty(geom) )
		return NULL;

	sb = stringbuffer_create();
	rv = rtgeom_to_kml2_sb(geom, precision, prefix, sb);
	
	if ( rv == RT_FAILURE )
	{
		stringbuffer_destroy(sb);
		return NULL;
	}
	
	kml = stringbuffer_getstringcopy(sb);
	stringbuffer_destroy(sb);
	
	return kml;
}

static int 
rtgeom_to_kml2_sb(const RTGEOM *geom, int precision, const char *prefix, stringbuffer_t *sb)
{
	switch (geom->type)
	{
	case RTPOINTTYPE:
		return rtpoint_to_kml2_sb((RTPOINT*)geom, precision, prefix, sb);

	case RTLINETYPE:
		return rtline_to_kml2_sb((RTLINE*)geom, precision, prefix, sb);

	case RTPOLYGONTYPE:
		return rtpoly_to_kml2_sb((RTPOLY*)geom, precision, prefix, sb);

	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
		return rtcollection_to_kml2_sb((RTCOLLECTION*)geom, precision, prefix, sb);

	default:
		rterror("rtgeom_to_kml2: '%s' geometry type not supported", rttype_name(geom->type));
		return RT_FAILURE;
	}
}

static int 
ptarray_to_kml2_sb(const POINTARRAY *pa, int precision, stringbuffer_t *sb)
{
	int i, j;
	int dims = FLAGS_GET_Z(pa->flags) ? 3 : 2;
	RTPOINT4D pt;
	double *d;
	
	for ( i = 0; i < pa->npoints; i++ )
	{
		getPoint4d_p(pa, i, &pt);
		d = (double*)(&pt);
		if ( i ) stringbuffer_append(sb," ");
		for (j = 0; j < dims; j++)
		{
			if ( j ) stringbuffer_append(sb,",");
			if( fabs(d[j]) < OUT_MAX_DOUBLE )
			{
				if ( stringbuffer_aprintf(sb, "%.*f", precision, d[j]) < 0 ) return RT_FAILURE;
			}
			else 
			{
				if ( stringbuffer_aprintf(sb, "%g", d[j]) < 0 ) return RT_FAILURE;
			}
			stringbuffer_trim_trailing_zeroes(sb);
		}
	}
	return RT_SUCCESS;
}


static int 
rtpoint_to_kml2_sb(const RTPOINT *point, int precision, const char *prefix, stringbuffer_t *sb)
{
	/* Open point */
	if ( stringbuffer_aprintf(sb, "<%sPoint><%scoordinates>", prefix, prefix) < 0 ) return RT_FAILURE;
	/* Coordinate array */
	if ( ptarray_to_kml2_sb(point->point, precision, sb) == RT_FAILURE ) return RT_FAILURE;
	/* Close point */
	if ( stringbuffer_aprintf(sb, "</%scoordinates></%sPoint>", prefix, prefix) < 0 ) return RT_FAILURE;
	return RT_SUCCESS;
}

static int 
rtline_to_kml2_sb(const RTLINE *line, int precision, const char *prefix, stringbuffer_t *sb)
{
	/* Open linestring */
	if ( stringbuffer_aprintf(sb, "<%sLineString><%scoordinates>", prefix, prefix) < 0 ) return RT_FAILURE;
	/* Coordinate array */
	if ( ptarray_to_kml2_sb(line->points, precision, sb) == RT_FAILURE ) return RT_FAILURE;
	/* Close linestring */
	if ( stringbuffer_aprintf(sb, "</%scoordinates></%sLineString>", prefix, prefix) < 0 ) return RT_FAILURE;
	
	return RT_SUCCESS;
}

static int 
rtpoly_to_kml2_sb(const RTPOLY *poly, int precision, const char *prefix, stringbuffer_t *sb)
{
	int i, rv;
	
	/* Open polygon */
	if ( stringbuffer_aprintf(sb, "<%sPolygon>", prefix) < 0 ) return RT_FAILURE;
	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Inner or outer ring opening tags */
		if( i )
			rv = stringbuffer_aprintf(sb, "<%sinnerBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);
		else
			rv = stringbuffer_aprintf(sb, "<%souterBoundaryIs><%sLinearRing><%scoordinates>", prefix, prefix, prefix);		
		if ( rv < 0 ) return RT_FAILURE;
		
		/* Coordinate array */
		if ( ptarray_to_kml2_sb(poly->rings[i], precision, sb) == RT_FAILURE ) return RT_FAILURE;
		
		/* Inner or outer ring closing tags */
		if( i )
			rv = stringbuffer_aprintf(sb, "</%scoordinates></%sLinearRing></%sinnerBoundaryIs>", prefix, prefix, prefix);
		else
			rv = stringbuffer_aprintf(sb, "</%scoordinates></%sLinearRing></%souterBoundaryIs>", prefix, prefix, prefix);		
		if ( rv < 0 ) return RT_FAILURE;
	}
	/* Close polygon */
	if ( stringbuffer_aprintf(sb, "</%sPolygon>", prefix) < 0 ) return RT_FAILURE;

	return RT_SUCCESS;
}

static int 
rtcollection_to_kml2_sb(const RTCOLLECTION *col, int precision, const char *prefix, stringbuffer_t *sb)
{
	int i, rv;
		
	/* Open geometry */
	if ( stringbuffer_aprintf(sb, "<%sMultiGeometry>", prefix) < 0 ) return RT_FAILURE;
	for ( i = 0; i < col->ngeoms; i++ )
	{
		rv = rtgeom_to_kml2_sb(col->geoms[i], precision, prefix, sb);
		if ( rv == RT_FAILURE ) return RT_FAILURE;		
	}
	/* Close geometry */
	if ( stringbuffer_aprintf(sb, "</%sMultiGeometry>", prefix) < 0 ) return RT_FAILURE;

	return RT_SUCCESS;
}
