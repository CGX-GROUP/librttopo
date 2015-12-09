/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of hte GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

/** @file
*
* SVG output routines.
* Originally written by: Klaus Förster <klaus@svg.cc>
* Refactored by: Olivier Courtin (Camptocamp)
*
* BNF SVG Path: <http://www.w3.org/TR/SVG/paths.html#PathDataBNF>
**********************************************************************/

#include "librtgeom_internal.h"

static char * assvg_point(const RTPOINT *point, int relative, int precision);
static char * assvg_line(const RTLINE *line, int relative, int precision);
static char * assvg_polygon(const RTPOLY *poly, int relative, int precision);
static char * assvg_multipoint(const RTMPOINT *mpoint, int relative, int precision);
static char * assvg_multiline(const RTMLINE *mline, int relative, int precision);
static char * assvg_multipolygon(const RTMPOLY *mpoly, int relative, int precision);
static char * assvg_collection(const RTCOLLECTION *col, int relative, int precision);

static size_t assvg_geom_size(const RTGEOM *geom, int relative, int precision);
static size_t assvg_geom_buf(const RTGEOM *geom, char *output, int relative, int precision);
static size_t pointArray_svg_size(RTPOINTARRAY *pa, int precision);
static size_t pointArray_svg_rel(RTPOINTARRAY *pa, char * output, int close_ring, int precision);
static size_t pointArray_svg_abs(RTPOINTARRAY *pa, char * output, int close_ring, int precision);


/**
 * Takes a GEOMETRY and returns a SVG representation
 */
char *
rtgeom_to_svg(const RTGEOM *geom, int precision, int relative)
{
	char *ret = NULL;
	int type = geom->type;

	/* Empty string for empties */
	if( rtgeom_is_empty(geom) )
	{
		ret = rtalloc(1);
		ret[0] = '\0';
		return ret;
	}
	
	switch (type)
	{
	case RTPOINTTYPE:
		ret = assvg_point((RTPOINT*)geom, relative, precision);
		break;
	case RTLINETYPE:
		ret = assvg_line((RTLINE*)geom, relative, precision);
		break;
	case RTPOLYGONTYPE:
		ret = assvg_polygon((RTPOLY*)geom, relative, precision);
		break;
	case RTMULTIPOINTTYPE:
		ret = assvg_multipoint((RTMPOINT*)geom, relative, precision);
		break;
	case RTMULTILINETYPE:
		ret = assvg_multiline((RTMLINE*)geom, relative, precision);
		break;
	case RTMULTIPOLYGONTYPE:
		ret = assvg_multipolygon((RTMPOLY*)geom, relative, precision);
		break;
	case RTCOLLECTIONTYPE:
		ret = assvg_collection((RTCOLLECTION*)geom, relative, precision);
		break;

	default:
		rterror("rtgeom_to_svg: '%s' geometry type not supported",
		        rttype_name(type));
	}

	return ret;
}


/**
 * Point Geometry
 */

static size_t
assvg_point_size(const RTPOINT *point, int circle, int precision)
{
	size_t size;

	size = (OUT_MAX_DIGS_DOUBLE + precision) * 2;
	if (circle) size += sizeof("cx='' cy=''");
	else size += sizeof("x='' y=''");

	return size;
}

static size_t
assvg_point_buf(const RTPOINT *point, char * output, int circle, int precision)
{
	char *ptr=output;
	char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	RTPOINT2D pt;

	getPoint2d_p(point->point, 0, &pt);

	if (fabs(pt.x) < OUT_MAX_DOUBLE)
		sprintf(x, "%.*f", precision, pt.x);
	else
		sprintf(x, "%g", pt.x);
	trim_trailing_zeros(x);

	/* SVG Y axis is reversed, an no need to transform 0 into -0 */
	if (fabs(pt.y) < OUT_MAX_DOUBLE)
		sprintf(y, "%.*f", precision, fabs(pt.y) ? pt.y * -1 : pt.y);
	else
		sprintf(y, "%g", fabs(pt.y) ? pt.y * -1 : pt.y);
	trim_trailing_zeros(y);

	if (circle) ptr += sprintf(ptr, "x=\"%s\" y=\"%s\"", x, y);
	else ptr += sprintf(ptr, "cx=\"%s\" cy=\"%s\"", x, y);

	return (ptr-output);
}

static char *
assvg_point(const RTPOINT *point, int circle, int precision)
{
	char *output;
	int size;

	size = assvg_point_size(point, circle, precision);
	output = rtalloc(size);
	assvg_point_buf(point, output, circle, precision);

	return output;
}


