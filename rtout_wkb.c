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

#include <math.h>

#include "librtgeom_internal.h"
#include "rtgeom_log.h"

static uint8_t* rtgeom_to_wkb_buf(const RTGEOM *geom, uint8_t *buf, uint8_t variant);
static size_t rtgeom_to_wkb_size(const RTGEOM *geom, uint8_t variant);

/*
* Look-up table for hex writer
*/
static char *hexchr = "0123456789ABCDEF";

char* hexbytes_from_bytes(uint8_t *bytes, size_t size) 
{
	char *hex;
	int i;
	if ( ! bytes || ! size )
	{
		rterror("hexbutes_from_bytes: invalid input");
		return NULL;
	}
	hex = rtalloc(size * 2 + 1);
	hex[2*size] = '\0';
	for( i = 0; i < size; i++ )
	{
		/* Top four bits to 0-F */
		hex[2*i] = hexchr[bytes[i] >> 4];
		/* Bottom four bits to 0-F */
		hex[2*i+1] = hexchr[bytes[i] & 0x0F];
	}
	return hex;
}

/*
* Optional SRID
*/
static int rtgeom_wkb_needs_srid(const RTGEOM *geom, uint8_t variant)
{
	/* Sub-components of collections inherit their SRID from the parent.
	   We force that behavior with the RTWKB_NO_SRID flag */
	if ( variant & RTWKB_NO_SRID )
		return RT_FALSE;
		
	/* We can only add an SRID if the geometry has one, and the 
	   RTWKB form is extended */	
	if ( (variant & RTWKB_EXTENDED) && rtgeom_has_srid(geom) )
		return RT_TRUE;
		
	/* Everything else doesn't get an SRID */
	return RT_FALSE;
}

/*
* GeometryType
*/
static uint32_t rtgeom_wkb_type(const RTGEOM *geom, uint8_t variant)
{
	uint32_t wkb_type = 0;

	switch ( geom->type )
	{
	case RTPOINTTYPE:
		wkb_type = RTWKB_POINT_TYPE;
		break;
	case RTLINETYPE:
		wkb_type = RTWKB_LINESTRING_TYPE;
		break;
	case RTPOLYGONTYPE:
		wkb_type = RTWKB_POLYGON_TYPE;
		break;
	case RTMULTIPOINTTYPE:
		wkb_type = RTWKB_MULTIPOINT_TYPE;
		break;
	case RTMULTILINETYPE:
		wkb_type = RTWKB_MULTILINESTRING_TYPE;
		break;
	case RTMULTIPOLYGONTYPE:
		wkb_type = RTWKB_MULTIPOLYGON_TYPE;
		break;
	case RTCOLLECTIONTYPE:
		wkb_type = RTWKB_GEOMETRYCOLLECTION_TYPE;
		break;
	case RTCIRCSTRINGTYPE:
		wkb_type = RTWKB_CIRCULARSTRING_TYPE;
		break;
	case RTCOMPOUNDTYPE:
		wkb_type = RTWKB_COMPOUNDCURVE_TYPE;
		break;
	case RTCURVEPOLYTYPE:
		wkb_type = RTWKB_CURVEPOLYGON_TYPE;
		break;
	case RTMULTICURVETYPE:
		wkb_type = RTWKB_MULTICURVE_TYPE;
		break;
	case RTMULTISURFACETYPE:
		wkb_type = RTWKB_MULTISURFACE_TYPE;
		break;
	case RTPOLYHEDRALSURFACETYPE:
		wkb_type = RTWKB_POLYHEDRALSURFACE_TYPE;
		break;
	case RTTINTYPE:
		wkb_type = RTWKB_TIN_TYPE;
		break;
	case RTTRIANGLETYPE:
		wkb_type = RTWKB_TRIANGLE_TYPE;
		break;
	default:
		rterror("Unsupported geometry type: %s [%d]",
			rttype_name(geom->type), geom->type);
	}

	if ( variant & RTWKB_EXTENDED )
	{
		if ( FLAGS_GET_Z(geom->flags) )
			wkb_type |= RTWKBZOFFSET;
		if ( FLAGS_GET_M(geom->flags) )
			wkb_type |= RTWKBMOFFSET;
/*		if ( geom->srid != SRID_UNKNOWN && ! (variant & RTWKB_NO_SRID) ) */
		if ( rtgeom_wkb_needs_srid(geom, variant) )
			wkb_type |= RTWKBSRIDFLAG;
	}
	else if ( variant & RTWKB_ISO )
	{
		/* Z types are in the 1000 range */
		if ( FLAGS_GET_Z(geom->flags) )
			wkb_type += 1000;
		/* M types are in the 2000 range */
		if ( FLAGS_GET_M(geom->flags) )
			wkb_type += 2000;
		/* ZM types are in the 1000 + 2000 = 3000 range, see above */
	}
	return wkb_type;
}

