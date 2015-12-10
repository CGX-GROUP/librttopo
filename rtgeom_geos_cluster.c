/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright 2015 Daniel Baston <dbaston@gmail.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <string.h>
#include "librtgeom.h"
#include "librtgeom_internal.h"
#include "rtgeom_log.h"
#include "rtgeom_geos.h"
#include "rtunionfind.h"

/* Utility struct used to pass information to the GEOSSTRtree_query callback */
struct UnionIfIntersectingContext
{
	UNIONFIND* uf;
	char error;
	uint32_t* p;
	const GEOSPreparedGeometry* prep;
	GEOSGeometry** geoms;
};

/* Utility struct used to pass information to the GEOSSTRtree_query callback */
struct UnionIfDWithinContext
{
	UNIONFIND* uf;
	char error;
	uint32_t* p;
	RTGEOM** geoms;
	double tolerance;
};

/* Utility struct to keep GEOSSTRtree and associated structures to be freed after use */
struct STRTree
{
	GEOSSTRtree* tree;
	GEOSGeometry** envelopes;
	uint32_t* geom_ids;
	uint32_t num_geoms;
};

static struct STRTree make_strtree(RTCTX *ctx, void** geoms, uint32_t num_geoms, char is_rtgeom);
static void destroy_strtree(RTCTX *ctx, struct STRTree tree);
static void union_if_intersecting(RTCTX *ctx, void* item, void* userdata);
static void union_if_dwithin(RTCTX *ctx, void* item, void* userdata);
static int union_intersecting_pairs(RTCTX *ctx, GEOSGeometry** geoms, uint32_t num_geoms, UNIONFIND* uf);
static int combine_geometries(RTCTX *ctx, UNIONFIND* uf, void** geoms, uint32_t num_geoms, void*** clustersGeoms, uint32_t* num_clusters, char is_rtgeom);

/** Make a GEOSSTRtree of either GEOSGeometry* or RTGEOM* pointers */
static struct STRTree
make_strtree(RTCTX *ctx, void** geoms, uint32_t num_geoms, char is_rtgeom)
{
	struct STRTree tree;
	tree.tree = GEOSSTRtree_create(num_geoms);
	if (tree.tree == NULL)
	{
		return tree;
	}
	tree.envelopes = rtalloc(ctx, num_geoms * sizeof(GEOSGeometry*));
	tree.geom_ids  = rtalloc(ctx, num_geoms * sizeof(uint32_t));
	tree.num_geoms = num_geoms;

	size_t i;
	for (i = 0; i < num_geoms; i++)
	{
		tree.geom_ids[i] = i;
		if (!is_rtgeom)
		{
			tree.envelopes[i] = GEOSEnvelope(geoms[i]);
		}
		else
		{
            const RTGBOX* box = rtgeom_get_bbox(ctx, geoms[i]);
            if (box)
            {
                tree.envelopes[i] = GBOX2GEOS(ctx, box);
            } 
            else
            {
                /* Empty geometry */
                tree.envelopes[i] = GEOSGeom_createEmptyPolygon();
            }
		}
		GEOSSTRtree_insert(tree.tree, tree.envelopes[i], &(tree.geom_ids[i]));
	}
	return tree;
}

/** Clean up STRTree after use */
static void
destroy_strtree(RTCTX *ctx, struct STRTree tree)
{
	size_t i;
	GEOSSTRtree_destroy(tree.tree);

	for (i = 0; i < tree.num_geoms; i++)
	{
		GEOSGeom_destroy(tree.envelopes[i]);
	}
	rtfree(ctx, tree.geom_ids);
	rtfree(ctx, tree.envelopes);
}

/* Callback function for GEOSSTRtree_query */
static void
union_if_intersecting(RTCTX *ctx, void* item, void* userdata)
{
	struct UnionIfIntersectingContext *cxt = userdata;
	if (cxt->error)
	{
		return;
	}
	uint32_t q = *((uint32_t*) item);
	uint32_t p = *(cxt->p);

	if (p != q && UF_find(ctx, cxt->uf, p) != UF_find(ctx, cxt->uf, q))
	{
		/* Lazy initialize prepared geometry */
		if (cxt->prep == NULL)
		{
			cxt->prep = GEOSPrepare(cxt->geoms[p]);
		}
		int geos_result = GEOSPreparedIntersects(cxt->prep, cxt->geoms[q]);
		if (geos_result > 1)
		{
			cxt->error = geos_result;
			return;
		}
		if (geos_result)
		{
			UF_union(ctx, cxt->uf, p, q);
		}
	}
}

