/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2001-2003 Refractions Research Inc.
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "rttopo_config.h"
#include "librtgeom_internal.h"
#include "librtgeom.h"
#include "rtgeom_log.h"
#include <string.h>


/** convert decimal degress to radians */
static void
to_rad(POINT4D *pt)
{
	pt->x *= M_PI/180.0;
	pt->y *= M_PI/180.0;
}

/** convert radians to decimal degress */
static void
to_dec(POINT4D *pt)
{
	pt->x *= 180.0/M_PI;
	pt->y *= 180.0/M_PI;
}

/**
 * Transform given POINTARRAY
 * from inpj projection to outpj projection
 */
int
ptarray_transform(POINTARRAY *pa, projPJ inpj, projPJ outpj)
{
  int i;
	POINT4D p;

  for ( i = 0; i < pa->npoints; i++ )
  {
    getPoint4d_p(pa, i, &p);
    if ( ! point4d_transform(&p, inpj, outpj) ) return RT_FAILURE;
    ptarray_set_point4d(pa, i, &p);
  }

	return RT_SUCCESS;
}


/**
 * Transform given SERIALIZED geometry
 * from inpj projection to outpj projection
 */
int
rtgeom_transform(RTGEOM *geom, projPJ inpj, projPJ outpj)
{
	int i;

	/* No points to transform in an empty! */
	if ( rtgeom_is_empty(geom) )
		return RT_SUCCESS;

	switch(geom->type)
	{
		case RTPOINTTYPE:
		case RTLINETYPE:
		case RTCIRCSTRINGTYPE:
		case RTTRIANGLETYPE:
		{
			RTLINE *g = (RTLINE*)geom;
      if ( ! ptarray_transform(g->points, inpj, outpj) ) return RT_FAILURE;
			break;
		}
		case RTPOLYGONTYPE:
		{
			RTPOLY *g = (RTPOLY*)geom;
			for ( i = 0; i < g->nrings; i++ )
			{
        if ( ! ptarray_transform(g->rings[i], inpj, outpj) ) return RT_FAILURE;
			}
			break;
		}
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTCOLLECTIONTYPE:
		case RTCOMPOUNDTYPE:
		case RTCURVEPOLYTYPE:
		case RTMULTICURVETYPE:
		case RTMULTISURFACETYPE:
		case RTPOLYHEDRALSURFACETYPE:
		case RTTINTYPE:
		{
			RTCOLLECTION *g = (RTCOLLECTION*)geom;
			for ( i = 0; i < g->ngeoms; i++ )
			{
				if ( ! rtgeom_transform(g->geoms[i], inpj, outpj) ) return RT_FAILURE;
			}
			break;
		}
		default:
		{
			rterror("rtgeom_transform: Cannot handle type '%s'",
			          rttype_name(geom->type));
			return RT_FAILURE;
		}
	}
	return RT_SUCCESS;
}

int
point4d_transform(POINT4D *pt, projPJ srcpj, projPJ dstpj)
{
	int* pj_errno_ref;
	POINT4D orig_pt;

	/* Make a copy of the input point so we can report the original should an error occur */
	orig_pt.x = pt->x;
	orig_pt.y = pt->y;
	orig_pt.z = pt->z;

	if (pj_is_latlong(srcpj)) to_rad(pt) ;

	RTDEBUGF(4, "transforming POINT(%f %f) from '%s' to '%s'", orig_pt.x, orig_pt.y, pj_get_def(srcpj,0), pj_get_def(dstpj,0));

	/* Perform the transform */
	pj_transform(srcpj, dstpj, 1, 0, &(pt->x), &(pt->y), &(pt->z));

	/* For NAD grid-shift errors, display an error message with an additional hint */
	pj_errno_ref = pj_get_errno_ref();

	if (*pj_errno_ref != 0)
	{
		if (*pj_errno_ref == -38)
		{
			rtnotice("PostGIS was unable to transform the point because either no grid shift files were found, or the point does not lie within the range for which the grid shift is defined. Refer to the ST_Transform() section of the PostGIS manual for details on how to configure PostGIS to alter this behaviour.");
			rterror("transform: couldn't project point (%g %g %g): %s (%d)", 
			        orig_pt.x, orig_pt.y, orig_pt.z, pj_strerrno(*pj_errno_ref), *pj_errno_ref);
			return 0;
		}
		else
		{
			rterror("transform: couldn't project point (%g %g %g): %s (%d)",
			        orig_pt.x, orig_pt.y, orig_pt.z, pj_strerrno(*pj_errno_ref), *pj_errno_ref);
			return 0;
		}
	}

	if (pj_is_latlong(dstpj)) to_dec(pt);
	return 1;
}

projPJ
rtproj_from_string(const char *str1)
{
	int t;
	char *params[1024];  /* one for each parameter */
	char *loc;
	char *str;
	size_t slen;
	projPJ result;


	if (str1 == NULL) return NULL;

	slen = strlen(str1);

	if (slen == 0) return NULL;

	str = rtalloc(slen+1);
	strcpy(str, str1);

	/*
	 * first we split the string into a bunch of smaller strings,
	 * based on the " " separator
	 */

	params[0] = str; /* 1st param, we'll null terminate at the " " soon */

	loc = str;
	t = 1;
	while  ((loc != NULL) && (*loc != 0) )
	{
		loc = strchr(loc, ' ');
		if (loc != NULL)
		{
			*loc = 0; /* null terminate */
			params[t] = loc+1;
			loc++; /* next char */
			t++; /*next param */
		}
	}

	if (!(result=pj_init(t, params)))
	{
		rtfree(str);
		return NULL;
	}
	rtfree(str);
	return result;
}



