/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2001-2006 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "librtgeom_internal.h"
#include "rtgeom_log.h"


RTPSURFACE* rtpsurface_add_rtpoly(RTPSURFACE *mobj, const RTPOLY *obj)
{
	return (RTPSURFACE*)rtcollection_add_rtgeom((RTCOLLECTION*)mobj, (RTGEOM*)obj);
}


void rtpsurface_free(RTPSURFACE *psurf)
{
	int i;
	if ( ! psurf ) return;
	if ( psurf->bbox )
		rtfree(psurf->bbox);

	for ( i = 0; i < psurf->ngeoms; i++ )
		if ( psurf->geoms && psurf->geoms[i] )
			rtpoly_free(psurf->geoms[i]);

	if ( psurf->geoms )
		rtfree(psurf->geoms);

	rtfree(psurf);
}


void printRTPSURFACE(RTPSURFACE *psurf)
{
	int i, j;
	RTPOLY *patch;

	if (psurf->type != POLYHEDRALSURFACETYPE)
		rterror("printRTPSURFACE called with something else than a POLYHEDRALSURFACE");

	rtnotice("RTPSURFACE {");
	rtnotice("    ndims = %i", (int)FLAGS_NDIMS(psurf->flags));
	rtnotice("    SRID = %i", (int)psurf->srid);
	rtnotice("    ngeoms = %i", (int)psurf->ngeoms);

	for (i=0; i<psurf->ngeoms; i++)
	{
		patch = (RTPOLY *) psurf->geoms[i];
		for (j=0; j<patch->nrings; j++)
		{
			rtnotice("    RING # %i :",j);
			printPA(patch->rings[j]);
		}
	}
	rtnotice("}");
}




/*
 * TODO rewrite all this stuff to be based on a truly topological model
 */

struct struct_psurface_arcs
{
	double ax, ay, az;
	double bx, by, bz;
	int cnt, face;
};
typedef struct struct_psurface_arcs *psurface_arcs;

/* We supposed that the geometry is valid
   we could have wrong result if not */
int rtpsurface_is_closed(const RTPSURFACE *psurface)
{
	int i, j, k;
	int narcs, carc;
	int found;
	psurface_arcs arcs;
	POINT4D pa, pb;
	RTPOLY *patch;

	/* If surface is not 3D, it's can't be closed */
	if (!FLAGS_GET_Z(psurface->flags)) return 0;

	/* If surface is less than 4 faces hard to be closed too */
	if (psurface->ngeoms < 4) return 0;

	/* Max theorical arcs number if no one is shared ... */
	for (i=0, narcs=0 ; i < psurface->ngeoms ; i++)
	{
		patch = (RTPOLY *) psurface->geoms[i];
		narcs += patch->rings[0]->npoints - 1;
	}

	arcs = rtalloc(sizeof(struct struct_psurface_arcs) * narcs);
	for (i=0, carc=0; i < psurface->ngeoms ; i++)
	{

		patch = (RTPOLY *) psurface->geoms[i];
		for (j=0; j < patch->rings[0]->npoints - 1; j++)
		{

			getPoint4d_p(patch->rings[0], j,   &pa);
			getPoint4d_p(patch->rings[0], j+1, &pb);

			/* remove redundant points if any */
			if (pa.x == pb.x && pa.y == pb.y && pa.z == pb.z) continue;

			/* Make sure to order the 'lower' point first */
			if ( (pa.x > pb.x) ||
			        (pa.x == pb.x && pa.y > pb.y) ||
			        (pa.x == pb.x && pa.y == pb.y && pa.z > pb.z) )
			{
				pa = pb;
				getPoint4d_p(patch->rings[0], j, &pb);
			}

			for (found=0, k=0; k < carc ; k++)
			{

				if (  ( arcs[k].ax == pa.x && arcs[k].ay == pa.y &&
				        arcs[k].az == pa.z && arcs[k].bx == pb.x &&
				        arcs[k].by == pb.y && arcs[k].bz == pb.z &&
				        arcs[k].face != i) )
				{
					arcs[k].cnt++;
					found = 1;

					/* Look like an invalid PolyhedralSurface
					      anyway not a closed one */
					if (arcs[k].cnt > 2)
					{
						rtfree(arcs);
						return 0;
					}
				}
			}

			if (!found)
			{
				arcs[carc].cnt=1;
				arcs[carc].face=i;
				arcs[carc].ax = pa.x;
				arcs[carc].ay = pa.y;
				arcs[carc].az = pa.z;
				arcs[carc].bx = pb.x;
				arcs[carc].by = pb.y;
				arcs[carc].bz = pb.z;
				carc++;

				/* Look like an invalid PolyhedralSurface
				      anyway not a closed one */
				if (carc > narcs)
				{
					rtfree(arcs);
					return 0;
				}
			}
		}
	}

	/* A polyhedron is closed if each edge
	       is shared by exactly 2 faces */
	for (k=0; k < carc ; k++)
	{
		if (arcs[k].cnt != 2)
		{
			rtfree(arcs);
			return 0;
		}
	}
	rtfree(arcs);

	/* Invalid Polyhedral case */
	if (carc < psurface->ngeoms) return 0;

	return 1;
}