/*
* Endian
*/
static uint8_t* endian_to_wkb_buf(uint8_t *buf, uint8_t variant)
{
	if ( variant & RTWKB_HEX )
	{
		buf[0] = '0';
		buf[1] = ((variant & RTWKB_NDR) ? '1' : '0');
		return buf + 2;
	}
	else
	{
		buf[0] = ((variant & RTWKB_NDR) ? 1 : 0);
		return buf + 1;
	}
}

/*
* SwapBytes?
*/
static inline int wkb_swap_bytes(uint8_t variant)
{
	/* If requested variant matches machine arch, we don't have to swap! */
	if ( ((variant & RTWKB_NDR) && (getMachineEndian() == NDR)) ||
	     ((! (variant & RTWKB_NDR)) && (getMachineEndian() == XDR)) )
	{
		return RT_FALSE;
	}
	return RT_TRUE;
}

/*
* Integer32
*/
static uint8_t* integer_to_wkb_buf(const int ival, uint8_t *buf, uint8_t variant)
{
	char *iptr = (char*)(&ival);
	int i = 0;

	if ( sizeof(int) != RTWKB_INT_SIZE )
	{
		rterror("Machine int size is not %d bytes!", RTWKB_INT_SIZE);
	}
	RTDEBUGF(4, "Writing value '%u'", ival);
	if ( variant & RTWKB_HEX )
	{
		int swap = wkb_swap_bytes(variant);
		/* Machine/request arch mismatch, so flip byte order */
		for ( i = 0; i < RTWKB_INT_SIZE; i++ )
		{
			int j = (swap ? RTWKB_INT_SIZE - 1 - i : i);
			uint8_t b = iptr[j];
			/* Top four bits to 0-F */
			buf[2*i] = hexchr[b >> 4];
			/* Bottom four bits to 0-F */
			buf[2*i+1] = hexchr[b & 0x0F];
		}
		return buf + (2 * RTWKB_INT_SIZE);
	}
	else
	{
		/* Machine/request arch mismatch, so flip byte order */
		if ( wkb_swap_bytes(variant) )
		{
			for ( i = 0; i < RTWKB_INT_SIZE; i++ )
			{
				buf[i] = iptr[RTWKB_INT_SIZE - 1 - i];
			}
		}
		/* If machine arch and requested arch match, don't flip byte order */
		else
		{
			memcpy(buf, iptr, RTWKB_INT_SIZE);
		}
		return buf + RTWKB_INT_SIZE;
	}
}