/**
 * Line Geometry
 */

static size_t
assvg_line_size(const RTLINE *line, int relative, int precision)
{
	size_t size;

	size = sizeof("M ");
	size += pointArray_svg_size(line->points, precision);

	return size;
}

static size_t
assvg_line_buf(const RTLINE *line, char * output, int relative, int precision)
{
	char *ptr=output;

	/* Start path with SVG MoveTo */
	ptr += sprintf(ptr, "M ");
	if (relative)
		ptr += pointArray_svg_rel(line->points, ptr, 1, precision);
	else
		ptr += pointArray_svg_abs(line->points, ptr, 1, precision);

	return (ptr-output);
}

static char *
assvg_line(const RTLINE *line, int relative, int precision)
{
	char *output;
	int size;

	size = assvg_line_size(line, relative, precision);
	output = rtalloc(size);
	assvg_line_buf(line, output, relative, precision);

	return output;
}


/**
 * Polygon Geometry
 */

static size_t
assvg_polygon_size(const RTPOLY *poly, int relative, int precision)
{
	int i;
	size_t size=0;

	for (i=0; i<poly->nrings; i++)
		size += pointArray_svg_size(poly->rings[i], precision) + sizeof(" ");
	size += sizeof("M  Z") * poly->nrings;

	return size;
}

static size_t
assvg_polygon_buf(const RTPOLY *poly, char * output, int relative, int precision)
{
	int i;
	char *ptr=output;

	for (i=0; i<poly->nrings; i++)
	{
		if (i) ptr += sprintf(ptr, " ");	/* Space beetween each ring */
		ptr += sprintf(ptr, "M ");		/* Start path with SVG MoveTo */

		if (relative)
		{
			ptr += pointArray_svg_rel(poly->rings[i], ptr, 0, precision);
			ptr += sprintf(ptr, " z");	/* SVG closepath */
		}
		else
		{
			ptr += pointArray_svg_abs(poly->rings[i], ptr, 0, precision);
			ptr += sprintf(ptr, " Z");	/* SVG closepath */
		}
	}

	return (ptr-output);
}

static char *
assvg_polygon(const RTPOLY *poly, int relative, int precision)
{
	char *output;
	int size;

	size = assvg_polygon_size(poly, relative, precision);
	output = rtalloc(size);
	assvg_polygon_buf(poly, output, relative, precision);

	return output;
}


/**
 * Multipoint Geometry
 */

static size_t
assvg_multipoint_size(const RTMPOINT *mpoint, int relative, int precision)
{
	const RTPOINT *point;
	size_t size=0;
	int i;

	for (i=0 ; i<mpoint->ngeoms ; i++)
	{
		point = mpoint->geoms[i];
		size += assvg_point_size(point, relative, precision);
	}
	size += sizeof(",") * --i;  /* Arbitrary comma separator */

	return size;
}

static size_t
assvg_multipoint_buf(const RTMPOINT *mpoint, char *output, int relative, int precision)
{
	const RTPOINT *point;
	int i;
	char *ptr=output;

	for (i=0 ; i<mpoint->ngeoms ; i++)
	{
		if (i) ptr += sprintf(ptr, ",");  /* Arbitrary comma separator */
		point = mpoint->geoms[i];
		ptr += assvg_point_buf(point, ptr, relative, precision);
	}

	return (ptr-output);
}

static char *
assvg_multipoint(const RTMPOINT *mpoint, int relative, int precision)
{
	char *output;
	int size;

	size = assvg_multipoint_size(mpoint, relative, precision);
	output = rtalloc(size);
	assvg_multipoint_buf(mpoint, output, relative, precision);

	return output;
}


/**
 * Multiline Geometry
 */

static size_t
assvg_multiline_size(const RTMLINE *mline, int relative, int precision)
{
	const RTLINE *line;
	size_t size=0;
	int i;

	for (i=0 ; i<mline->ngeoms ; i++)
	{
		line = mline->geoms[i];
		size += assvg_line_size(line, relative, precision);
	}
	size += sizeof(" ") * --i;   /* SVG whitespace Separator */

	return size;
}

static size_t
assvg_multiline_buf(const RTMLINE *mline, char *output, int relative, int precision)
{
	const RTLINE *line;
	int i;
	char *ptr=output;

	for (i=0 ; i<mline->ngeoms ; i++)
	{
		if (i) ptr += sprintf(ptr, " ");  /* SVG whitespace Separator */
		line = mline->geoms[i];
		ptr += assvg_line_buf(line, ptr, relative, precision);
	}

	return (ptr-output);
}