/* Callback function for GEOSSTRtree_query */
static void
union_if_dwithin(RTCTX *ctx, void* item, void* userdata)
{
	struct UnionIfDWithinContext *cxt = userdata;
	if (cxt->error)
	{
		return;
	}
	uint32_t q = *((uint32_t*) item);
	uint32_t p = *(cxt->p);

	if (p != q && UF_find(ctx, cxt->uf, p) != UF_find(ctx, cxt->uf, q))
	{
		double mindist = rtgeom_mindistance2d_tolerance(ctx, cxt->geoms[p], cxt->geoms[q], cxt->tolerance);
		if (mindist == FLT_MAX)
		{
			cxt->error = 1;
			return;
		}

		if (mindist <= cxt->tolerance)
		{
			UF_union(ctx, cxt->uf, p, q);
		}
	}
}

/* Identify intersecting geometries and mark them as being in the same set */
static int
union_intersecting_pairs(RTCTX *ctx, GEOSGeometry** geoms, uint32_t num_geoms, UNIONFIND* uf)
{
	uint32_t i;

	if (num_geoms <= 1)
	{
		return RT_SUCCESS;
	}

	struct STRTree tree = make_strtree(ctx, (void**) geoms, num_geoms, 0);
	if (tree.tree == NULL)
	{
		destroy_strtree(ctx, tree);
		return RT_FAILURE;
	}
	for (i = 0; i < num_geoms; i++)
	{
        if (GEOSisEmpty(geoms[i]))
        {
            continue;
        }

		struct UnionIfIntersectingContext cxt =
		{
			.uf = uf,
			.error = 0,
			.p = &i,
			.prep = NULL,
			.geoms = geoms
		};
		GEOSGeometry* query_envelope = GEOSEnvelope(geoms[i]);
		GEOSSTRtree_query(tree.tree, query_envelope, &union_if_intersecting, &cxt);

		GEOSGeom_destroy(query_envelope);
		GEOSPreparedGeom_destroy(cxt.prep);
		if (cxt.error)
		{
			return RT_FAILURE;
		}
	}

	destroy_strtree(ctx, tree);
	return RT_SUCCESS;
}

/* Identify geometries within a distance tolerance and mark them as being in the same set */
static int
union_pairs_within_distance(RTCTX *ctx, RTGEOM** geoms, uint32_t num_geoms, UNIONFIND* uf, double tolerance)
{
	uint32_t i;

	if (num_geoms <= 1)
	{
		return RT_SUCCESS;
	}

	struct STRTree tree = make_strtree(ctx, (void**) geoms, num_geoms, 1);
	if (tree.tree == NULL)
	{
		destroy_strtree(ctx, tree);
		return RT_FAILURE;
	}

	for (i = 0; i < num_geoms; i++)
	{
		struct UnionIfDWithinContext cxt =
		{
			.uf = uf,
			.error = 0,
			.p = &i,
			.geoms = geoms,
			.tolerance = tolerance
		};

        const RTGBOX* geom_extent = rtgeom_get_bbox(ctx, geoms[i]);
        if (!geom_extent)
        {
            /* Empty geometry */
            continue;
        }
		RTGBOX* query_extent = gbox_clone(ctx, geom_extent);
		gbox_expand(ctx, query_extent, tolerance);
		GEOSGeometry* query_envelope = GBOX2GEOS(ctx, query_extent);

		if (!query_envelope)
		{
			destroy_strtree(ctx, tree);
			return RT_FAILURE;
		}

		GEOSSTRtree_query(tree.tree, query_envelope, &union_if_dwithin, &cxt);

		rtfree(ctx, query_extent);
		GEOSGeom_destroy(query_envelope);
		if (cxt.error)
		{
			return RT_FAILURE;
		}
	}

	destroy_strtree(ctx, tree);
	return RT_SUCCESS;
}