/*
* Float64
*/
static uint8_t* double_to_wkb_buf(const double d, uint8_t *buf, uint8_t variant)
{
	char *dptr = (char*)(&d);
	int i = 0;

	if ( sizeof(double) != RTWKB_DOUBLE_SIZE )
	{
		rterror("Machine double size is not %d bytes!", RTWKB_DOUBLE_SIZE);
	}

	if ( variant & RTWKB_HEX )
	{
		int swap =  wkb_swap_bytes(variant);
		/* Machine/request arch mismatch, so flip byte order */
		for ( i = 0; i < RTWKB_DOUBLE_SIZE; i++ )
		{
			int j = (swap ? RTWKB_DOUBLE_SIZE - 1 - i : i);
			uint8_t b = dptr[j];
			/* Top four bits to 0-F */
			buf[2*i] = hexchr[b >> 4];
			/* Bottom four bits to 0-F */
			buf[2*i+1] = hexchr[b & 0x0F];
		}
		return buf + (2 * RTWKB_DOUBLE_SIZE);
	}
	else
	{
		/* Machine/request arch mismatch, so flip byte order */
		if ( wkb_swap_bytes(variant) )
		{
			for ( i = 0; i < RTWKB_DOUBLE_SIZE; i++ )
			{
				buf[i] = dptr[RTWKB_DOUBLE_SIZE - 1 - i];
			}
		}
		/* If machine arch and requested arch match, don't flip byte order */
		else
		{
			memcpy(buf, dptr, RTWKB_DOUBLE_SIZE);
		}
		return buf + RTWKB_DOUBLE_SIZE;
	}
}


/*
* Empty
*/
static size_t empty_to_wkb_size(const RTGEOM *geom, uint8_t variant)
{
	/* endian byte + type integer */
	size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE;

	/* optional srid integer */
	if ( rtgeom_wkb_needs_srid(geom, variant) )
		size += RTWKB_INT_SIZE;

	/* Represent POINT EMPTY as POINT(NaN NaN) */
	if ( geom->type == RTPOINTTYPE )
	{
		const RTPOINT *pt = (RTPOINT*)geom;
		size += RTWKB_DOUBLE_SIZE * FLAGS_NDIMS(pt->point->flags);		
	}
	/* num-elements */
	else
	{
		size += RTWKB_INT_SIZE;
	}

	return size;
}

static uint8_t* empty_to_wkb_buf(const RTGEOM *geom, uint8_t *buf, uint8_t variant)
{
	uint32_t wkb_type = rtgeom_wkb_type(geom, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);

	/* Set the geometry type */
	buf = integer_to_wkb_buf(wkb_type, buf, variant);

	/* Set the SRID if necessary */
	if ( rtgeom_wkb_needs_srid(geom, variant) )
		buf = integer_to_wkb_buf(geom->srid, buf, variant);

	/* Represent POINT EMPTY as POINT(NaN NaN) */
	if ( geom->type == RTPOINTTYPE )
	{
		const RTPOINT *pt = (RTPOINT*)geom;
		static double nn = NAN;
		int i;
		for ( i = 0; i < FLAGS_NDIMS(pt->point->flags); i++ )
		{
			buf = double_to_wkb_buf(nn, buf, variant);
		}
	}
	/* Everything else is flagged as empty using num-elements == 0 */
	else
	{
		/* Set nrings/npoints/ngeoms to zero */
		buf = integer_to_wkb_buf(0, buf, variant);
	}
	
	return buf;
}

/*
* RTPOINTARRAY
*/
static size_t ptarray_to_wkb_size(const RTPOINTARRAY *pa, uint8_t variant)
{
	int dims = 2;
	size_t size = 0;

	if ( variant & (RTWKB_ISO | RTWKB_EXTENDED) )
		dims = FLAGS_NDIMS(pa->flags);

	/* Include the npoints if it's not a POINT type) */
	if ( ! ( variant & RTWKB_NO_NPOINTS ) )
		size += RTWKB_INT_SIZE;

	/* size of the double list */
	size += pa->npoints * dims * RTWKB_DOUBLE_SIZE;

	return size;
}

