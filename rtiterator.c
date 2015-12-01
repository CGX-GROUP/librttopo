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
 * of geometries, and a stack of POINTARRAYs extracted from those geometries.
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
prepend_node(void* g, LISTNODE* front)
{
	LISTNODE* n = rtalloc(sizeof(LISTNODE));
	n->item = g;
	n->next = front;

	return n;
}

static LISTNODE*
pop_node(LISTNODE* i)
{
	LISTNODE* next = i->next;
	rtfree(i);
	return next;
}

static int
add_rtgeom_to_stack(RTPOINTITERATOR* s, RTGEOM* g)
{
	if (rtgeom_is_empty(g))
		return RT_FAILURE;

	s->geoms = prepend_node(g, s->geoms);
	return RT_SUCCESS;
}

/** Return a pointer to the first of one or more LISTNODEs holding the POINTARRAYs
 *  of a geometry.  Will not handle GeometryCollections.
 */
static LISTNODE*
extract_pointarrays_from_rtgeom(RTGEOM* g)
{
	switch(rtgeom_get_type(g))
	{
	case POINTTYPE:
		return prepend_node(rtgeom_as_rtpoint(g)->point, NULL);
	case LINETYPE:
		return prepend_node(rtgeom_as_rtline(g)->points, NULL);
	case TRIANGLETYPE:
		return prepend_node(rtgeom_as_rttriangle(g)->points, NULL);
	case CIRCSTRINGTYPE:
		return prepend_node(rtgeom_as_rtcircstring(g)->points, NULL);
	case POLYGONTYPE:
	{
		LISTNODE* n = NULL;

		RTPOLY* p = rtgeom_as_rtpoly(g);
		int i;
		for (i = p->nrings - 1; i >= 0; i--)
		{
			n = prepend_node(p->rings[i], n);
		}

		return n;
	}
	default:
		rterror("Unsupported geometry type for rtpointiterator");
	}

	return NULL;
}

/** Remove an RTCOLLECTION from the iterator stack, and add the components of the
 *  RTCOLLECTIONs to the stack.
 */
static void
unroll_collection(RTPOINTITERATOR* s)
{
	int i;
	RTCOLLECTION* c;

	if (!s->geoms)
	{
		return;
	}

	c = (RTCOLLECTION*) s->geoms->item;
	s->geoms = pop_node(s->geoms);

	for (i = c->ngeoms - 1; i >= 0; i--)
	{
		RTGEOM* g = rtcollection_getsubgeom(c, i);

		add_rtgeom_to_stack(s, g);
	}
}

/** Unroll RTCOLLECTIONs from the top of the stack, as necessary, until the element at the
 *  top of the stack is not a RTCOLLECTION.
 */
static void
unroll_collections(RTPOINTITERATOR* s)
{
	while(s->geoms && rtgeom_is_collection(s->geoms->item))
	{
		unroll_collection(s);
	}
}

static int
rtpointiterator_advance(RTPOINTITERATOR* s)
{
	s->i += 1;

	/* We've reached the end of our current POINTARRAY.  Try to see if there
	 * are any more POINTARRAYS on the stack. */
	if (s->pointarrays && s->i >= ((POINTARRAY*) s->pointarrays->item)->npoints)
	{
		s->pointarrays = pop_node(s->pointarrays);
		s->i = 0;
	}

	/* We don't have a current POINTARRAY.  Pull a geometry from the stack, and
	 * decompose it into its POINTARRARYs. */
	if (!s->pointarrays)
	{
		RTGEOM* g;
		unroll_collections(s);

		if (!s->geoms)
		{
			return RT_FAILURE;
		}

		s->i = 0;
		g = s->geoms->item;
		s->pointarrays = extract_pointarrays_from_rtgeom(g);

		s->geoms = pop_node(s->geoms);
	}

	if (!s->pointarrays)
	{
		return RT_FAILURE;
	}
	return RT_SUCCESS;
}

/* Public API implementation */

int
rtpointiterator_peek(RTPOINTITERATOR* s, POINT4D* p)
{
	if (!rtpointiterator_has_next(s))
		return RT_FAILURE;

	return getPoint4d_p(s->pointarrays->item, s->i, p);
}

int
rtpointiterator_has_next(RTPOINTITERATOR* s)
{
	if (s->pointarrays && s->i < ((POINTARRAY*) s->pointarrays->item)->npoints)
		return RT_TRUE;
	return RT_FALSE;
}

int
rtpointiterator_next(RTPOINTITERATOR* s, POINT4D* p)
{
	if (!rtpointiterator_has_next(s))
		return RT_FAILURE;

	/* If p is NULL, just advance without reading */
	if (p && !rtpointiterator_peek(s, p))
		return RT_FAILURE;

	rtpointiterator_advance(s);
	return RT_SUCCESS;
}

int
rtpointiterator_modify_next(RTPOINTITERATOR* s, const POINT4D* p)
{
	if (!rtpointiterator_has_next(s))
		return RT_FAILURE;

	if (!s->allow_modification)
	{
		rterror("Cannot write to read-only iterator");
		return RT_FAILURE;
	}

	ptarray_set_point4d(s->pointarrays->item, s->i, p);

	rtpointiterator_advance(s);
	return RT_SUCCESS;
}

RTPOINTITERATOR*
rtpointiterator_create(const RTGEOM* g)
{
	RTPOINTITERATOR* it = rtpointiterator_create_rw((RTGEOM*) g);
	it->allow_modification = RT_FALSE;

	return it;
}

RTPOINTITERATOR*
rtpointiterator_create_rw(RTGEOM* g)
{
	RTPOINTITERATOR* it = rtalloc(sizeof(RTPOINTITERATOR));

	it->geoms = NULL;
	it->pointarrays = NULL;
	it->i = 0;
	it->allow_modification = RT_TRUE;

	add_rtgeom_to_stack(it, g);
	rtpointiterator_advance(it);

	return it;
}

void
rtpointiterator_destroy(RTPOINTITERATOR* s)
{
	while (s->geoms != NULL)
	{
		s->geoms = pop_node(s->geoms);
	}

	while (s->pointarrays != NULL)
	{
		s->pointarrays = pop_node(s->pointarrays);
	}

	rtfree(s);
}