/** Takes an array of GEOSGeometry* and constructs an array of GEOSGeometry*, where each element in the constructed
 *  array is a GeometryCollection representing a set of interconnected geometries. Caller is responsible for
 *  freeing the input array, but not for destroying the GEOSGeometry* items inside it.  */
int
cluster_intersecting(RTCTX *ctx, GEOSGeometry** geoms, uint32_t num_geoms, GEOSGeometry*** clusterGeoms, uint32_t* num_clusters)
{
	int cluster_success;
	UNIONFIND* uf = UF_create(ctx, num_geoms);

	if (union_intersecting_pairs(ctx, geoms, num_geoms, uf) == RT_FAILURE)
	{
		UF_destroy(ctx, uf);
		return RT_FAILURE;
	}

	cluster_success = combine_geometries(ctx, uf, (void**) geoms, num_geoms, (void***) clusterGeoms, num_clusters, 0);
	UF_destroy(ctx, uf);
	return cluster_success;
}

/** Takes an array of RTGEOM* and constructs an array of RTGEOM*, where each element in the constructed array is a
 *  GeometryCollection representing a set of geometries separated by no more than the specified tolerance. Caller is
 *  responsible for freeing the input array, but not the RTGEOM* items inside it. */
int
cluster_within_distance(RTCTX *ctx, RTGEOM** geoms, uint32_t num_geoms, double tolerance, RTGEOM*** clusterGeoms, uint32_t* num_clusters)
{
	int cluster_success;
	UNIONFIND* uf = UF_create(ctx, num_geoms);

	if (union_pairs_within_distance(ctx, geoms, num_geoms, uf, tolerance) == RT_FAILURE)
	{
		UF_destroy(ctx, uf);
		return RT_FAILURE;
	}

	cluster_success = combine_geometries(ctx, uf, (void**) geoms, num_geoms, (void***) clusterGeoms, num_clusters, 1);
	UF_destroy(ctx, uf);
	return cluster_success;
}

/** Uses a UNIONFIND to identify the set with which each input geometry is associated, and groups the geometries into
 *  GeometryCollections.  Supplied geometry array may be of either RTGEOM* or GEOSGeometry*; is_rtgeom is used to
 *  identify which. Caller is responsible for freeing input geometry array but not the items contained within it. */
static int
combine_geometries(RTCTX *ctx, UNIONFIND* uf, void** geoms, uint32_t num_geoms, void*** clusterGeoms, uint32_t* num_clusters, char is_rtgeom)
{
	size_t i, j, k;

	/* Combine components of each cluster into their own GeometryCollection */
	*num_clusters = uf->num_clusters;
	*clusterGeoms = rtalloc(ctx, *num_clusters * sizeof(void*));

	void** geoms_in_cluster = rtalloc(ctx, num_geoms * sizeof(void*));
	uint32_t* ordered_components = UF_ordered_by_cluster(ctx, uf);
	for (i = 0, j = 0, k = 0; i < num_geoms; i++)
	{
		geoms_in_cluster[j++] = geoms[ordered_components[i]];

		/* Is this the last geometry in the component? */
		if ((i == num_geoms - 1) ||
		        (UF_find(ctx, uf, ordered_components[i]) != UF_find(ctx, uf, ordered_components[i+1])))
		{
			if (is_rtgeom)
			{
				RTGEOM** components = rtalloc(ctx, num_geoms * sizeof(RTGEOM*));
				memcpy(components, geoms_in_cluster, num_geoms * sizeof(RTGEOM*));
				(*clusterGeoms)[k++] = rtcollection_construct(ctx, RTCOLLECTIONTYPE, components[0]->srid, NULL, j, (RTGEOM**) components);
			}
			else
			{
				(*clusterGeoms)[k++] = GEOSGeom_createCollection(GEOS_GEOMETRYCOLLECTION, (GEOSGeometry**) geoms_in_cluster, j);
			}
			j = 0;
		}
	}

	rtfree(ctx, geoms_in_cluster);
	rtfree(ctx, ordered_components);

	return RT_SUCCESS;
}