static uint8_t* ptarray_to_wkb_buf(const RTPOINTARRAY *pa, uint8_t *buf, uint8_t variant)
{
	int dims = 2;
	int pa_dims = FLAGS_NDIMS(pa->flags);
	int i, j;
	double *dbl_ptr;

	/* SFSQL is artays 2-d. Extended and ISO use all available dimensions */
	if ( (variant & RTWKB_ISO) || (variant & RTWKB_EXTENDED) )
		dims = pa_dims;

	/* Set the number of points (if it's not a POINT type) */
	if ( ! ( variant & RTWKB_NO_NPOINTS ) )
		buf = integer_to_wkb_buf(pa->npoints, buf, variant);

	/* Bulk copy the coordinates when: dimensionality matches, output format */
	/* is not hex, and output endian matches internal endian. */
	if ( pa->npoints && (dims == pa_dims) && ! wkb_swap_bytes(variant) && ! (variant & RTWKB_HEX)  )
	{
		size_t size = pa->npoints * dims * RTWKB_DOUBLE_SIZE;
		memcpy(buf, getPoint_internal(pa, 0), size);
		buf += size;
	}
	/* Copy coordinates one-by-one otherwise */
	else 
	{
		for ( i = 0; i < pa->npoints; i++ )
		{
			RTDEBUGF(4, "Writing point #%d", i);
			dbl_ptr = (double*)getPoint_internal(pa, i);
			for ( j = 0; j < dims; j++ )
			{
				RTDEBUGF(4, "Writing dimension #%d (buf = %p)", j, buf);
				buf = double_to_wkb_buf(dbl_ptr[j], buf, variant);
			}
		}
	}
	RTDEBUGF(4, "Done (buf = %p)", buf);
	return buf;
}

/*
* POINT
*/
static size_t rtpoint_to_wkb_size(const RTPOINT *pt, uint8_t variant)
{
	/* Endian flag + type number */
	size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)pt) )
		return empty_to_wkb_size((RTGEOM*)pt, variant);

	/* Extended RTWKB needs space for optional SRID integer */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)pt, variant) )
		size += RTWKB_INT_SIZE;

	/* Points */
	size += ptarray_to_wkb_size(pt->point, variant | RTWKB_NO_NPOINTS);
	return size;
}

static uint8_t* rtpoint_to_wkb_buf(const RTPOINT *pt, uint8_t *buf, uint8_t variant)
{
	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)pt) )
		return empty_to_wkb_buf((RTGEOM*)pt, buf, variant);

	/* Set the endian flag */
	RTDEBUGF(4, "Entering function, buf = %p", buf);
	buf = endian_to_wkb_buf(buf, variant);
	RTDEBUGF(4, "Endian set, buf = %p", buf);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(rtgeom_wkb_type((RTGEOM*)pt, variant), buf, variant);
	RTDEBUGF(4, "Type set, buf = %p", buf);
	/* Set the optional SRID for extended variant */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)pt, variant) )
	{
		buf = integer_to_wkb_buf(pt->srid, buf, variant);
		RTDEBUGF(4, "SRID set, buf = %p", buf);
	}
	/* Set the coordinates */
	buf = ptarray_to_wkb_buf(pt->point, buf, variant | RTWKB_NO_NPOINTS);
	RTDEBUGF(4, "Pointarray set, buf = %p", buf);
	return buf;
}

/*
* LINESTRING, CIRCULARSTRING
*/
static size_t rtline_to_wkb_size(const RTLINE *line, uint8_t variant)
{
	/* Endian flag + type number */
	size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)line) )
		return empty_to_wkb_size((RTGEOM*)line, variant);

	/* Extended RTWKB needs space for optional SRID integer */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)line, variant) )
		size += RTWKB_INT_SIZE;

	/* Size of point array */
	size += ptarray_to_wkb_size(line->points, variant);
	return size;
}

static uint8_t* rtline_to_wkb_buf(const RTLINE *line, uint8_t *buf, uint8_t variant)
{
	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)line) )
		return empty_to_wkb_buf((RTGEOM*)line, buf, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(rtgeom_wkb_type((RTGEOM*)line, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)line, variant) )
		buf = integer_to_wkb_buf(line->srid, buf, variant);
	/* Set the coordinates */
	buf = ptarray_to_wkb_buf(line->points, buf, variant);
	return buf;
}

