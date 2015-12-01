/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright 2009-2010 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * ST_MakeValid
 *
 * Attempts to make an invalid geometries valid w/out losing
 * points.
 *
 * Polygons may become lines or points or a collection of
 * polygons lines and points (collapsed ring cases).
 *
 * Author: Sandro Santilli <strk@keybit.net>
 *
 * Work done for Faunalia (http://www.faunalia.it) with fundings
 * from Regione Toscana - Sistema Informativo per il Governo
 * del Territorio e dell'Ambiente (RT-SIGTA).
 *
 * Thanks to Dr. Horst Duester for previous work on a plpgsql version
 * of the cleanup logic [1]
 *
 * Thanks to Andrea Peri for recommandations on constraints.
 *
 * [1] http://www.sogis1.so.ch/sogis/dl/postgis/cleanGeometry.sql
 *
 *
 **********************************************************************/

#include "librtgeom.h"
#include "rtgeom_geos.h"
#include "librtgeom_internal.h"
#include "rtgeom_log.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* #define RTGEOM_DEBUG_LEVEL 4 */
/* #define PARANOIA_LEVEL 2 */
#undef RTGEOM_PROFILE_MAKEVALID


/*
 * Return Nth vertex in GEOSGeometry as a POINT.
 * May return NULL if the geometry has NO vertexex.
 */
GEOSGeometry* RTGEOM_GEOS_getPointN(const GEOSGeometry*, uint32_t);
GEOSGeometry*
RTGEOM_GEOS_getPointN(const GEOSGeometry* g_in, uint32_t n)
{
	uint32_t dims;
	const GEOSCoordSequence* seq_in;
	GEOSCoordSeq seq_out;
	double val;
	uint32_t sz;
	int gn;
	GEOSGeometry* ret;

	switch ( GEOSGeomTypeId(g_in) )
	{
	case GEOS_MULTIPOINT:
	case GEOS_MULTILINESTRING:
	case GEOS_MULTIPOLYGON:
	case GEOS_GEOMETRYCOLLECTION:
	{
		for (gn=0; gn<GEOSGetNumGeometries(g_in); ++gn)
		{
			const GEOSGeometry* g = GEOSGetGeometryN(g_in, gn);
			ret = RTGEOM_GEOS_getPointN(g,n);
			if ( ret ) return ret;
		}
		break;
	}

	case GEOS_POLYGON:
	{
		ret = RTGEOM_GEOS_getPointN(GEOSGetExteriorRing(g_in), n);
		if ( ret ) return ret;
		for (gn=0; gn<GEOSGetNumInteriorRings(g_in); ++gn)
		{
			const GEOSGeometry* g = GEOSGetInteriorRingN(g_in, gn);
			ret = RTGEOM_GEOS_getPointN(g, n);
			if ( ret ) return ret;
		}
		break;
	}

	case GEOS_POINT:
	case GEOS_LINESTRING:
	case GEOS_LINEARRING:
		break;

	}

	seq_in = GEOSGeom_getCoordSeq(g_in);
	if ( ! seq_in ) return NULL;
	if ( ! GEOSCoordSeq_getSize(seq_in, &sz) ) return NULL;
	if ( ! sz ) return NULL;

	if ( ! GEOSCoordSeq_getDimensions(seq_in, &dims) ) return NULL;

	seq_out = GEOSCoordSeq_create(1, dims);
	if ( ! seq_out ) return NULL;

	if ( ! GEOSCoordSeq_getX(seq_in, n, &val) ) return NULL;
	if ( ! GEOSCoordSeq_setX(seq_out, n, val) ) return NULL;
	if ( ! GEOSCoordSeq_getY(seq_in, n, &val) ) return NULL;
	if ( ! GEOSCoordSeq_setY(seq_out, n, val) ) return NULL;
	if ( dims > 2 )
	{
		if ( ! GEOSCoordSeq_getZ(seq_in, n, &val) ) return NULL;
		if ( ! GEOSCoordSeq_setZ(seq_out, n, val) ) return NULL;
	}

	return GEOSGeom_createPoint(seq_out);
}



RTGEOM * rtcollection_make_geos_friendly(RTCOLLECTION *g);
RTGEOM * rtline_make_geos_friendly(RTLINE *line);
RTGEOM * rtpoly_make_geos_friendly(RTPOLY *poly);
POINTARRAY* ring_make_geos_friendly(POINTARRAY* ring);

/*
 * Ensure the geometry is "structurally" valid
 * (enough for GEOS to accept it)
 * May return the input untouched (if already valid).
 * May return geometries of lower dimension (on collapses)
 */
static RTGEOM *
rtgeom_make_geos_friendly(RTGEOM *geom)
{
	RTDEBUGF(2, "rtgeom_make_geos_friendly enter (type %d)", geom->type);
	switch (geom->type)
	{
	case POINTTYPE:
	case MULTIPOINTTYPE:
		/* a point is artays valid */
		return geom;
		break;

	case LINETYPE:
		/* lines need at least 2 points */
		return rtline_make_geos_friendly((RTLINE *)geom);
		break;

	case POLYGONTYPE:
		/* polygons need all rings closed and with npoints > 3 */
		return rtpoly_make_geos_friendly((RTPOLY *)geom);
		break;

	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
		return rtcollection_make_geos_friendly((RTCOLLECTION *)geom);
		break;

	case CIRCSTRINGTYPE:
	case COMPOUNDTYPE:
	case CURVEPOLYTYPE:
	case MULTISURFACETYPE:
	case MULTICURVETYPE:
	default:
		rterror("rtgeom_make_geos_friendly: unsupported input geometry type: %s (%d)", rttype_name(geom->type), geom->type);
		break;
	}
	return 0;
}

/*
 * Close the point array, if not already closed in 2d.
 * Returns the input if already closed in 2d, or a newly
 * constructed POINTARRAY.
 * TODO: move in ptarray.c
 */
POINTARRAY* ptarray_close2d(POINTARRAY* ring);
POINTARRAY*
ptarray_close2d(POINTARRAY* ring)
{
	POINTARRAY* newring;

	/* close the ring if not already closed (2d only) */
	if ( ! ptarray_is_closed_2d(ring) )
	{
		/* close it up */
		newring = ptarray_addPoint(ring,
		                           getPoint_internal(ring, 0),
		                           FLAGS_NDIMS(ring->flags),
		                           ring->npoints);
		ring = newring;
	}
	return ring;
}

/* May return the same input or a new one (never zero) */
POINTARRAY*
ring_make_geos_friendly(POINTARRAY* ring)
{
	POINTARRAY* closedring;
	POINTARRAY* ring_in = ring;

	/* close the ring if not already closed (2d only) */
	closedring = ptarray_close2d(ring);
	if (closedring != ring )
	{
		ring = closedring;
	}

	/* return 0 for collapsed ring (after closeup) */

	while ( ring->npoints < 4 )
	{
		POINTARRAY *oring = ring;
		RTDEBUGF(4, "ring has %d points, adding another", ring->npoints);
		/* let's add another... */
		ring = ptarray_addPoint(ring,
		                        getPoint_internal(ring, 0),
		                        FLAGS_NDIMS(ring->flags),
		                        ring->npoints);
		if ( oring != ring_in ) ptarray_free(oring);
	}


	return ring;
}

/* Make sure all rings are closed and have > 3 points.
 * May return the input untouched.
 */
RTGEOM *
rtpoly_make_geos_friendly(RTPOLY *poly)
{
	RTGEOM* ret;
	POINTARRAY **new_rings;
	int i;

	/* If the polygon has no rings there's nothing to do */
	if ( ! poly->nrings ) return (RTGEOM*)poly;

	/* Allocate enough pointers for all rings */
	new_rings = rtalloc(sizeof(POINTARRAY*)*poly->nrings);

	/* All rings must be closed and have > 3 points */
	for (i=0; i<poly->nrings; i++)
	{
		POINTARRAY* ring_in = poly->rings[i];
		POINTARRAY* ring_out = ring_make_geos_friendly(ring_in);

		if ( ring_in != ring_out )
		{
			RTDEBUGF(3, "rtpoly_make_geos_friendly: ring %d cleaned, now has %d points", i, ring_out->npoints);
			ptarray_free(ring_in);
		}
		else
		{
			RTDEBUGF(3, "rtpoly_make_geos_friendly: ring %d untouched", i);
		}

		assert ( ring_out );
		new_rings[i] = ring_out;
	}

	rtfree(poly->rings);
	poly->rings = new_rings;
	ret = (RTGEOM*)poly;

	return ret;
}

/* Need NO or >1 points. Duplicate first if only one. */
RTGEOM *
rtline_make_geos_friendly(RTLINE *line)
{
	RTGEOM *ret;

	if (line->points->npoints == 1) /* 0 is fine, 2 is fine */
	{
#if 1
		/* Duplicate point */
		line->points = ptarray_addPoint(line->points,
		                                getPoint_internal(line->points, 0),
		                                FLAGS_NDIMS(line->points->flags),
		                                line->points->npoints);
		ret = (RTGEOM*)line;
#else
		/* Turn into a point */
		ret = (RTGEOM*)rtpoint_construct(line->srid, 0, line->points);
#endif
		return ret;
	}
	else
	{
		return (RTGEOM*)line;
		/* return rtline_clone(line); */
	}
}

RTGEOM *
rtcollection_make_geos_friendly(RTCOLLECTION *g)
{
	RTGEOM **new_geoms;
	uint32_t i, new_ngeoms=0;
	RTCOLLECTION *ret;

	/* enough space for all components */
	new_geoms = rtalloc(sizeof(RTGEOM *)*g->ngeoms);

	ret = rtalloc(sizeof(RTCOLLECTION));
	memcpy(ret, g, sizeof(RTCOLLECTION));
    ret->maxgeoms = g->ngeoms;

	for (i=0; i<g->ngeoms; i++)
	{
		RTGEOM* newg = rtgeom_make_geos_friendly(g->geoms[i]);
		if ( newg ) new_geoms[new_ngeoms++] = newg;
	}

	ret->bbox = NULL; /* recompute later... */

	ret->ngeoms = new_ngeoms;
	if ( new_ngeoms )
	{
		ret->geoms = new_geoms;
	}
	else
	{
		free(new_geoms);
		ret->geoms = NULL;
        ret->maxgeoms = 0;
	}

	return (RTGEOM*)ret;
}

/*
 * Fully node given linework
 */
static GEOSGeometry*
RTGEOM_GEOS_nodeLines(const GEOSGeometry* lines)
{
	GEOSGeometry* noded;
	GEOSGeometry* point;

	/*
	 * Union with first geometry point, obtaining full noding
	 * and dissolving of duplicated repeated points
	 *
	 * TODO: substitute this with UnaryUnion?
	 */

	point = RTGEOM_GEOS_getPointN(lines, 0);
	if ( ! point ) return NULL;

	RTDEBUGF(3,
	               "Boundary point: %s",
	               rtgeom_to_ewkt(GEOS2RTGEOM(point, 0)));

	noded = GEOSUnion(lines, point);
	if ( NULL == noded )
	{
		GEOSGeom_destroy(point);
		return NULL;
	}

	GEOSGeom_destroy(point);

	RTDEBUGF(3,
	               "RTGEOM_GEOS_nodeLines: in[%s] out[%s]",
	               rtgeom_to_ewkt(GEOS2RTGEOM(lines, 0)),
	               rtgeom_to_ewkt(GEOS2RTGEOM(noded, 0)));

	return noded;
}

#if RTGEOM_GEOS_VERSION >= 33
/*
 * We expect initGEOS being called already.
 * Will return NULL on error (expect error handler being called by then)
 *
 */
static GEOSGeometry*
RTGEOM_GEOS_makeValidPolygon(const GEOSGeometry* gin)
{
	GEOSGeom gout;
	GEOSGeom geos_bound;
	GEOSGeom geos_cut_edges, geos_area, collapse_points;
	GEOSGeometry *vgeoms[3]; /* One for area, one for cut-edges */
	unsigned int nvgeoms=0;

	assert (GEOSGeomTypeId(gin) == GEOS_POLYGON ||
	        GEOSGeomTypeId(gin) == GEOS_MULTIPOLYGON);

	geos_bound = GEOSBoundary(gin);
	if ( NULL == geos_bound )
	{
		return NULL;
	}

	RTDEBUGF(3,
	               "Boundaries: %s",
	               rtgeom_to_ewkt(GEOS2RTGEOM(geos_bound, 0)));

	/* Use noded boundaries as initial "cut" edges */

#ifdef RTGEOM_PROFILE_MAKEVALID
  rtnotice("ST_MakeValid: noding lines");
#endif


	geos_cut_edges = RTGEOM_GEOS_nodeLines(geos_bound);
	if ( NULL == geos_cut_edges )
	{
		GEOSGeom_destroy(geos_bound);
		rtnotice("RTGEOM_GEOS_nodeLines(): %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* NOTE: the noding process may drop lines collapsing to points.
	 *       We want to retrive any of those */
	{
		GEOSGeometry* pi;
		GEOSGeometry* po;

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: extracting unique points from bounds");
#endif

		pi = GEOSGeom_extractUniquePoints(geos_bound);
		if ( NULL == pi )
		{
			GEOSGeom_destroy(geos_bound);
			rtnotice("GEOSGeom_extractUniquePoints(): %s",
			         rtgeom_geos_errmsg);
			return NULL;
		}

		RTDEBUGF(3,
		               "Boundaries input points %s",
		               rtgeom_to_ewkt(GEOS2RTGEOM(pi, 0)));

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: extracting unique points from cut_edges");
#endif

		po = GEOSGeom_extractUniquePoints(geos_cut_edges);
		if ( NULL == po )
		{
			GEOSGeom_destroy(geos_bound);
			GEOSGeom_destroy(pi);
			rtnotice("GEOSGeom_extractUniquePoints(): %s",
			         rtgeom_geos_errmsg);
			return NULL;
		}

		RTDEBUGF(3,
		               "Boundaries output points %s",
		               rtgeom_to_ewkt(GEOS2RTGEOM(po, 0)));

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: find collapse points");
#endif

		collapse_points = GEOSDifference(pi, po);
		if ( NULL == collapse_points )
		{
			GEOSGeom_destroy(geos_bound);
			GEOSGeom_destroy(pi);
			GEOSGeom_destroy(po);
			rtnotice("GEOSDifference(): %s", rtgeom_geos_errmsg);
			return NULL;
		}

		RTDEBUGF(3,
		               "Collapse points: %s",
		               rtgeom_to_ewkt(GEOS2RTGEOM(collapse_points, 0)));

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: cleanup(1)");
#endif

		GEOSGeom_destroy(pi);
		GEOSGeom_destroy(po);
	}
	GEOSGeom_destroy(geos_bound);

	RTDEBUGF(3,
	               "Noded Boundaries: %s",
	               rtgeom_to_ewkt(GEOS2RTGEOM(geos_cut_edges, 0)));

	/* And use an empty geometry as initial "area" */
	geos_area = GEOSGeom_createEmptyPolygon();
	if ( ! geos_area )
	{
		rtnotice("GEOSGeom_createEmptyPolygon(): %s", rtgeom_geos_errmsg);
		GEOSGeom_destroy(geos_cut_edges);
		return NULL;
	}

	/*
	 * See if an area can be build with the remaining edges
	 * and if it can, symdifference with the original area.
	 * Iterate this until no more polygons can be created
	 * with left-over edges.
	 */
	while (GEOSGetNumGeometries(geos_cut_edges))
	{
		GEOSGeometry* new_area=0;
		GEOSGeometry* new_area_bound=0;
		GEOSGeometry* symdif=0;
		GEOSGeometry* new_cut_edges=0;

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: building area from %d edges", GEOSGetNumGeometries(geos_cut_edges)); 
#endif

		/*
		 * ASSUMPTION: cut_edges should already be fully noded
		 */

		new_area = RTGEOM_GEOS_buildArea(geos_cut_edges);
		if ( ! new_area )   /* must be an exception */
		{
			GEOSGeom_destroy(geos_cut_edges);
			GEOSGeom_destroy(geos_area);
			rtnotice("RTGEOM_GEOS_buildArea() threw an error: %s",
			         rtgeom_geos_errmsg);
			return NULL;
		}

		if ( GEOSisEmpty(new_area) )
		{
			/* no more rings can be build with thes edges */
			GEOSGeom_destroy(new_area);
			break;
		}

		/*
		 * We succeeded in building a ring !
		 */

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: ring built with %d cut edges, saving boundaries", GEOSGetNumGeometries(geos_cut_edges)); 
#endif

		/*
		 * Save the new ring boundaries first (to compute
		 * further cut edges later)
		 */
		new_area_bound = GEOSBoundary(new_area);
		if ( ! new_area_bound )
		{
			/* We did check for empty area already so
			 * this must be some other error */
			rtnotice("GEOSBoundary('%s') threw an error: %s",
			         rtgeom_to_ewkt(GEOS2RTGEOM(new_area, 0)),
			         rtgeom_geos_errmsg);
			GEOSGeom_destroy(new_area);
			GEOSGeom_destroy(geos_area);
			return NULL;
		}

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: running SymDifference with new area"); 
#endif

		/*
		 * Now symdif new and old area
		 */
		symdif = GEOSSymDifference(geos_area, new_area);
		if ( ! symdif )   /* must be an exception */
		{
			GEOSGeom_destroy(geos_cut_edges);
			GEOSGeom_destroy(new_area);
			GEOSGeom_destroy(new_area_bound);
			GEOSGeom_destroy(geos_area);
			rtnotice("GEOSSymDifference() threw an error: %s",
			         rtgeom_geos_errmsg);
			return NULL;
		}

		GEOSGeom_destroy(geos_area);
		GEOSGeom_destroy(new_area);
		geos_area = symdif;
		symdif = 0;

		/*
		 * Now let's re-set geos_cut_edges with what's left
		 * from the original boundary.
		 * ASSUMPTION: only the previous cut-edges can be
		 *             left, so we don't need to reconsider
		 *             the whole original boundaries
		 *
		 * NOTE: this is an expensive operation. 
		 *
		 */

#ifdef RTGEOM_PROFILE_MAKEVALID
    rtnotice("ST_MakeValid: computing new cut_edges (GEOSDifference)"); 
#endif

		new_cut_edges = GEOSDifference(geos_cut_edges, new_area_bound);
		GEOSGeom_destroy(new_area_bound);
		if ( ! new_cut_edges )   /* an exception ? */
		{
			/* cleanup and throw */
			GEOSGeom_destroy(geos_cut_edges);
			GEOSGeom_destroy(geos_area);
			/* TODO: Shouldn't this be an rterror ? */
			rtnotice("GEOSDifference() threw an error: %s",
			         rtgeom_geos_errmsg);
			return NULL;
		}
		GEOSGeom_destroy(geos_cut_edges);
		geos_cut_edges = new_cut_edges;
	}

#ifdef RTGEOM_PROFILE_MAKEVALID
  rtnotice("ST_MakeValid: final checks");
#endif

	if ( ! GEOSisEmpty(geos_area) )
	{
		vgeoms[nvgeoms++] = geos_area;
	}
	else
	{
		GEOSGeom_destroy(geos_area);
	}

	if ( ! GEOSisEmpty(geos_cut_edges) )
	{
		vgeoms[nvgeoms++] = geos_cut_edges;
	}
	else
	{
		GEOSGeom_destroy(geos_cut_edges);
	}

	if ( ! GEOSisEmpty(collapse_points) )
	{
		vgeoms[nvgeoms++] = collapse_points;
	}
	else
	{
		GEOSGeom_destroy(collapse_points);
	}

	if ( 1 == nvgeoms )
	{
		/* Return cut edges */
		gout = vgeoms[0];
	}
	else
	{
		/* Collect areas and lines (if any line) */
		gout = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, vgeoms, nvgeoms);
		if ( ! gout )   /* an exception again */
		{
			/* cleanup and throw */
			/* TODO: Shouldn't this be an rterror ? */
			rtnotice("GEOSGeom_createCollection() threw an error: %s",
			         rtgeom_geos_errmsg);
			/* TODO: cleanup! */
			return NULL;
		}
	}

	return gout;

}

static GEOSGeometry*
RTGEOM_GEOS_makeValidLine(const GEOSGeometry* gin)
{
	GEOSGeometry* noded;
	noded = RTGEOM_GEOS_nodeLines(gin);
	return noded;
}

static GEOSGeometry*
RTGEOM_GEOS_makeValidMultiLine(const GEOSGeometry* gin)
{
	GEOSGeometry** lines;
	GEOSGeometry** points;
	GEOSGeometry* mline_out=0;
	GEOSGeometry* mpoint_out=0;
	GEOSGeometry* gout=0;
	uint32_t nlines=0, nlines_alloc;
	uint32_t npoints=0;
	uint32_t ngeoms=0, nsubgeoms;
	uint32_t i, j;

	ngeoms = GEOSGetNumGeometries(gin);

	nlines_alloc = ngeoms;
	lines = rtalloc(sizeof(GEOSGeometry*)*nlines_alloc);
	points = rtalloc(sizeof(GEOSGeometry*)*ngeoms);

	for (i=0; i<ngeoms; ++i)
	{
		const GEOSGeometry* g = GEOSGetGeometryN(gin, i);
		GEOSGeometry* vg;
		vg = RTGEOM_GEOS_makeValidLine(g);
		if ( GEOSisEmpty(vg) )
		{
			/* we don't care about this one */
			GEOSGeom_destroy(vg);
		}
		if ( GEOSGeomTypeId(vg) == GEOS_POINT )
		{
			points[npoints++] = vg;
		}
		else if ( GEOSGeomTypeId(vg) == GEOS_LINESTRING )
		{
			lines[nlines++] = vg;
		}
		else if ( GEOSGeomTypeId(vg) == GEOS_MULTILINESTRING )
		{
			nsubgeoms=GEOSGetNumGeometries(vg);
			nlines_alloc += nsubgeoms;
			lines = rtrealloc(lines, sizeof(GEOSGeometry*)*nlines_alloc);
			for (j=0; j<nsubgeoms; ++j)
			{
				const GEOSGeometry* gc = GEOSGetGeometryN(vg, j);
				/* NOTE: ownership of the cloned geoms will be
				 *       taken by final collection */
				lines[nlines++] = GEOSGeom_clone(gc);
			}
		}
		else
		{
			/* NOTE: return from GEOSGeomType will leak
			 * but we really don't expect this to happen */
			rterror("unexpected geom type returned "
			        "by RTGEOM_GEOS_makeValid: %s",
			        GEOSGeomType(vg));
		}
	}

	if ( npoints )
	{
		if ( npoints > 1 )
		{
			mpoint_out = GEOSGeom_createCollection(GEOS_MULTIPOINT,
			                                       points, npoints);
		}
		else
		{
			mpoint_out = points[0];
		}
	}

	if ( nlines )
	{
		if ( nlines > 1 )
		{
			mline_out = GEOSGeom_createCollection(
			                GEOS_MULTILINESTRING, lines, nlines);
		}
		else
		{
			mline_out = lines[0];
		}
	}

	rtfree(lines);

	if ( mline_out && mpoint_out )
	{
		points[0] = mline_out;
		points[1] = mpoint_out;
		gout = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION,
		                                 points, 2);
	}
	else if ( mline_out )
	{
		gout = mline_out;
	}
	else if ( mpoint_out )
	{
		gout = mpoint_out;
	}

	rtfree(points);

	return gout;
}

static GEOSGeometry* RTGEOM_GEOS_makeValid(const GEOSGeometry*);

/*
 * We expect initGEOS being called already.
 * Will return NULL on error (expect error handler being called by then)
 */
static GEOSGeometry*
RTGEOM_GEOS_makeValidCollection(const GEOSGeometry* gin)
{
	int nvgeoms;
	GEOSGeometry **vgeoms;
	GEOSGeom gout;
	unsigned int i;

	nvgeoms = GEOSGetNumGeometries(gin);
	if ( nvgeoms == -1 ) {
		rterror("GEOSGetNumGeometries: %s", rtgeom_geos_errmsg);
		return 0;
	}

	vgeoms = rtalloc( sizeof(GEOSGeometry*) * nvgeoms );
	if ( ! vgeoms ) {
		rterror("RTGEOM_GEOS_makeValidCollection: out of memory");
		return 0;
	}

	for ( i=0; i<nvgeoms; ++i ) {
		vgeoms[i] = RTGEOM_GEOS_makeValid( GEOSGetGeometryN(gin, i) );
		if ( ! vgeoms[i] ) {
			while (i--) GEOSGeom_destroy(vgeoms[i]);
			rtfree(vgeoms);
			/* we expect rterror being called already by makeValid */
			return NULL;
		}
	}

	/* Collect areas and lines (if any line) */
	gout = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, vgeoms, nvgeoms);
	if ( ! gout )   /* an exception again */
	{
		/* cleanup and throw */
		for ( i=0; i<nvgeoms; ++i ) GEOSGeom_destroy(vgeoms[i]);
		rtfree(vgeoms);
		rterror("GEOSGeom_createCollection() threw an error: %s",
		         rtgeom_geos_errmsg);
		return NULL;
	}
	rtfree(vgeoms);

	return gout;

}


