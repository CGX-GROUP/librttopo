/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright 2011-2015 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * Split (multi)polygon by line, line by (multi)line
 * or (multi)polygon boundary, line by point.
 * Returns at most components as a collection.
 * First element of the collection is artays the part which
 * remains after the cut, while the second element is the
 * part which has been cut out. We arbitrarely take the part
 * on the *right* of cut lines as the part which has been cut out.
 * For a line cut by a point the part which remains is the one
 * from start of the line to the cut point.
 *
 * Author: Sandro Santilli <strk@keybit.net>
 *
 * Work done for Faunalia (http://www.faunalia.it) with fundings
 * from Regione Toscana - Sistema Informativo per il Governo
 * del Territorio e dell'Ambiente (RT-SIGTA).
 *
 * Thanks to the PostGIS community for sharing poly/line ideas [1]
 *
 * [1] http://trac.osgeo.org/postgis/wiki/UsersWikiSplitPolygonWithLineString
 *
 * Further evolved for RT-SITA to allow splitting lines by multilines
 * and (multi)polygon boundaries (CIG 6002233F59)
 *
 **********************************************************************/

#include "rtgeom_geos.h"
#include "librtgeom_internal.h"

#include <string.h>
#include <assert.h>

static RTGEOM* rtline_split_by_line(const RTLINE* rtgeom_in, const RTGEOM* blade_in);
static RTGEOM* rtline_split_by_point(const RTLINE* rtgeom_in, const RTPOINT* blade_in);
static RTGEOM* rtline_split_by_mpoint(const RTLINE* rtgeom_in, const RTMPOINT* blade_in);
static RTGEOM* rtline_split(const RTLINE* rtgeom_in, const RTGEOM* blade_in);
static RTGEOM* rtpoly_split_by_line(const RTPOLY* rtgeom_in, const RTLINE* blade_in);
static RTGEOM* rtcollection_split(const RTCOLLECTION* rtcoll_in, const RTGEOM* blade_in);
static RTGEOM* rtpoly_split(const RTPOLY* rtpoly_in, const RTGEOM* blade_in);

/* Initializes and uses GEOS internally */
static RTGEOM*
rtline_split_by_line(const RTLINE* rtline_in, const RTGEOM* blade_in)
{
	RTGEOM** components;
	RTGEOM* diff;
	RTCOLLECTION* out;
	GEOSGeometry* gdiff; /* difference */
	GEOSGeometry* g1;
	GEOSGeometry* g2;
	int ret;

	/* ASSERT blade_in is LINE or MULTILINE */
	assert (blade_in->type == RTLINETYPE ||
	        blade_in->type == RTMULTILINETYPE ||
	        blade_in->type == RTPOLYGONTYPE ||
	        blade_in->type == RTMULTIPOLYGONTYPE );

	/* Possible outcomes:
	 *
	 *  1. The lines do not cross or overlap
	 *      -> Return a collection with single element
	 *  2. The lines cross
	 *      -> Return a collection of all elements resulting from the split
	 */

	initGEOS(rtgeom_geos_error, rtgeom_geos_error);

	g1 = RTGEOM2GEOS((RTGEOM*)rtline_in, 0);
	if ( ! g1 )
	{
		rterror("RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}
	g2 = RTGEOM2GEOS(blade_in, 0);
	if ( ! g2 )
	{
		GEOSGeom_destroy(g1);
		rterror("RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* If blade is a polygon, pick its boundary */
	if ( blade_in->type == RTPOLYGONTYPE || blade_in->type == RTMULTIPOLYGONTYPE )
	{
		gdiff = GEOSBoundary(g2);
		GEOSGeom_destroy(g2);
		if ( ! gdiff )
		{
			GEOSGeom_destroy(g1);
			rterror("GEOSBoundary: %s", rtgeom_geos_errmsg);
			return NULL;
		}
		g2 = gdiff; gdiff = NULL;
	}

	/* If interior intersecton is linear we can't split */
	ret = GEOSRelatePattern(g1, g2, "1********");
	if ( 2 == ret )
	{
		rterror("GEOSRelatePattern: %s", rtgeom_geos_errmsg);
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		return NULL;
	}
	if ( ret )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		rterror("Splitter line has linear intersection with input");
		return NULL;
	}


	gdiff = GEOSDifference(g1,g2);
	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	if (gdiff == NULL)
	{
		rterror("GEOSDifference: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	diff = GEOS2RTGEOM(gdiff, RTFLAGS_GET_Z(rtline_in->flags));
	GEOSGeom_destroy(gdiff);
	if (NULL == diff)
	{
		rterror("GEOS2RTGEOM: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	out = rtgeom_as_rtcollection(diff);
	if ( ! out )
	{
		components = rtalloc(sizeof(RTGEOM*)*1);
		components[0] = diff;
		out = rtcollection_construct(RTCOLLECTIONTYPE, rtline_in->srid,
		                             NULL, 1, components);
	}
	else
	{
	  /* Set SRID */
		rtgeom_set_srid((RTGEOM*)out, rtline_in->srid);
	  /* Force collection type */
	  out->type = RTCOLLECTIONTYPE;
	}


	return (RTGEOM*)out;
}

static RTGEOM*
rtline_split_by_point(const RTLINE* rtline_in, const RTPOINT* blade_in)
{
	RTMLINE* out;

	out = rtmline_construct_empty(rtline_in->srid,
		RTFLAGS_GET_Z(rtline_in->flags),
		RTFLAGS_GET_M(rtline_in->flags));
	if ( rtline_split_by_point_to(rtline_in, blade_in, out) < 2 )
	{
		rtmline_add_rtline(out, rtline_clone_deep(rtline_in));
	}

	/* Turn multiline into collection */
	out->type = RTCOLLECTIONTYPE;

	return (RTGEOM*)out;
}

static RTGEOM*
rtline_split_by_mpoint(const RTLINE* rtline_in, const RTMPOINT* mp)
{
  RTMLINE* out;
  int i, j;

  out = rtmline_construct_empty(rtline_in->srid,
          RTFLAGS_GET_Z(rtline_in->flags),
          RTFLAGS_GET_M(rtline_in->flags));
  rtmline_add_rtline(out, rtline_clone_deep(rtline_in));

  for (i=0; i<mp->ngeoms; ++i)
  {
    for (j=0; j<out->ngeoms; ++j)
    {
      rtline_in = out->geoms[j];
      RTPOINT *blade_in = mp->geoms[i];
      int ret = rtline_split_by_point_to(rtline_in, blade_in, out);
      if ( 2 == ret )
      {
        /* the point splits this line,
         * 2 splits were added to collection.
         * We'll move the latest added into
         * the slot of the current one.
         */
        rtline_free(out->geoms[j]);
        out->geoms[j] = out->geoms[--out->ngeoms];
      }
    }
  }

  /* Turn multiline into collection */
  out->type = RTCOLLECTIONTYPE;

  return (RTGEOM*)out;
}

int
rtline_split_by_point_to(const RTLINE* rtline_in, const RTPOINT* blade_in,
                         RTMLINE* v)
{
	double loc, dist;
	RTPOINT4D pt, pt_projected;
	RTPOINTARRAY* pa1;
	RTPOINTARRAY* pa2;
	double vstol; /* vertex snap tolerance */

	/* Possible outcomes:
	 *
	 *  1. The point is not on the line or on the boundary
	 *      -> Leave collection untouched, return 0
	 *  2. The point is on the boundary
	 *      -> Leave collection untouched, return 1
	 *  3. The point is in the line
	 *      -> Push 2 elements on the collection:
	 *         o start_point - cut_point
	 *         o cut_point - last_point
	 *      -> Return 2
	 */

	getPoint4d_p(blade_in->point, 0, &pt);
	loc = ptarray_locate_point(rtline_in->points, &pt, &dist, &pt_projected);

	/* rtnotice("Location: %g -- Distance: %g", loc, dist); */

	if ( dist > 0 )   /* TODO: accept a tolerance ? */
	{
		/* No intersection */
		return 0;
	}

	if ( loc == 0 || loc == 1 )
	{
		/* Intersection is on the boundary */
		return 1;
	}

	/* There is a real intersection, let's get two substrings */

	/* Compute vertex snap tolerance based on line length
	 * TODO: take as parameter ? */
	vstol = ptarray_length_2d(rtline_in->points) / 1e14;

	pa1 = ptarray_substring(rtline_in->points, 0, loc, vstol);
	pa2 = ptarray_substring(rtline_in->points, loc, 1, vstol);

	/* NOTE: I've seen empty pointarrays with loc != 0 and loc != 1 */
	if ( pa1->npoints == 0 || pa2->npoints == 0 ) {
		ptarray_free(pa1);
		ptarray_free(pa2);
		/* Intersection is on the boundary */
		return 1;
	}

	rtmline_add_rtline(v, rtline_construct(SRID_UNKNOWN, NULL, pa1));
	rtmline_add_rtline(v, rtline_construct(SRID_UNKNOWN, NULL, pa2));
	return 2;
}

static RTGEOM*
rtline_split(const RTLINE* rtline_in, const RTGEOM* blade_in)
{
	switch (blade_in->type)
	{
	case RTPOINTTYPE:
		return rtline_split_by_point(rtline_in, (RTPOINT*)blade_in);
	case RTMULTIPOINTTYPE:
		return rtline_split_by_mpoint(rtline_in, (RTMPOINT*)blade_in);

	case RTLINETYPE:
	case RTMULTILINETYPE:
	case RTPOLYGONTYPE:
	case RTMULTIPOLYGONTYPE:
		return rtline_split_by_line(rtline_in, blade_in);

	default:
		rterror("Splitting a Line by a %s is unsupported",
		        rttype_name(blade_in->type));
		return NULL;
	}
	return NULL;
}

/* Initializes and uses GEOS internally */
static RTGEOM*
rtpoly_split_by_line(const RTPOLY* rtpoly_in, const RTLINE* blade_in)
{
	RTCOLLECTION* out;
	GEOSGeometry* g1;
	GEOSGeometry* g2;
	GEOSGeometry* g1_bounds;
	GEOSGeometry* polygons;
	const GEOSGeometry *vgeoms[1];
	int i,n;
	int hasZ = RTFLAGS_GET_Z(rtpoly_in->flags);


	/* Possible outcomes:
	 *
	 *  1. The line does not split the polygon
	 *      -> Return a collection with single element
	 *  2. The line does split the polygon
	 *      -> Return a collection of all elements resulting from the split
	 */

	initGEOS(rtgeom_geos_error, rtgeom_geos_error);

	g1 = RTGEOM2GEOS((RTGEOM*)rtpoly_in, 0);
	if ( NULL == g1 )
	{
		rterror("RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}
	g1_bounds = GEOSBoundary(g1);
	if ( NULL == g1_bounds )
	{
		GEOSGeom_destroy(g1);
		rterror("GEOSBoundary: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	g2 = RTGEOM2GEOS((RTGEOM*)blade_in, 0);
	if ( NULL == g2 )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g1_bounds);
		rterror("RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	vgeoms[0] = GEOSUnion(g1_bounds, g2);
	if ( NULL == vgeoms[0] )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g1_bounds);
		rterror("GEOSUnion: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* debugging..
		rtnotice("Bounds poly: %s",
		               rtgeom_to_ewkt(GEOS2RTGEOM(g1_bounds, hasZ)));
		rtnotice("Line: %s",
		               rtgeom_to_ewkt(GEOS2RTGEOM(g2, hasZ)));

		rtnotice("Noded bounds: %s",
		               rtgeom_to_ewkt(GEOS2RTGEOM(vgeoms[0], hasZ)));
	*/

	polygons = GEOSPolygonize(vgeoms, 1);
	if ( NULL == polygons )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g1_bounds);
		GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
		rterror("GEOSPolygonize: %s", rtgeom_geos_errmsg);
		return NULL;
	}

#if PARANOIA_LEVEL > 0
	if ( GEOSGeometryTypeId(polygons) != RTCOLLECTIONTYPE )
	{
		GEOSGeom_destroy(g1);
		GEOSGeom_destroy(g2);
		GEOSGeom_destroy(g1_bounds);
		GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
		GEOSGeom_destroy(polygons);
		rterror("Unexpected return from GEOSpolygonize");
		return 0;
	}
#endif

	/* We should now have all polygons, just skip
	 * the ones which are in holes of the original
	 * geometries and return the rest in a collection
	 */
	n = GEOSGetNumGeometries(polygons);
	out = rtcollection_construct_empty(RTCOLLECTIONTYPE, rtpoly_in->srid,
				     hasZ, 0);
	/* Allocate space for all polys */
	out->geoms = rtrealloc(out->geoms, sizeof(RTGEOM*)*n);
	assert(0 == out->ngeoms);
	for (i=0; i<n; ++i)
	{
		GEOSGeometry* pos; /* point on surface */
		const GEOSGeometry* p = GEOSGetGeometryN(polygons, i);
		int contains;

		pos = GEOSPointOnSurface(p);
		if ( ! pos )
		{
			GEOSGeom_destroy(g1);
			GEOSGeom_destroy(g2);
			GEOSGeom_destroy(g1_bounds);
			GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
			GEOSGeom_destroy(polygons);
			rterror("GEOSPointOnSurface: %s", rtgeom_geos_errmsg);
			return NULL;
		}

		contains = GEOSContains(g1, pos);
		if ( 2 == contains )
		{
			GEOSGeom_destroy(g1);
			GEOSGeom_destroy(g2);
			GEOSGeom_destroy(g1_bounds);
			GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
			GEOSGeom_destroy(polygons);
			GEOSGeom_destroy(pos);
			rterror("GEOSContains: %s", rtgeom_geos_errmsg);
			return NULL;
		}

		GEOSGeom_destroy(pos);

		if ( 0 == contains )
		{
			/* Original geometry doesn't contain
			 * a point in this ring, must be an hole
			 */
			continue;
		}

		out->geoms[out->ngeoms++] = GEOS2RTGEOM(p, hasZ);
	}

	GEOSGeom_destroy(g1);
	GEOSGeom_destroy(g2);
	GEOSGeom_destroy(g1_bounds);
	GEOSGeom_destroy((GEOSGeometry*)vgeoms[0]);
	GEOSGeom_destroy(polygons);

	return (RTGEOM*)out;
}

static RTGEOM*
rtcollection_split(const RTCOLLECTION* rtcoll_in, const RTGEOM* blade_in)
{
	RTGEOM** split_vector=NULL;
	RTCOLLECTION* out;
	size_t split_vector_capacity;
	size_t split_vector_size=0;
	size_t i,j;

	split_vector_capacity=8;
	split_vector = rtalloc(split_vector_capacity * sizeof(RTGEOM*));
	if ( ! split_vector )
	{
		rterror("Out of virtual memory");
		return NULL;
	}

	for (i=0; i<rtcoll_in->ngeoms; ++i)
	{
		RTCOLLECTION* col;
		RTGEOM* split = rtgeom_split(rtcoll_in->geoms[i], blade_in);
		/* an exception should prevent this from ever returning NULL */
		if ( ! split ) return NULL;

		col = rtgeom_as_rtcollection(split);
		/* Output, if any, will artays be a collection */
		assert(col);

		/* Reallocate split_vector if needed */
		if ( split_vector_size + col->ngeoms > split_vector_capacity )
		{
			/* NOTE: we could be smarter on reallocations here */
			split_vector_capacity += col->ngeoms;
			split_vector = rtrealloc(split_vector,
			                         split_vector_capacity * sizeof(RTGEOM*));
			if ( ! split_vector )
			{
				rterror("Out of virtual memory");
				return NULL;
			}
		}

		for (j=0; j<col->ngeoms; ++j)
		{
			col->geoms[j]->srid = SRID_UNKNOWN; /* strip srid */
			split_vector[split_vector_size++] = col->geoms[j];
		}
		rtfree(col->geoms);
		rtfree(col);
	}

	/* Now split_vector has split_vector_size geometries */
	out = rtcollection_construct(RTCOLLECTIONTYPE, rtcoll_in->srid,
	                             NULL, split_vector_size, split_vector);

	return (RTGEOM*)out;
}

static RTGEOM*
rtpoly_split(const RTPOLY* rtpoly_in, const RTGEOM* blade_in)
{
	switch (blade_in->type)
	{
	case RTLINETYPE:
		return rtpoly_split_by_line(rtpoly_in, (RTLINE*)blade_in);
	default:
		rterror("Splitting a Polygon by a %s is unsupported",
		        rttype_name(blade_in->type));
		return NULL;
	}
	return NULL;
}

/* exported */
RTGEOM*
rtgeom_split(const RTGEOM* rtgeom_in, const RTGEOM* blade_in)
{
	switch (rtgeom_in->type)
	{
	case RTLINETYPE:
		return rtline_split((const RTLINE*)rtgeom_in, blade_in);

	case RTPOLYGONTYPE:
		return rtpoly_split((const RTPOLY*)rtgeom_in, blade_in);

	case RTMULTIPOLYGONTYPE:
	case RTMULTILINETYPE:
	case RTCOLLECTIONTYPE:
		return rtcollection_split((const RTCOLLECTION*)rtgeom_in, blade_in);

	default:
		rterror("Splitting of %s geometries is unsupported",
		        rttype_name(rtgeom_in->type));
		return NULL;
	}

}