/*
* TRIANGLE
*/
static size_t rttriangle_to_wkb_size(const RTTRIANGLE *tri, uint8_t variant)
{
	/* endian flag + type number + number of rings */
	size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE + RTWKB_INT_SIZE;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)tri) )
		return empty_to_wkb_size((RTGEOM*)tri, variant);

	/* Extended RTWKB needs space for optional SRID integer */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)tri, variant) )
		size += RTWKB_INT_SIZE;

	/* How big is this point array? */
	size += ptarray_to_wkb_size(tri->points, variant);

	return size;
}

static uint8_t* rttriangle_to_wkb_buf(const RTTRIANGLE *tri, uint8_t *buf, uint8_t variant)
{
	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)tri) )
		return empty_to_wkb_buf((RTGEOM*)tri, buf, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	
	/* Set the geometry type */
	buf = integer_to_wkb_buf(rtgeom_wkb_type((RTGEOM*)tri, variant), buf, variant);
	
	/* Set the optional SRID for extended variant */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)tri, variant) )
		buf = integer_to_wkb_buf(tri->srid, buf, variant);

	/* Set the number of rings (only one, it's a triangle, buddy) */
	buf = integer_to_wkb_buf(1, buf, variant);
	
	/* Write that ring */
	buf = ptarray_to_wkb_buf(tri->points, buf, variant);

	return buf;
}

/*
* POLYGON
*/
static size_t rtpoly_to_wkb_size(const RTPOLY *poly, uint8_t variant)
{
	/* endian flag + type number + number of rings */
	size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE + RTWKB_INT_SIZE;
	int i = 0;
	
	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)poly) )
		return empty_to_wkb_size((RTGEOM*)poly, variant);

	/* Extended RTWKB needs space for optional SRID integer */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)poly, variant) )
		size += RTWKB_INT_SIZE;

	for ( i = 0; i < poly->nrings; i++ )
	{
		/* Size of ring point array */
		size += ptarray_to_wkb_size(poly->rings[i], variant);
	}

	return size;
}

static uint8_t* rtpoly_to_wkb_buf(const RTPOLY *poly, uint8_t *buf, uint8_t variant)
{
	int i;

	/* Only process empty at this level in the EXTENDED case */
	if ( (variant & RTWKB_EXTENDED) && rtgeom_is_empty((RTGEOM*)poly) )
		return empty_to_wkb_buf((RTGEOM*)poly, buf, variant);

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(rtgeom_wkb_type((RTGEOM*)poly, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)poly, variant) )
		buf = integer_to_wkb_buf(poly->srid, buf, variant);
	/* Set the number of rings */
	buf = integer_to_wkb_buf(poly->nrings, buf, variant);

	for ( i = 0; i < poly->nrings; i++ )
	{
		buf = ptarray_to_wkb_buf(poly->rings[i], buf, variant);
	}

	return buf;
}


/*
* MULTIPOINT, MULTILINESTRING, MULTIPOLYGON, GEOMETRYCOLLECTION
* MULTICURVE, COMPOUNDCURVE, MULTISURFACE, CURVEPOLYGON, TIN, 
* POLYHEDRALSURFACE
*/
static size_t rtcollection_to_wkb_size(const RTCOLLECTION *col, uint8_t variant)
{
	/* Endian flag + type number + number of subgeoms */
	size_t size = RTWKB_BYTE_SIZE + RTWKB_INT_SIZE + RTWKB_INT_SIZE;
	int i = 0;

	/* Extended RTWKB needs space for optional SRID integer */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)col, variant) )
		size += RTWKB_INT_SIZE;

	for ( i = 0; i < col->ngeoms; i++ )
	{
		/* size of subgeom */
		size += rtgeom_to_wkb_size((RTGEOM*)col->geoms[i], variant | RTWKB_NO_SRID);
	}

	return size;
}

