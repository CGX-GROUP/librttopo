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

#include "librtgeom.h"
#include "rtgeom_log.h"

struct LISTNODE
{
	struct LISTNODE* next;
	void* item;
};
typedef struct LISTNODE LISTNODE;

/* The RTPOINTITERATOR consists of two stacks of items to process: a stack
 * of geometries, and a stack of RTPOINTARRAYs extracted from those geometries.
 * The index "i" refers to the "next" point, which is found at the top of the
 * pointarrays stack.
 *
 * When the pointarrays stack is depleted, we pull a geometry from the geometry
 * stack to replenish it.
 */
struct RTPOINTITERATOR
{
	LISTNODE* geoms;
	LISTNODE* pointarrays;
	uint32_t i;
	char allow_modification;
};

static LISTNODE*
prepend_node(RTCTX *ctx, void* g, LISTNODE* front)
{
	LISTNODE* n = rtalloc(ctx, sizeof(LISTNODE));
	n->item = g;
	n->next = front;

	return n;
}

static LISTNODE*
pop_node(RTCTX *ctx, LISTNODE* i)
{
	LISTNODE* next = i->next;
	rtfree(ctx, i);
	return next;
}

static int
add_rtgeom_to_stack(RTCTX *ctx, RTPOINTITERATOR* s, RTGEOM* g)
{
	if (rtgeom_is_empty(ctx, g))
		return RT_FAILURE;

	s->geoms = prepend_node(ctx, g, s->geoms);
	return RT_SUCCESS;
}

/** Return a pointer to the first of one or more LISTNODEs holding the RTPOINTARRAYs
 *  of a geometry.  Will not handle GeometryCollections.
 */
static LISTNODE*
extract_pointarrays_from_rtgeom(RTCTX *ctx, RTGEOM* g)
{
	switch(rtgeom_get_type(ctx, g))
	{
	case RTPOINTTYPE:
		return prepend_node(ctx, rtgeom_as_rtpoint(ctx, g)->point, NULL);
	case RTLINETYPE:
		return prepend_node(ctx, rtgeom_as_rtline(ctx, g)->points, NULL);
	case RTTRIANGLETYPE:
		return prepend_node(ctx, rtgeom_as_rttriangle(ctx, g)->points, NULL);
	case RTCIRCSTRINGTYPE:
		return prepend_node(ctx, rtgeom_as_rtcircstring(ctx, g)->points, NULL);
	case RTPOLYGONTYPE:
	{
		LISTNODE* n = NULL;

		RTPOLY* p = rtgeom_as_rtpoly(ctx, g);
		int i;
		for (i = p->nrings - 1; i >= 0; i--)
		{
			n = prepend_node(ctx, p->rings[i], n);
		}

		return n;
	}
	default:
		rterror(ctx, "Unsupported geometry type for rtpointiterator");
	}

	return NULL;
}

/** Remove an RTCOLLECTION from the iterator stack, and add the components of the
 *  RTCOLLECTIONs to the stack.
 */
static void
unroll_collection(RTCTX *ctx, RTPOINTITERATOR* s)
{
	int i;
	RTCOLLECTION* c;

	if (!s->geoms)
	{
		return;
	}

	c = (RTCOLLECTION*) s->geoms->item;
	s->geoms = pop_node(ctx, s->geoms);

	for (i = c->ngeoms - 1; i >= 0; i--)
	{
		RTGEOM* g = rtcollection_getsubgeom(ctx, c, i);

		add_rtgeom_to_stack(ctx, s, g);
	}
}

/** Unroll RTCOLLECTIONs from the top of the stack, as necessary, until the element at the
 *  top of the stack is not a RTCOLLECTION.
 */
static void
unroll_collections(RTCTX *ctx, RTPOINTITERATOR* s)
{
	while(s->geoms && rtgeom_is_collection(ctx, s->geoms->item))
	{
		unroll_collection(ctx, s);
	}
}

