/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2011 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************
 *
 * Node a set of linestrings 
 *
 **********************************************************************/

#include "rtgeom_geos.h"
#include "librtgeom_internal.h"

#include <string.h>
#include <assert.h>

static int
rtgeom_ngeoms(const RTGEOM* n)
{
	const RTCOLLECTION* c = rtgeom_as_rtcollection(n);
	if ( c ) return c->ngeoms;
	else return 1;
}

static const RTGEOM*
rtgeom_subgeom(const RTGEOM* g, int n)
{
	const RTCOLLECTION* c = rtgeom_as_rtcollection(g);
	if ( c ) return rtcollection_getsubgeom((RTCOLLECTION*)c, n);
	else return g;
}


static void
rtgeom_collect_endpoints(const RTGEOM* rtg, RTMPOINT* col)
{
	int i, n;
	RTLINE* l;

	switch (rtg->type)
	{
		case MULTILINETYPE:
			for ( i = 0,
			        n = rtgeom_ngeoms(rtg);
			      i < n; ++i )
			{
				rtgeom_collect_endpoints(
					rtgeom_subgeom(rtg, i),
					col);
			}
			break;
		case LINETYPE:
			l = (RTLINE*)rtg;
			col = rtmpoint_add_rtpoint(col,
				rtline_get_rtpoint(l, 0));
			col = rtmpoint_add_rtpoint(col,
				rtline_get_rtpoint(l, l->points->npoints-1));
			break;
		default:
			rterror("rtgeom_collect_endpoints: invalid type %s",
				rttype_name(rtg->type));
			break;
	}
}

static RTMPOINT*
rtgeom_extract_endpoints(const RTGEOM* rtg)
{
	RTMPOINT* col = rtmpoint_construct_empty(SRID_UNKNOWN,
	                              FLAGS_GET_Z(rtg->flags),
	                              FLAGS_GET_M(rtg->flags));
	rtgeom_collect_endpoints(rtg, col);

	return col;
}