static uint8_t* rtcollection_to_wkb_buf(const RTCOLLECTION *col, uint8_t *buf, uint8_t variant)
{
	int i;

	/* Set the endian flag */
	buf = endian_to_wkb_buf(buf, variant);
	/* Set the geometry type */
	buf = integer_to_wkb_buf(rtgeom_wkb_type((RTGEOM*)col, variant), buf, variant);
	/* Set the optional SRID for extended variant */
	if ( rtgeom_wkb_needs_srid((RTGEOM*)col, variant) )
		buf = integer_to_wkb_buf(col->srid, buf, variant);
	/* Set the number of sub-geometries */
	buf = integer_to_wkb_buf(col->ngeoms, buf, variant);

	/* Write the sub-geometries. Sub-geometries do not get SRIDs, they
	   inherit from their parents. */
	for ( i = 0; i < col->ngeoms; i++ )
	{
		buf = rtgeom_to_wkb_buf(col->geoms[i], buf, variant | RTWKB_NO_SRID);
	}

	return buf;
}

/*
* GEOMETRY
*/
static size_t rtgeom_to_wkb_size(const RTGEOM *geom, uint8_t variant)
{
	size_t size = 0;

	if ( geom == NULL )
		return 0;

	/* Short circuit out empty geometries */
	if ( (!(variant & RTWKB_EXTENDED)) && rtgeom_is_empty(geom) )
	{
		return empty_to_wkb_size(geom, variant);
	}

	switch ( geom->type )
	{
		case RTPOINTTYPE:
			size += rtpoint_to_wkb_size((RTPOINT*)geom, variant);
			break;

		/* LineString and CircularString both have points elements */
		case RTCIRCSTRINGTYPE:
		case RTLINETYPE:
			size += rtline_to_wkb_size((RTLINE*)geom, variant);
			break;

		/* Polygon has nrings and rings elements */
		case RTPOLYGONTYPE:
			size += rtpoly_to_wkb_size((RTPOLY*)geom, variant);
			break;

		/* Triangle has one ring of three points */
		case RTTRIANGLETYPE:
			size += rttriangle_to_wkb_size((RTTRIANGLE*)geom, variant);
			break;

		/* All these Collection types have ngeoms and geoms elements */
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTCOMPOUNDTYPE:
		case RTCURVEPOLYTYPE:
		case RTMULTICURVETYPE:
		case RTMULTISURFACETYPE:
		case RTCOLLECTIONTYPE:
		case RTPOLYHEDRALSURFACETYPE:
		case RTTINTYPE:
			size += rtcollection_to_wkb_size((RTCOLLECTION*)geom, variant);
			break;

		/* Unknown type! */
		default:
			rterror("Unsupported geometry type: %s [%d]", rttype_name(geom->type), geom->type);
	}

	return size;
}

/* TODO handle the TRIANGLE type properly */

static uint8_t* rtgeom_to_wkb_buf(const RTGEOM *geom, uint8_t *buf, uint8_t variant)
{

	/* Do not simplify empties when outputting to canonical form */
	if ( rtgeom_is_empty(geom) & ! (variant & RTWKB_EXTENDED) )
		return empty_to_wkb_buf(geom, buf, variant);

	switch ( geom->type )
	{
		case RTPOINTTYPE:
			return rtpoint_to_wkb_buf((RTPOINT*)geom, buf, variant);

		/* LineString and CircularString both have 'points' elements */
		case RTCIRCSTRINGTYPE:
		case RTLINETYPE:
			return rtline_to_wkb_buf((RTLINE*)geom, buf, variant);

		/* Polygon has 'nrings' and 'rings' elements */
		case RTPOLYGONTYPE:
			return rtpoly_to_wkb_buf((RTPOLY*)geom, buf, variant);

		/* Triangle has one ring of three points */
		case RTTRIANGLETYPE:
			return rttriangle_to_wkb_buf((RTTRIANGLE*)geom, buf, variant);

		/* All these Collection types have 'ngeoms' and 'geoms' elements */
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTCOMPOUNDTYPE:
		case RTCURVEPOLYTYPE:
		case RTMULTICURVETYPE:
		case RTMULTISURFACETYPE:
		case RTCOLLECTIONTYPE:
		case RTPOLYHEDRALSURFACETYPE:
		case RTTINTYPE:
			return rtcollection_to_wkb_buf((RTCOLLECTION*)geom, buf, variant);

		/* Unknown type! */
		default:
			rterror("Unsupported geometry type: %s [%d]", rttype_name(geom->type), geom->type);
	}
	/* Return value to keep compiler happy. */
	return 0;
}