static GEOSGeometry*
RTGEOM_GEOS_makeValid(const GEOSGeometry* gin)
{
	GEOSGeometry* gout;
	char ret_char;

	/*
	 * Step 2: return what we got so far if already valid
	 */

	ret_char = GEOSisValid(gin);
	if ( ret_char == 2 )
	{
		/* I don't think should ever happen */
		rterror("GEOSisValid(): %s", rtgeom_geos_errmsg);
		return NULL;
	}
	else if ( ret_char )
	{
		RTDEBUGF(3,
		               "Geometry [%s] is valid. ",
		               rtgeom_to_ewkt(GEOS2RTGEOM(gin, 0)));

		/* It's valid at this step, return what we have */
		return GEOSGeom_clone(gin);
	}

	RTDEBUGF(3,
	               "Geometry [%s] is still not valid: %s. "
	               "Will try to clean up further.",
	               rtgeom_to_ewkt(GEOS2RTGEOM(gin, 0)), rtgeom_geos_errmsg);



	/*
	 * Step 3 : make what we got valid
	 */

	switch (GEOSGeomTypeId(gin))
	{
	case GEOS_MULTIPOINT:
	case GEOS_POINT:
		/* points are artays valid, but we might have invalid ordinate values */
		rtnotice("PUNTUAL geometry resulted invalid to GEOS -- dunno how to clean that up");
		return NULL;
		break;

	case GEOS_LINESTRING:
		gout = RTGEOM_GEOS_makeValidLine(gin);
		if ( ! gout )  /* an exception or something */
		{
			/* cleanup and throw */
			rterror("%s", rtgeom_geos_errmsg);
			return NULL;
		}
		break; /* we've done */

	case GEOS_MULTILINESTRING:
		gout = RTGEOM_GEOS_makeValidMultiLine(gin);
		if ( ! gout )  /* an exception or something */
		{
			/* cleanup and throw */
			rterror("%s", rtgeom_geos_errmsg);
			return NULL;
		}
		break; /* we've done */

	case GEOS_POLYGON:
	case GEOS_MULTIPOLYGON:
	{
		gout = RTGEOM_GEOS_makeValidPolygon(gin);
		if ( ! gout )  /* an exception or something */
		{
			/* cleanup and throw */
			rterror("%s", rtgeom_geos_errmsg);
			return NULL;
		}
		break; /* we've done */
	}

	case GEOS_GEOMETRYCOLLECTION:
	{
		gout = RTGEOM_GEOS_makeValidCollection(gin);
		if ( ! gout )  /* an exception or something */
		{
			/* cleanup and throw */
			rterror("%s", rtgeom_geos_errmsg);
			return NULL;
		}
		break; /* we've done */
	}

	default:
	{
		char* typname = GEOSGeomType(gin);
		rtnotice("ST_MakeValid: doesn't support geometry type: %s",
		         typname);
		GEOSFree(typname);
		return NULL;
		break;
	}
	}

#if PARANOIA_LEVEL > 1
	/*
	 * Now check if every point of input is also found
	 * in output, or abort by returning NULL
	 *
	 * Input geometry was rtgeom_in
	 */
	{
			int loss;
			GEOSGeometry *pi, *po, *pd;

			/* TODO: handle some errors here...
			 * Lack of exceptions is annoying indeed,
			 * I'm getting old --strk;
			 */
			pi = GEOSGeom_extractUniquePoints(gin);
			po = GEOSGeom_extractUniquePoints(gout);
			pd = GEOSDifference(pi, po); /* input points - output points */
			GEOSGeom_destroy(pi);
			GEOSGeom_destroy(po);
			loss = !GEOSisEmpty(pd);
			GEOSGeom_destroy(pd);
			if ( loss )
			{
				rtnotice("Vertices lost in RTGEOM_GEOS_makeValid");
				/* return NULL */
			}
	}
#endif /* PARANOIA_LEVEL > 1 */


	return gout;
}

