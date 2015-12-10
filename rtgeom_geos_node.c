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
rtgeom_ngeoms(const RTCTX *ctx, const RTGEOM* n)
{
	const RTCOLLECTION* c = rtgeom_as_rtcollection(ctx, n);
	if ( c ) return c->ngeoms;
	else return 1;
}

static const RTGEOM*
rtgeom_subgeom(const RTCTX *ctx, const RTGEOM* g, int n)
{
	const RTCOLLECTION* c = rtgeom_as_rtcollection(ctx, g);
	if ( c ) return rtcollection_getsubgeom(ctx, (RTCOLLECTION*)c, n);
	else return g;
}


static void
rtgeom_collect_endpoints(const RTCTX *ctx, const RTGEOM* rtg, RTMPOINT* col)
{
	int i, n;
	RTLINE* l;

	switch (rtg->type)
	{
		case RTMULTILINETYPE:
			for ( i = 0,
			        n = rtgeom_ngeoms(ctx, rtg);
			      i < n; ++i )
			{
				rtgeom_collect_endpoints(ctx, 
					rtgeom_subgeom(ctx, rtg, i),
					col);
			}
			break;
		case RTLINETYPE:
			l = (RTLINE*)rtg;
			col = rtmpoint_add_rtpoint(ctx, col,
				rtline_get_rtpoint(ctx, l, 0));
			col = rtmpoint_add_rtpoint(ctx, col,
				rtline_get_rtpoint(ctx, l, l->points->npoints-1));
			break;
		default:
			rterror(ctx, "rtgeom_collect_endpoints: invalid type %s",
				rttype_name(ctx, rtg->type));
			break;
	}
}

static RTMPOINT*
rtgeom_extract_endpoints(const RTCTX *ctx, const RTGEOM* rtg)
{
	RTMPOINT* col = rtmpoint_construct_empty(ctx, SRID_UNKNOWN,
	                              RTFLAGS_GET_Z(rtg->flags),
	                              RTFLAGS_GET_M(rtg->flags));
	rtgeom_collect_endpoints(ctx, rtg, col);

	return col;
}

