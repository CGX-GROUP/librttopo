/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2001-2003 Refractions Research Inc.
 * Copyright 2009-2010 Olivier Courtin <olivier.courtin@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "librtgeom_internal.h"
#include <string.h>	/* strlen */
#include <assert.h>

static char *asgeojson_point(const RTPOINT *point, char *srs, GBOX *bbox, int precision);
static char *asgeojson_line(const RTLINE *line, char *srs, GBOX *bbox, int precision);
static char *asgeojson_poly(const RTPOLY *poly, char *srs, GBOX *bbox, int precision);
static char * asgeojson_multipoint(const RTMPOINT *mpoint, char *srs, GBOX *bbox, int precision);
static char * asgeojson_multiline(const RTMLINE *mline, char *srs, GBOX *bbox, int precision);
static char * asgeojson_multipolygon(const RTMPOLY *mpoly, char *srs, GBOX *bbox, int precision);
static char * asgeojson_collection(const RTCOLLECTION *col, char *srs, GBOX *bbox, int precision);
static size_t asgeojson_geom_size(const RTGEOM *geom, GBOX *bbox, int precision);
static size_t asgeojson_geom_buf(const RTGEOM *geom, char *output, GBOX *bbox, int precision);

static size_t pointArray_to_geojson(POINTARRAY *pa, char *buf, int precision);
static size_t pointArray_geojson_size(POINTARRAY *pa, int precision);

/**
 * Takes a GEOMETRY and returns a GeoJson representation
 */
char *
rtgeom_to_geojson(const RTGEOM *geom, char *srs, int precision, int has_bbox)
{
	int type = geom->type;
	GBOX *bbox = NULL;
	GBOX tmp;

	if ( precision > OUT_MAX_DOUBLE_PRECISION ) precision = OUT_MAX_DOUBLE_PRECISION;

	if (has_bbox) 
	{
		/* Whether these are geography or geometry, 
		   the GeoJSON expects a cartesian bounding box */
		rtgeom_calculate_gbox_cartesian(geom, &tmp);
		bbox = &tmp;
	}		

	switch (type)
	{
	case POINTTYPE:
		return asgeojson_point((RTPOINT*)geom, srs, bbox, precision);
	case LINETYPE:
		return asgeojson_line((RTLINE*)geom, srs, bbox, precision);
	case POLYGONTYPE:
		return asgeojson_poly((RTPOLY*)geom, srs, bbox, precision);
	case MULTIPOINTTYPE:
		return asgeojson_multipoint((RTMPOINT*)geom, srs, bbox, precision);
	case MULTILINETYPE:
		return asgeojson_multiline((RTMLINE*)geom, srs, bbox, precision);
	case MULTIPOLYGONTYPE:
		return asgeojson_multipolygon((RTMPOLY*)geom, srs, bbox, precision);
	case COLLECTIONTYPE:
		return asgeojson_collection((RTCOLLECTION*)geom, srs, bbox, precision);
	default:
		rterror("rtgeom_to_geojson: '%s' geometry type not supported",
		        rttype_name(type));
	}

	/* Never get here */
	return NULL;
}



/**
 * Handle SRS
 */
static size_t
asgeojson_srs_size(char *srs)
{
	int size;

	size = sizeof("'crs':{'type':'name',");
	size += sizeof("'properties':{'name':''}},");
	size += strlen(srs) * sizeof(char);

	return size;
}

static size_t
asgeojson_srs_buf(char *output, char *srs)
{
	char *ptr = output;

	ptr += sprintf(ptr, "\"crs\":{\"type\":\"name\",");
	ptr += sprintf(ptr, "\"properties\":{\"name\":\"%s\"}},", srs);

	return (ptr-output);
}



/**
 * Handle Bbox
 */
static size_t
asgeojson_bbox_size(int hasz, int precision)
{
	int size;

	if (!hasz)
	{
		size = sizeof("\"bbox\":[,,,],");
		size +=	2 * 2 * (OUT_MAX_DIGS_DOUBLE + precision);
	}
	else
	{
		size = sizeof("\"bbox\":[,,,,,],");
		size +=	2 * 3 * (OUT_MAX_DIGS_DOUBLE + precision);
	}

	return size;
}

