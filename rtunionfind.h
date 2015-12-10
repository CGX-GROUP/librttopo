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

#ifndef _RTUNIONFIND
#define _RTUNIONFIND 1

#include "librtgeom.h"

typedef struct
{
	uint32_t* clusters;
	uint32_t* cluster_sizes;
	uint32_t num_clusters;
	uint32_t N;
} UNIONFIND;

/* Allocate a UNIONFIND structure of capacity N */
UNIONFIND* UF_create(RTCTX *ctx, uint32_t N);

/* Release memory associated with UNIONFIND structure */
void UF_destroy(RTCTX *ctx, UNIONFIND* uf);

/* Identify the cluster id associated with specified component id */
uint32_t UF_find(RTCTX *ctx, UNIONFIND* uf, uint32_t i);

/* Merge the clusters that contain the two specified components ids */
void UF_union(RTCTX *ctx, UNIONFIND* uf, uint32_t i, uint32_t j);

/* Return an array of component ids, where components that are in the
 * same cluster are contiguous in the array */
uint32_t* UF_ordered_by_cluster(RTCTX *ctx, UNIONFIND* uf);

#endif