static char *
assvg_multiline(const RTMLINE *mline, int relative, int precision)
{
	char *output;
	int size;

	size = assvg_multiline_size(mline, relative, precision);
	output = rtalloc(size);
	assvg_multiline_buf(mline, output, relative, precision);

	return output;
}


/*
 * Multipolygon Geometry
 */

static size_t
assvg_multipolygon_size(const RTMPOLY *mpoly, int relative, int precision)
{
	const RTPOLY *poly;
	size_t size=0;
	int i;

	for (i=0 ; i<mpoly->ngeoms ; i++)
	{
		poly = mpoly->geoms[i];
		size += assvg_polygon_size(poly, relative, precision);
	}
	size += sizeof(" ") * --i;   /* SVG whitespace Separator */

	return size;
}

static size_t
assvg_multipolygon_buf(const RTMPOLY *mpoly, char *output, int relative, int precision)
{
	const RTPOLY *poly;
	int i;
	char *ptr=output;

	for (i=0 ; i<mpoly->ngeoms ; i++)
	{
		if (i) ptr += sprintf(ptr, " ");  /* SVG whitespace Separator */
		poly = mpoly->geoms[i];
		ptr += assvg_polygon_buf(poly, ptr, relative, precision);
	}

	return (ptr-output);
}

static char *
assvg_multipolygon(const RTMPOLY *mpoly, int relative, int precision)
{
	char *output;
	int size;

	size = assvg_multipolygon_size(mpoly, relative, precision);
	output = rtalloc(size);
	assvg_multipolygon_buf(mpoly, output, relative, precision);

	return output;
}


/**
* Collection Geometry
*/

static size_t
assvg_collection_size(const RTCOLLECTION *col, int relative, int precision)
{
	int i = 0;
	size_t size=0;
	const RTGEOM *subgeom;

	for (i=0; i<col->ngeoms; i++)
	{
		subgeom = col->geoms[i];
		size += assvg_geom_size(subgeom, relative, precision);
	}

	if ( i ) /* We have some geometries, so add space for delimiters. */
		size += sizeof(";") * --i;

	if (size == 0) size++; /* GEOMETRYCOLLECTION EMPTY, space for null terminator */

	return size;
}

static size_t
assvg_collection_buf(const RTCOLLECTION *col, char *output, int relative, int precision)
{
	int i;
	char *ptr=output;
	const RTGEOM *subgeom;

	/* EMPTY GEOMETRYCOLLECTION */
	if (col->ngeoms == 0) *ptr = '\0';

	for (i=0; i<col->ngeoms; i++)
	{
		if (i) ptr += sprintf(ptr, ";");
		subgeom = col->geoms[i];
		ptr += assvg_geom_buf(subgeom, ptr, relative, precision);
	}

	return (ptr - output);
}

static char *
assvg_collection(const RTCOLLECTION *col, int relative, int precision)
{
	char *output;
	int size;

	size = assvg_collection_size(col, relative, precision);
	output = rtalloc(size);
	assvg_collection_buf(col, output, relative, precision);

	return output;
}


static size_t
assvg_geom_buf(const RTGEOM *geom, char *output, int relative, int precision)
{
    int type = geom->type;
	char *ptr=output;

	switch (type)
	{
	case RTPOINTTYPE:
		ptr += assvg_point_buf((RTPOINT*)geom, ptr, relative, precision);
		break;

	case RTLINETYPE:
		ptr += assvg_line_buf((RTLINE*)geom, ptr, relative, precision);
		break;

	case RTPOLYGONTYPE:
		ptr += assvg_polygon_buf((RTPOLY*)geom, ptr, relative, precision);
		break;

	case RTMULTIPOINTTYPE:
		ptr += assvg_multipoint_buf((RTMPOINT*)geom, ptr, relative, precision);
		break;

	case RTMULTILINETYPE:
		ptr += assvg_multiline_buf((RTMLINE*)geom, ptr, relative, precision);
		break;

	case RTMULTIPOLYGONTYPE:
		ptr += assvg_multipolygon_buf((RTMPOLY*)geom, ptr, relative, precision);
		break;

	default:
		rterror("assvg_geom_buf: '%s' geometry type not supported.",
		        rttype_name(type));
	}

	return (ptr-output);
}