static size_t
asgeojson_bbox_buf(char *output, GBOX *bbox, int hasz, int precision)
{
	char *ptr = output;

	if (!hasz)
		ptr += sprintf(ptr, "\"bbox\":[%.*f,%.*f,%.*f,%.*f],",
		               precision, bbox->xmin, precision, bbox->ymin,
		               precision, bbox->xmax, precision, bbox->ymax);
	else
		ptr += sprintf(ptr, "\"bbox\":[%.*f,%.*f,%.*f,%.*f,%.*f,%.*f],",
		               precision, bbox->xmin, precision, bbox->ymin, precision, bbox->zmin,
		               precision, bbox->xmax, precision, bbox->ymax, precision, bbox->zmax);

	return (ptr-output);
}



/**
 * Point Geometry
 */

static size_t
asgeojson_point_size(const RTPOINT *point, char *srs, GBOX *bbox, int precision)
{
	int size;

	size = pointArray_geojson_size(point->point, precision);
	size += sizeof("{'type':'Point',");
	size += sizeof("'coordinates':}");

	if ( rtpoint_is_empty(point) )
		size += 2; /* [] */

	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(FLAGS_GET_Z(point->flags), precision);

	return size;
}

static size_t
asgeojson_point_buf(const RTPOINT *point, char *srs, char *output, GBOX *bbox, int precision)
{
	char *ptr = output;

	ptr += sprintf(ptr, "{\"type\":\"Point\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, FLAGS_GET_Z(point->flags), precision);

	ptr += sprintf(ptr, "\"coordinates\":");
	if ( rtpoint_is_empty(point) )
		ptr += sprintf(ptr, "[]");
	ptr += pointArray_to_geojson(point->point, ptr, precision);
	ptr += sprintf(ptr, "}");

	return (ptr-output);
}

static char *
asgeojson_point(const RTPOINT *point, char *srs, GBOX *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_point_size(point, srs, bbox, precision);
	output = rtalloc(size);
	asgeojson_point_buf(point, srs, output, bbox, precision);
	return output;
}



/**
 * Line Geometry
 */

static size_t
asgeojson_line_size(const RTLINE *line, char *srs, GBOX *bbox, int precision)
{
	int size;

	size = sizeof("{'type':'LineString',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(FLAGS_GET_Z(line->flags), precision);
	size += sizeof("'coordinates':[]}");
	size += pointArray_geojson_size(line->points, precision);

	return size;
}

static size_t
asgeojson_line_buf(const RTLINE *line, char *srs, char *output, GBOX *bbox, int precision)
{
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"LineString\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, FLAGS_GET_Z(line->flags), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");
	ptr += pointArray_to_geojson(line->points, ptr, precision);
	ptr += sprintf(ptr, "]}");

	return (ptr-output);
}

static char *
asgeojson_line(const RTLINE *line, char *srs, GBOX *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_line_size(line, srs, bbox, precision);
	output = rtalloc(size);
	asgeojson_line_buf(line, srs, output, bbox, precision);

	return output;
}



/**
 * Polygon Geometry
 */

static size_t
asgeojson_poly_size(const RTPOLY *poly, char *srs, GBOX *bbox, int precision)
{
	size_t size;
	int i;

	size = sizeof("{\"type\":\"Polygon\",");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(FLAGS_GET_Z(poly->flags), precision);
	size += sizeof("\"coordinates\":[");
	for (i=0, size=0; i<poly->nrings; i++)
	{
		size += pointArray_geojson_size(poly->rings[i], precision);
		size += sizeof("[]");
	}
	size += sizeof(",") * i;
	size += sizeof("]}");

	return size;
}

static size_t
asgeojson_poly_buf(const RTPOLY *poly, char *srs, char *output, GBOX *bbox, int precision)
{
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"Polygon\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, FLAGS_GET_Z(poly->flags), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");
	for (i=0; i<poly->nrings; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		ptr += sprintf(ptr, "[");
		ptr += pointArray_to_geojson(poly->rings[i], ptr, precision);
		ptr += sprintf(ptr, "]");
	}
	ptr += sprintf(ptr, "]}");

	return (ptr-output);
}