static int
rtpointiterator_advance(RTCTX *ctx, RTPOINTITERATOR* s)
{
	s->i += 1;

	/* We've reached the end of our current RTPOINTARRAY.  Try to see if there
	 * are any more RTPOINTARRAYS on the stack. */
	if (s->pointarrays && s->i >= ((RTPOINTARRAY*) s->pointarrays->item)->npoints)
	{
		s->pointarrays = pop_node(ctx, s->pointarrays);
		s->i = 0;
	}

	/* We don't have a current RTPOINTARRAY.  Pull a geometry from the stack, and
	 * decompose it into its POINTARRARYs. */
	if (!s->pointarrays)
	{
		RTGEOM* g;
		unroll_collections(ctx, s);

		if (!s->geoms)
		{
			return RT_FAILURE;
		}

		s->i = 0;
		g = s->geoms->item;
		s->pointarrays = extract_pointarrays_from_rtgeom(ctx, g);

		s->geoms = pop_node(ctx, s->geoms);
	}

	if (!s->pointarrays)
	{
		return RT_FAILURE;
	}
	return RT_SUCCESS;
}

/* Public API implementation */

int
rtpointiterator_peek(RTCTX *ctx, RTPOINTITERATOR* s, RTPOINT4D* p)
{
	if (!rtpointiterator_has_next(ctx, s))
		return RT_FAILURE;

	return getPoint4d_p(ctx, s->pointarrays->item, s->i, p);
}

int
rtpointiterator_has_next(RTCTX *ctx, RTPOINTITERATOR* s)
{
	if (s->pointarrays && s->i < ((RTPOINTARRAY*) s->pointarrays->item)->npoints)
		return RT_TRUE;
	return RT_FALSE;
}

int
rtpointiterator_next(RTCTX *ctx, RTPOINTITERATOR* s, RTPOINT4D* p)
{
	if (!rtpointiterator_has_next(ctx, s))
		return RT_FAILURE;

	/* If p is NULL, just advance without reading */
	if (p && !rtpointiterator_peek(ctx, s, p))
		return RT_FAILURE;

	rtpointiterator_advance(ctx, s);
	return RT_SUCCESS;
}

int
rtpointiterator_modify_next(RTCTX *ctx, RTPOINTITERATOR* s, const RTPOINT4D* p)
{
	if (!rtpointiterator_has_next(ctx, s))
		return RT_FAILURE;

	if (!s->allow_modification)
	{
		rterror(ctx, "Cannot write to read-only iterator");
		return RT_FAILURE;
	}

	ptarray_set_point4d(ctx, s->pointarrays->item, s->i, p);

	rtpointiterator_advance(ctx, s);
	return RT_SUCCESS;
}

RTPOINTITERATOR*
rtpointiterator_create(RTCTX *ctx, const RTGEOM* g)
{
	RTPOINTITERATOR* it = rtpointiterator_create_rw(ctx, (RTGEOM*) g);
	it->allow_modification = RT_FALSE;

	return it;
}

RTPOINTITERATOR*
rtpointiterator_create_rw(RTCTX *ctx, RTGEOM* g)
{
	RTPOINTITERATOR* it = rtalloc(ctx, sizeof(RTPOINTITERATOR));

	it->geoms = NULL;
	it->pointarrays = NULL;
	it->i = 0;
	it->allow_modification = RT_TRUE;

	add_rtgeom_to_stack(ctx, it, g);
	rtpointiterator_advance(ctx, it);

	return it;
}

void
rtpointiterator_destroy(RTCTX *ctx, RTPOINTITERATOR* s)
{
	while (s->geoms != NULL)
	{
		s->geoms = pop_node(ctx, s->geoms);
	}

	while (s->pointarrays != NULL)
	{
		s->pointarrays = pop_node(ctx, s->pointarrays);
	}

	rtfree(ctx, s);
}