/**
* Convert RTGEOM to a char* in RTWKB format. Caller is responsible for freeing
* the returned array.
*
* @param variant. Unsigned bitmask value. Accepts one of: RTWKB_ISO, RTWKB_EXTENDED, RTWKB_SFSQL.
* Accepts any of: RTWKB_NDR, RTWKB_HEX. For example: Variant = ( RTWKB_ISO | RTWKB_NDR ) would
* return the little-endian ISO form of RTWKB. For Example: Variant = ( RTWKB_EXTENDED | RTWKB_HEX )
* would return the big-endian extended form of RTWKB, as hex-encoded ASCII (the "canonical form").
* @param size_out If supplied, will return the size of the returned memory segment,
* including the null terminator in the case of ASCII.
*/
uint8_t* rtgeom_to_wkb(const RTGEOM *geom, uint8_t variant, size_t *size_out)
{
	size_t buf_size;
	uint8_t *buf = NULL;
	uint8_t *wkb_out = NULL;

	/* Initialize output size */
	if ( size_out ) *size_out = 0;

	if ( geom == NULL )
	{
		RTDEBUG(4,"Cannot convert NULL into RTWKB.");
		rterror("Cannot convert NULL into RTWKB.");
		return NULL;
	}

	/* Calculate the required size of the output buffer */
	buf_size = rtgeom_to_wkb_size(geom, variant);
	RTDEBUGF(4, "RTWKB output size: %d", buf_size);

	if ( buf_size == 0 )
	{
		RTDEBUG(4,"Error calculating output RTWKB buffer size.");
		rterror("Error calculating output RTWKB buffer size.");
		return NULL;
	}

	/* Hex string takes twice as much space as binary + a null character */
	if ( variant & RTWKB_HEX )
	{
		buf_size = 2 * buf_size + 1;
		RTDEBUGF(4, "Hex RTWKB output size: %d", buf_size);
	}

	/* If neither or both variants are specified, choose the native order */
	if ( ! (variant & RTWKB_NDR || variant & RTWKB_XDR) ||
	       (variant & RTWKB_NDR && variant & RTWKB_XDR) )
	{
		if ( getMachineEndian() == NDR ) 
			variant = variant | RTWKB_NDR;
		else
			variant = variant | RTWKB_XDR;
	}

	/* Allocate the buffer */
	buf = rtalloc(buf_size);

	if ( buf == NULL )
	{
		RTDEBUGF(4,"Unable to allocate %d bytes for RTWKB output buffer.", buf_size);
		rterror("Unable to allocate %d bytes for RTWKB output buffer.", buf_size);
		return NULL;
	}

	/* Retain a pointer to the front of the buffer for later */
	wkb_out = buf;

	/* Write the RTWKB into the output buffer */
	buf = rtgeom_to_wkb_buf(geom, buf, variant);

	/* Null the last byte if this is a hex output */
	if ( variant & RTWKB_HEX )
	{
		*buf = '\0';
		buf++;
	}

	RTDEBUGF(4,"buf (%p) - wkb_out (%p) = %d", buf, wkb_out, buf - wkb_out);

	/* The buffer pointer should now land at the end of the allocated buffer space. Let's check. */
	if ( buf_size != (buf - wkb_out) )
	{
		RTDEBUG(4,"Output RTWKB is not the same size as the allocated buffer.");
		rterror("Output RTWKB is not the same size as the allocated buffer.");
		rtfree(wkb_out);
		return NULL;
	}

	/* Report output size */
	if ( size_out ) *size_out = buf_size;

	return wkb_out;
}

char* rtgeom_to_hexwkb(const RTGEOM *geom, uint8_t variant, size_t *size_out)
{
	return (char*)rtgeom_to_wkb(geom, variant | RTWKB_HEX, size_out);
}