/* Assumes initGEOS was called already */
/* May return RTPOINT or RTMPOINT */
static RTGEOM*
rtgeom_extract_unique_endpoints(const RTGEOM* rtg)
{
#if RTGEOM_GEOS_VERSION < 33
	rterror("The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'GEOSUnaryUnion' function (3.3.0+ required)",
	        RTGEOM_GEOS_VERSION);
	return NULL;
#else /* RTGEOM_GEOS_VERSION >= 33 */
	RTGEOM* ret;
	GEOSGeometry *gepu;
	RTMPOINT *epall = rtgeom_extract_endpoints(rtg);
	GEOSGeometry *gepall = RTGEOM2GEOS((RTGEOM*)epall, 1);
	rtmpoint_free(epall);
	if ( ! gepall ) {
		rterror("RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* UnaryUnion to remove duplicates */
	/* TODO: do it all within pgis using indices */
	gepu = GEOSUnaryUnion(gepall);
	if ( ! gepu ) {
		GEOSGeom_destroy(gepall);
		rterror("GEOSUnaryUnion: %s", rtgeom_geos_errmsg);
		return NULL;
	}
	GEOSGeom_destroy(gepall);

	ret = GEOS2RTGEOM(gepu, FLAGS_GET_Z(rtg->flags));
	GEOSGeom_destroy(gepu);
	if ( ! ret ) {
		rterror("Error during GEOS2RTGEOM");
		return NULL;
	}

	return ret;
#endif /* RTGEOM_GEOS_VERSION >= 33 */
}

/* exported */
extern RTGEOM* rtgeom_node(const RTGEOM* rtgeom_in);
RTGEOM*
rtgeom_node(const RTGEOM* rtgeom_in)
{
#if RTGEOM_GEOS_VERSION < 33
	rterror("The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'GEOSUnaryUnion' function (3.3.0+ required)",
	        RTGEOM_GEOS_VERSION);
	return NULL;
#else /* RTGEOM_GEOS_VERSION >= 33 */
	GEOSGeometry *g1, *gu, *gm;
	RTGEOM *ep, *lines;
	RTCOLLECTION *col, *tc;
	int pn, ln, np, nl;

	if ( rtgeom_dimension(rtgeom_in) != 1 ) {
		rterror("Noding geometries of dimension != 1 is unsupported");
		return NULL;
	}

	initGEOS(rtgeom_geos_error, rtgeom_geos_error);
	g1 = RTGEOM2GEOS(rtgeom_in, 1);
	if ( ! g1 ) {
		rterror("RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	ep = rtgeom_extract_unique_endpoints(rtgeom_in);
	if ( ! ep ) {
		GEOSGeom_destroy(g1);
		rterror("Error extracting unique endpoints from input");
		return NULL;
	}

	/* Unary union input to fully node */
	gu = GEOSUnaryUnion(g1);
	GEOSGeom_destroy(g1);
	if ( ! gu ) {
		rtgeom_free(ep);
		rterror("GEOSUnaryUnion: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* Linemerge (in case of overlaps) */
	gm = GEOSLineMerge(gu);
	GEOSGeom_destroy(gu);
	if ( ! gm ) {
		rtgeom_free(ep);
		rterror("GEOSLineMerge: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	lines = GEOS2RTGEOM(gm, FLAGS_GET_Z(rtgeom_in->flags));
	GEOSGeom_destroy(gm);
	if ( ! lines ) {
		rtgeom_free(ep);
		rterror("Error during GEOS2RTGEOM");
		return NULL;
	}

	/*
	 * Reintroduce endpoints from input, using split-line-by-point.
	 * Note that by now we can be sure that each point splits at 
	 * most _one_ segment as any point shared by multiple segments
	 * would already be a node. Also we can be sure that any of
	 * the segments endpoints won't split any other segment.
	 * We can use the above 2 assertions to early exit the loop.
	 */

	col = rtcollection_construct_empty(MULTILINETYPE, rtgeom_in->srid,
	                              FLAGS_GET_Z(rtgeom_in->flags),
	                              FLAGS_GET_M(rtgeom_in->flags));

	np = rtgeom_ngeoms(ep);
	for (pn=0; pn<np; ++pn) { /* for each point */

		const RTPOINT* p = (RTPOINT*)rtgeom_subgeom(ep, pn);

		nl = rtgeom_ngeoms(lines);
		for (ln=0; ln<nl; ++ln) { /* for each line */

			const RTLINE* l = (RTLINE*)rtgeom_subgeom(lines, ln);

			int s = rtline_split_by_point_to(l, p, (RTMLINE*)col);

			if ( ! s ) continue; /* not on this line */

			if ( s == 1 ) {
				/* found on this line, but not splitting it */
				break;
			}

			/* splits this line */

			/* replace this line with the two splits */
			if ( rtgeom_is_collection(lines) ) {
				tc = (RTCOLLECTION*)lines;
				rtcollection_reserve(tc, nl + 1);
				while (nl > ln+1) {
					tc->geoms[nl] = tc->geoms[nl-1];
					--nl;
				}
				rtgeom_free(tc->geoms[ln]);
				tc->geoms[ln]   = col->geoms[0];
				tc->geoms[ln+1] = col->geoms[1];
				tc->ngeoms++;
			} else {
				rtgeom_free(lines);
				/* transfer ownership rather than cloning */
				lines = (RTGEOM*)rtcollection_clone_deep(col);
				assert(col->ngeoms == 2);
				rtgeom_free(col->geoms[0]);
				rtgeom_free(col->geoms[1]);
			}

			/* reset the vector */
			assert(col->ngeoms == 2);
			col->ngeoms = 0;

			break;
		}

	}

	rtgeom_free(ep);
	rtcollection_free(col);

	lines->srid = rtgeom_in->srid;
	return (RTGEOM*)lines;
#endif /* RTGEOM_GEOS_VERSION >= 33 */
}