static size_t
assvg_geom_size(const RTGEOM *geom, int relative, int precision)
{
    int type = geom->type;
	size_t size = 0;

	switch (type)
	{
	case RTPOINTTYPE:
		size = assvg_point_size((RTPOINT*)geom, relative, precision);
		break;

	case RTLINETYPE:
		size = assvg_line_size((RTLINE*)geom, relative, precision);
		break;

	case RTPOLYGONTYPE:
		size = assvg_polygon_size((RTPOLY*)geom, relative, precision);
		break;

	case RTMULTIPOINTTYPE:
		size = assvg_multipoint_size((RTMPOINT*)geom, relative, precision);
		break;

	case RTMULTILINETYPE:
		size = assvg_multiline_size((RTMLINE*)geom, relative, precision);
		break;

	case RTMULTIPOLYGONTYPE:
		size = assvg_multipolygon_size((RTMPOLY*)geom, relative, precision);
		break;

	default:
		rterror("assvg_geom_size: '%s' geometry type not supported.",
		        rttype_name(type));
	}

	return size;
}


static size_t
pointArray_svg_rel(RTPOINTARRAY *pa, char *output, int close_ring, int precision)
{
	int i, end;
	char *ptr;
	char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	RTPOINT2D pt, lpt;

	ptr = output;

	if (close_ring) end = pa->npoints;
	else end = pa->npoints - 1;

	/* Starting point */
	getPoint2d_p(pa, 0, &pt);

	if (fabs(pt.x) < OUT_MAX_DOUBLE)
		sprintf(x, "%.*f", precision, pt.x);
	else
		sprintf(x, "%g", pt.x);
	trim_trailing_zeros(x);

	if (fabs(pt.y) < OUT_MAX_DOUBLE)
		sprintf(y, "%.*f", precision, fabs(pt.y) ? pt.y * -1 : pt.y);
	else
		sprintf(y, "%g", fabs(pt.y) ? pt.y * -1 : pt.y);
	trim_trailing_zeros(y);

	ptr += sprintf(ptr,"%s %s l", x, y);

	/* All the following ones */
	for (i=1 ; i < end ; i++)
	{
		lpt = pt;

		getPoint2d_p(pa, i, &pt);
		if (fabs(pt.x -lpt.x) < OUT_MAX_DOUBLE)
			sprintf(x, "%.*f", precision, pt.x -lpt.x);
		else
			sprintf(x, "%g", pt.x -lpt.x);
		trim_trailing_zeros(x);

		/* SVG Y axis is reversed, an no need to transform 0 into -0 */
		if (fabs(pt.y -lpt.y) < OUT_MAX_DOUBLE)
			sprintf(y, "%.*f", precision,
			        fabs(pt.y -lpt.y) ? (pt.y - lpt.y) * -1: (pt.y - lpt.y));
		else
			sprintf(y, "%g",
			        fabs(pt.y -lpt.y) ? (pt.y - lpt.y) * -1: (pt.y - lpt.y));
		trim_trailing_zeros(y);

		ptr += sprintf(ptr," %s %s", x, y);
	}

	return (ptr-output);
}


/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_svg_abs(RTPOINTARRAY *pa, char *output, int close_ring, int precision)
{
	int i, end;
	char *ptr;
	char x[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	char y[OUT_MAX_DIGS_DOUBLE+OUT_MAX_DOUBLE_PRECISION+1];
	RTPOINT2D pt;

	ptr = output;

	if (close_ring) end = pa->npoints;
	else end = pa->npoints - 1;

	for (i=0 ; i < end ; i++)
	{
		getPoint2d_p(pa, i, &pt);

		if (fabs(pt.x) < OUT_MAX_DOUBLE)
			sprintf(x, "%.*f", precision, pt.x);
		else
			sprintf(x, "%g", pt.x);
		trim_trailing_zeros(x);

		/* SVG Y axis is reversed, an no need to transform 0 into -0 */
		if (fabs(pt.y) < OUT_MAX_DOUBLE)
			sprintf(y, "%.*f", precision, fabs(pt.y) ? pt.y * -1:pt.y);
		else
			sprintf(y, "%g", fabs(pt.y) ? pt.y * -1:pt.y);
		trim_trailing_zeros(y);

		if (i == 1) ptr += sprintf(ptr, " L ");
		else if (i) ptr += sprintf(ptr, " ");
		ptr += sprintf(ptr,"%s %s", x, y);
	}

	return (ptr-output);
}


/**
 * Returns maximum size of rendered pointarray in bytes.
 */
static size_t
pointArray_svg_size(RTPOINTARRAY *pa, int precision)
{
	return (OUT_MAX_DIGS_DOUBLE + precision + sizeof(" "))
	       * 2 * pa->npoints + sizeof(" L ");
}