static char *
asgeojson_poly(const RTPOLY *poly, char *srs, GBOX *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_poly_size(poly, srs, bbox, precision);
	output = rtalloc(size);
	asgeojson_poly_buf(poly, srs, output, bbox, precision);

	return output;
}



/**
 * Multipoint Geometry
 */

static size_t
asgeojson_multipoint_size(const RTMPOINT *mpoint, char *srs, GBOX *bbox, int precision)
{
	RTPOINT * point;
	int size;
	int i;

	size = sizeof("{'type':'MultiPoint',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(FLAGS_GET_Z(mpoint->flags), precision);
	size += sizeof("'coordinates':[]}");

	for (i=0; i<mpoint->ngeoms; i++)
	{
		point = mpoint->geoms[i];
		size += pointArray_geojson_size(point->point, precision);
	}
	size += sizeof(",") * i;

	return size;
}

static size_t
asgeojson_multipoint_buf(const RTMPOINT *mpoint, char *srs, char *output, GBOX *bbox, int precision)
{
	RTPOINT *point;
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"MultiPoint\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, FLAGS_GET_Z(mpoint->flags), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");

	for (i=0; i<mpoint->ngeoms; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		point = mpoint->geoms[i];
		ptr += pointArray_to_geojson(point->point, ptr, precision);
	}
	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_multipoint(const RTMPOINT *mpoint, char *srs, GBOX *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_multipoint_size(mpoint, srs, bbox, precision);
	output = rtalloc(size);
	asgeojson_multipoint_buf(mpoint, srs, output, bbox, precision);

	return output;
}



/**
 * Multiline Geometry
 */

static size_t
asgeojson_multiline_size(const RTMLINE *mline, char *srs, GBOX *bbox, int precision)
{
	RTLINE * line;
	int size;
	int i;

	size = sizeof("{'type':'MultiLineString',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(FLAGS_GET_Z(mline->flags), precision);
	size += sizeof("'coordinates':[]}");

	for (i=0 ; i<mline->ngeoms; i++)
	{
		line = mline->geoms[i];
		size += pointArray_geojson_size(line->points, precision);
		size += sizeof("[]");
	}
	size += sizeof(",") * i;

	return size;
}

static size_t
asgeojson_multiline_buf(const RTMLINE *mline, char *srs, char *output, GBOX *bbox, int precision)
{
	RTLINE *line;
	int i;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"MultiLineString\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, FLAGS_GET_Z(mline->flags), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");

	for (i=0; i<mline->ngeoms; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		ptr += sprintf(ptr, "[");
		line = mline->geoms[i];
		ptr += pointArray_to_geojson(line->points, ptr, precision);
		ptr += sprintf(ptr, "]");
	}

	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_multiline(const RTMLINE *mline, char *srs, GBOX *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_multiline_size(mline, srs, bbox, precision);
	output = rtalloc(size);
	asgeojson_multiline_buf(mline, srs, output, bbox, precision);

	return output;
}



/**
 * MultiPolygon Geometry
 */

static size_t
asgeojson_multipolygon_size(const RTMPOLY *mpoly, char *srs, GBOX *bbox, int precision)
{
	RTPOLY *poly;
	int size;
	int i, j;

	size = sizeof("{'type':'MultiPolygon',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(FLAGS_GET_Z(mpoly->flags), precision);
	size += sizeof("'coordinates':[]}");

	for (i=0; i < mpoly->ngeoms; i++)
	{
		poly = mpoly->geoms[i];
		for (j=0 ; j <poly->nrings ; j++)
		{
			size += pointArray_geojson_size(poly->rings[j], precision);
			size += sizeof("[]");
		}
		size += sizeof("[]");
	}
	size += sizeof(",") * i;
	size += sizeof("]}");

	return size;
}

static size_t
asgeojson_multipolygon_buf(const RTMPOLY *mpoly, char *srs, char *output, GBOX *bbox, int precision)
{
	RTPOLY *poly;
	int i, j;
	char *ptr=output;

	ptr += sprintf(ptr, "{\"type\":\"MultiPolygon\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (bbox) ptr += asgeojson_bbox_buf(ptr, bbox, FLAGS_GET_Z(mpoly->flags), precision);
	ptr += sprintf(ptr, "\"coordinates\":[");
	for (i=0; i<mpoly->ngeoms; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		ptr += sprintf(ptr, "[");
		poly = mpoly->geoms[i];
		for (j=0 ; j < poly->nrings ; j++)
		{
			if (j) ptr += sprintf(ptr, ",");
			ptr += sprintf(ptr, "[");
			ptr += pointArray_to_geojson(poly->rings[j], ptr, precision);
			ptr += sprintf(ptr, "]");
		}
		ptr += sprintf(ptr, "]");
	}
	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_multipolygon(const RTMPOLY *mpoly, char *srs, GBOX *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_multipolygon_size(mpoly, srs, bbox, precision);
	output = rtalloc(size);
	asgeojson_multipolygon_buf(mpoly, srs, output, bbox, precision);

	return output;
}



/**
 * Collection Geometry
 */

static size_t
asgeojson_collection_size(const RTCOLLECTION *col, char *srs, GBOX *bbox, int precision)
{
	int i;
	int size;
	RTGEOM *subgeom;

	size = sizeof("{'type':'GeometryCollection',");
	if (srs) size += asgeojson_srs_size(srs);
	if (bbox) size += asgeojson_bbox_size(FLAGS_GET_Z(col->flags), precision);
	size += sizeof("'geometries':");

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		size += asgeojson_geom_size(subgeom, NULL, precision);
	}
	size += sizeof(",") * i;
	size += sizeof("]}");

	return size;
}

static size_t
asgeojson_collection_buf(const RTCOLLECTION *col, char *srs, char *output, GBOX *bbox, int precision)
{
	int i;
	char *ptr=output;
	RTGEOM *subgeom;

	ptr += sprintf(ptr, "{\"type\":\"GeometryCollection\",");
	if (srs) ptr += asgeojson_srs_buf(ptr, srs);
	if (col->ngeoms && bbox) ptr += asgeojson_bbox_buf(ptr, bbox, FLAGS_GET_Z(col->flags), precision);
	ptr += sprintf(ptr, "\"geometries\":[");

	for (i=0; i<col->ngeoms; i++)
	{
		if (i) ptr += sprintf(ptr, ",");
		subgeom = col->geoms[i];
		ptr += asgeojson_geom_buf(subgeom, ptr, NULL, precision);
	}

	ptr += sprintf(ptr, "]}");

	return (ptr - output);
}

static char *
asgeojson_collection(const RTCOLLECTION *col, char *srs, GBOX *bbox, int precision)
{
	char *output;
	int size;

	size = asgeojson_collection_size(col, srs, bbox, precision);
	output = rtalloc(size);
	asgeojson_collection_buf(col, srs, output, bbox, precision);

	return output;
}



static size_t
asgeojson_geom_size(const RTGEOM *geom, GBOX *bbox, int precision)
{
	int type = geom->type;
	size_t size = 0;

	switch (type)
	{
	case POINTTYPE:
		size = asgeojson_point_size((RTPOINT*)geom, NULL, bbox, precision);
		break;

	case LINETYPE:
		size = asgeojson_line_size((RTLINE*)geom, NULL, bbox, precision);
		break;

	case POLYGONTYPE:
		size = asgeojson_poly_size((RTPOLY*)geom, NULL, bbox, precision);
		break;

	case MULTIPOINTTYPE:
		size = asgeojson_multipoint_size((RTMPOINT*)geom, NULL, bbox, precision);
		break;

	case MULTILINETYPE:
		size = asgeojson_multiline_size((RTMLINE*)geom, NULL, bbox, precision);
		break;

	case MULTIPOLYGONTYPE:
		size = asgeojson_multipolygon_size((RTMPOLY*)geom, NULL, bbox, precision);
		break;

	default:
		rterror("GeoJson: geometry not supported.");
	}

	return size;
}


static size_t
asgeojson_geom_buf(const RTGEOM *geom, char *output, GBOX *bbox, int precision)
{
	int type = geom->type;
	char *ptr=output;

	switch (type)
	{
	case POINTTYPE:
		ptr += asgeojson_point_buf((RTPOINT*)geom, NULL, ptr, bbox, precision);
		break;

	case LINETYPE:
		ptr += asgeojson_line_buf((RTLINE*)geom, NULL, ptr, bbox, precision);
		break;

	case POLYGONTYPE:
		ptr += asgeojson_poly_buf((RTPOLY*)geom, NULL, ptr, bbox, precision);
		break;

	case MULTIPOINTTYPE:
		ptr += asgeojson_multipoint_buf((RTMPOINT*)geom, NULL, ptr, bbox, precision);
		break;

	case MULTILINETYPE:
		ptr += asgeojson_multiline_buf((RTMLINE*)geom, NULL, ptr, bbox, precision);
		break;

	case MULTIPOLYGONTYPE:
		ptr += asgeojson_multipolygon_buf((RTMPOLY*)geom, NULL, ptr, bbox, precision);
		break;

	default:
		if (bbox) rtfree(bbox);
		rterror("GeoJson: geometry not supported.");
	}

	return (ptr-output);
}

/*
 * Print an ordinate value using at most the given number of decimal digits
 *
 * The actual number of printed decimal digits may be less than the
 * requested ones if out of significant digits.
 *
 * The function will not write more than maxsize bytes, including the
 * terminating NULL. Returns the number of bytes that would have been
 * written if there was enough space (excluding terminating NULL).
 * So a return of ``bufsize'' or more means that the string was
 * truncated and misses a terminating NULL.
 *
 * TODO: export ?
 *
 */
static int
rtprint_double(double d, int maxdd, char *buf, size_t bufsize)
{
  double ad = fabs(d);
  int ndd = ad < 1 ? 0 : floor(log10(ad))+1; /* non-decimal digits */
  if (fabs(d) < OUT_MAX_DOUBLE)
  {
    if ( maxdd > (OUT_MAX_DOUBLE_PRECISION - ndd) )  maxdd -= ndd;
    return snprintf(buf, bufsize, "%.*f", maxdd, d);
  }
  else
  {
    return snprintf(buf, bufsize, "%g", d);
  }
}



static size_t
pointArray_to_geojson(POINTARRAY *pa, char *output, int precision)
{
	int i;
	char *ptr;
#define BUFSIZE OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION
	char x[BUFSIZE+1];
	char y[BUFSIZE+1];
	char z[BUFSIZE+1];

	assert ( precision <= OUT_MAX_DOUBLE_PRECISION );

  /* Ensure a terminating NULL at the end of buffers
   * so that we don't need to check for truncation
   * inprint_double */
  x[BUFSIZE] = '\0';
  y[BUFSIZE] = '\0';
  z[BUFSIZE] = '\0';

	ptr = output;

  /* TODO: rewrite this loop to be simpler and possibly quicker */
	if (!FLAGS_GET_Z(pa->flags))
	{
		for (i=0; i<pa->npoints; i++)
		{
			const POINT2D *pt;
			pt = getPoint2d_cp(pa, i);

			rtprint_double(pt->x, precision, x, BUFSIZE);
			trim_trailing_zeros(x);
			rtprint_double(pt->y, precision, y, BUFSIZE);
			trim_trailing_zeros(y);

			if ( i ) ptr += sprintf(ptr, ",");
			ptr += sprintf(ptr, "[%s,%s]", x, y);
		}
	}
	else
	{
		for (i=0; i<pa->npoints; i++)
		{
			const POINT3DZ *pt;
			pt = getPoint3dz_cp(pa, i);

			rtprint_double(pt->x, precision, x, BUFSIZE);
			trim_trailing_zeros(x);
			rtprint_double(pt->y, precision, y, BUFSIZE);
			trim_trailing_zeros(y);
			rtprint_double(pt->z, precision, z, BUFSIZE);
			trim_trailing_zeros(z);

			if ( i ) ptr += sprintf(ptr, ",");
			ptr += sprintf(ptr, "[%s,%s,%s]", x, y, z);
		}
	}

	return (ptr-output);
}



/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_geojson_size(POINTARRAY *pa, int precision)
{
	assert ( precision <= OUT_MAX_DOUBLE_PRECISION );
	if (FLAGS_NDIMS(pa->flags) == 2)
		return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(","))
		       * 2 * pa->npoints + sizeof(",[]");

	return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(",,"))
	       * 3 * pa->npoints + sizeof(",[]");
}