/* Exported. Uses GEOS internally */
RTGEOM*
rtgeom_make_valid(RTGEOM* rtgeom_in)
{
	int is3d;
	GEOSGeom geosgeom;
	GEOSGeometry* geosout;
	RTGEOM *rtgeom_out;

	is3d = FLAGS_GET_Z(rtgeom_in->flags);

	/*
	 * Step 1 : try to convert to GEOS, if impossible, clean that up first
	 *          otherwise (adding only duplicates of existing points)
	 */

	initGEOS(rtgeom_geos_error, rtgeom_geos_error);

	rtgeom_out = rtgeom_in;
	geosgeom = RTGEOM2GEOS(rtgeom_out, 0);
	if ( ! geosgeom )
	{
		RTDEBUGF(4,
		               "Original geom can't be converted to GEOS (%s)"
		               " - will try cleaning that up first",
		               rtgeom_geos_errmsg);


		rtgeom_out = rtgeom_make_geos_friendly(rtgeom_out);
		if ( ! rtgeom_out )
		{
			rterror("Could not make a valid geometry out of input");
		}

		/* try again as we did cleanup now */
		/* TODO: invoke RTGEOM2GEOS directly with autoclean ? */
		geosgeom = RTGEOM2GEOS(rtgeom_out, 0);
		if ( ! geosgeom )
		{
			rterror("Couldn't convert RTGEOM geom to GEOS: %s",
			        rtgeom_geos_errmsg);
			return NULL;
		}

	}
	else
	{
		RTDEBUG(4, "original geom converted to GEOS");
		rtgeom_out = rtgeom_in;
	}

	geosout = RTGEOM_GEOS_makeValid(geosgeom);
	GEOSGeom_destroy(geosgeom);
	if ( ! geosout )
	{
		return NULL;
	}

	rtgeom_out = GEOS2RTGEOM(geosout, is3d);
	GEOSGeom_destroy(geosout);

	if ( rtgeom_is_collection(rtgeom_in) && ! rtgeom_is_collection(rtgeom_out) )
	{{
		RTGEOM **ogeoms = rtalloc(sizeof(RTGEOM*));
		RTGEOM *ogeom;
		RTDEBUG(3, "rtgeom_make_valid: forcing multi");
		/* NOTE: this is safe because rtgeom_out is surely not rtgeom_in or
		 * otherwise we couldn't have a collection and a non-collection */
		assert(rtgeom_in != rtgeom_out);
		ogeoms[0] = rtgeom_out;
		ogeom = (RTGEOM *)rtcollection_construct(MULTITYPE[rtgeom_out->type],
		                          rtgeom_out->srid, rtgeom_out->bbox, 1, ogeoms);
		rtgeom_out->bbox = NULL;
		rtgeom_out = ogeom;
	}}

	rtgeom_out->srid = rtgeom_in->srid;
	return rtgeom_out;
}

#endif /* RTGEOM_GEOS_VERSION >= 33 */