/* Assumes initGEOS was called already */
/* May return RTPOINT or RTMPOINT */
static RTGEOM*
rtgeom_extract_unique_endpoints(const RTCTX *ctx, const RTGEOM* rtg)
{
#if RTGEOM_GEOS_VERSION < 33
	rterror(ctx, "The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'GEOSUnaryUnion' function (3.3.0+ required)",
	        RTGEOM_GEOS_VERSION);
	return NULL;
#else /* RTGEOM_GEOS_VERSION >= 33 */
	RTGEOM* ret;
	GEOSGeometry *gepu;
	RTMPOINT *epall = rtgeom_extract_endpoints(ctx, rtg);
	GEOSGeometry *gepall = RTGEOM2GEOS(ctx, (RTGEOM*)epall, 1);
	rtmpoint_free(ctx, epall);
	if ( ! gepall ) {
		rterror(ctx, "RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* UnaryUnion to remove duplicates */
	/* TODO: do it all within pgis using indices */
	gepu = GEOSUnaryUnion(gepall);
	if ( ! gepu ) {
		GEOSGeom_destroy(gepall);
		rterror(ctx, "GEOSUnaryUnion: %s", rtgeom_geos_errmsg);
		return NULL;
	}
	GEOSGeom_destroy(gepall);

	ret = GEOS2RTGEOM(ctx, gepu, RTFLAGS_GET_Z(rtg->flags));
	GEOSGeom_destroy(gepu);
	if ( ! ret ) {
		rterror(ctx, "Error during GEOS2RTGEOM");
		return NULL;
	}

	return ret;
#endif /* RTGEOM_GEOS_VERSION >= 33 */
}

/* exported */
extern RTGEOM* rtgeom_node(const RTCTX *ctx, const RTGEOM* rtgeom_in);
RTGEOM*
rtgeom_node(const RTCTX *ctx, const RTGEOM* rtgeom_in)
{
#if RTGEOM_GEOS_VERSION < 33
	rterror(ctx, "The GEOS version this postgis binary "
	        "was compiled against (%d) doesn't support "
	        "'GEOSUnaryUnion' function (3.3.0+ required)",
	        RTGEOM_GEOS_VERSION);
	return NULL;
#else /* RTGEOM_GEOS_VERSION >= 33 */
	GEOSGeometry *g1, *gu, *gm;
	RTGEOM *ep, *lines;
	RTCOLLECTION *col, *tc;
	int pn, ln, np, nl;

	if ( rtgeom_dimension(ctx, rtgeom_in) != 1 ) {
		rterror(ctx, "Noding geometries of dimension != 1 is unsupported");
		return NULL;
	}

	initGEOS(rtgeom_geos_error, rtgeom_geos_error);
	g1 = RTGEOM2GEOS(ctx, rtgeom_in, 1);
	if ( ! g1 ) {
		rterror(ctx, "RTGEOM2GEOS: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	ep = rtgeom_extract_unique_endpoints(ctx, rtgeom_in);
	if ( ! ep ) {
		GEOSGeom_destroy(g1);
		rterror(ctx, "Error extracting unique endpoints from input");
		return NULL;
	}

	/* Unary union input to fully node */
	gu = GEOSUnaryUnion(g1);
	GEOSGeom_destroy(g1);
	if ( ! gu ) {
		rtgeom_free(ctx, ep);
		rterror(ctx, "GEOSUnaryUnion: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	/* Linemerge (in case of overlaps) */
	gm = GEOSLineMerge(gu);
	GEOSGeom_destroy(gu);
	if ( ! gm ) {
		rtgeom_free(ctx, ep);
		rterror(ctx, "GEOSLineMerge: %s", rtgeom_geos_errmsg);
		return NULL;
	}

	lines = GEOS2RTGEOM(ctx, gm, RTFLAGS_GET_Z(rtgeom_in->flags));
	GEOSGeom_destroy(gm);
	if ( ! lines ) {
		rtgeom_free(ctx, ep);
		rterror(ctx, "Error during GEOS2RTGEOM");
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

	col = rtcollection_construct_empty(ctx, RTMULTILINETYPE, rtgeom_in->srid,
	                              RTFLAGS_GET_Z(rtgeom_in->flags),
	                              RTFLAGS_GET_M(rtgeom_in->flags));

	np = rtgeom_ngeoms(ctx, ep);
	for (pn=0; pn<np; ++pn) { /* for each point */

		const RTPOINT* p = (RTPOINT*)rtgeom_subgeom(ctx, ep, pn);

		nl = rtgeom_ngeoms(ctx, lines);
		for (ln=0; ln<nl; ++ln) { /* for each line */

			const RTLINE* l = (RTLINE*)rtgeom_subgeom(ctx, lines, ln);

			int s = rtline_split_by_point_to(ctx, l, p, (RTMLINE*)col);

			if ( ! s ) continue; /* not on this line */

			if ( s == 1 ) {
				/* found on this line, but not splitting it */
				break;
			}

			/* splits this line */

			/* replace this line with the two splits */
			if ( rtgeom_is_collection(ctx, lines) ) {
				tc = (RTCOLLECTION*)lines;
				rtcollection_reserve(ctx, tc, nl + 1);
				while (nl > ln+1) {
					tc->geoms[nl] = tc->geoms[nl-1];
					--nl;
				}
				rtgeom_free(ctx, tc->geoms[ln]);
				tc->geoms[ln]   = col->geoms[0];
				tc->geoms[ln+1] = col->geoms[1];
				tc->ngeoms++;
			} else {
				rtgeom_free(ctx, lines);
				/* transfer ownership rather than cloning */
				lines = (RTGEOM*)rtcollection_clone_deep(ctx, col);
				assert(col->ngeoms == 2);
				rtgeom_free(ctx, col->geoms[0]);
				rtgeom_free(ctx, col->geoms[1]);
			}

			/* reset the vector */
			assert(col->ngeoms == 2);
			col->ngeoms = 0;

			break;
		}

	}

	rtgeom_free(ctx, ep);
	rtcollection_free(ctx, col);

	lines->srid = rtgeom_in->srid;
	return (RTGEOM*)lines;
#endif /* RTGEOM_GEOS_VERSION >= 33 */
}

