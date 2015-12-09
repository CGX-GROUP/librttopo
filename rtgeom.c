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
#include <stdarg.h>

#include "librtgeom_internal.h"
#include "rtgeom_log.h"


/** Force Right-hand-rule on RTGEOM polygons **/
void
rtgeom_force_clockwise(RTGEOM *rtgeom)
{
	RTCOLLECTION *coll;
	int i;

	switch (rtgeom->type)
	{
	case RTPOLYGONTYPE:
		rtpoly_force_clockwise((RTPOLY *)rtgeom);
		return;

	case RTTRIANGLETYPE:
		rttriangle_force_clockwise((RTTRIANGLE *)rtgeom);
		return;

		/* Not handle POLYHEDRALSURFACE and TIN
		   as they are supposed to be well oriented */
	case RTMULTIPOLYGONTYPE:
	case RTCOLLECTIONTYPE:
		coll = (RTCOLLECTION *)rtgeom;
		for (i=0; i<coll->ngeoms; i++)
			rtgeom_force_clockwise(coll->geoms[i]);
		return;
	}
}

/** Reverse vertex order of RTGEOM **/
void
rtgeom_reverse(RTGEOM *rtgeom)
{
	int i;
	RTCOLLECTION *col;

	switch (rtgeom->type)
	{
	case RTLINETYPE:
		rtline_reverse((RTLINE *)rtgeom);
		return;
	case RTPOLYGONTYPE:
		rtpoly_reverse((RTPOLY *)rtgeom);
		return;
	case RTTRIANGLETYPE:
		rttriangle_reverse((RTTRIANGLE *)rtgeom);
		return;
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
		col = (RTCOLLECTION *)rtgeom;
		for (i=0; i<col->ngeoms; i++)
			rtgeom_reverse(col->geoms[i]);
		return;
	}
}

RTPOINT *
rtgeom_as_rtpoint(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTPOINTTYPE )
		return (RTPOINT *)rtgeom;
	else return NULL;
}

RTLINE *
rtgeom_as_rtline(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTLINETYPE )
		return (RTLINE *)rtgeom;
	else return NULL;
}

RTCIRCSTRING *
rtgeom_as_rtcircstring(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTCIRCSTRINGTYPE )
		return (RTCIRCSTRING *)rtgeom;
	else return NULL;
}

RTCOMPOUND *
rtgeom_as_rtcompound(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTCOMPOUNDTYPE )
		return (RTCOMPOUND *)rtgeom;
	else return NULL;
}

RTCURVEPOLY *
rtgeom_as_rtcurvepoly(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTCURVEPOLYTYPE )
		return (RTCURVEPOLY *)rtgeom;
	else return NULL;
}

RTPOLY *
rtgeom_as_rtpoly(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTPOLYGONTYPE )
		return (RTPOLY *)rtgeom;
	else return NULL;
}

RTTRIANGLE *
rtgeom_as_rttriangle(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTTRIANGLETYPE )
		return (RTTRIANGLE *)rtgeom;
	else return NULL;
}

RTCOLLECTION *
rtgeom_as_rtcollection(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom_is_collection(rtgeom) )
		return (RTCOLLECTION*)rtgeom;
	else return NULL;
}

RTMPOINT *
rtgeom_as_rtmpoint(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTMULTIPOINTTYPE )
		return (RTMPOINT *)rtgeom;
	else return NULL;
}

RTMLINE *
rtgeom_as_rtmline(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTMULTILINETYPE )
		return (RTMLINE *)rtgeom;
	else return NULL;
}

RTMPOLY *
rtgeom_as_rtmpoly(const RTGEOM *rtgeom)
{
	if ( rtgeom == NULL ) return NULL;
	if ( rtgeom->type == RTMULTIPOLYGONTYPE )
		return (RTMPOLY *)rtgeom;
	else return NULL;
}

RTPSURFACE *
rtgeom_as_rtpsurface(const RTGEOM *rtgeom)
{
	if ( rtgeom->type == RTPOLYHEDRALSURFACETYPE )
		return (RTPSURFACE *)rtgeom;
	else return NULL;
}

RTTIN *
rtgeom_as_rttin(const RTGEOM *rtgeom)
{
	if ( rtgeom->type == RTTINTYPE )
		return (RTTIN *)rtgeom;
	else return NULL;
}

RTGEOM *rttin_as_rtgeom(const RTTIN *obj)
{
	return (RTGEOM *)obj;
}

RTGEOM *rtpsurface_as_rtgeom(const RTPSURFACE *obj)
{
	return (RTGEOM *)obj;
}

RTGEOM *rtmpoly_as_rtgeom(const RTMPOLY *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtmline_as_rtgeom(const RTMLINE *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtmpoint_as_rtgeom(const RTMPOINT *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtcollection_as_rtgeom(const RTCOLLECTION *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtcircstring_as_rtgeom(const RTCIRCSTRING *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtcurvepoly_as_rtgeom(const RTCURVEPOLY *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtcompound_as_rtgeom(const RTCOMPOUND *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtpoly_as_rtgeom(const RTPOLY *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rttriangle_as_rtgeom(const RTTRIANGLE *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtline_as_rtgeom(const RTLINE *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}
RTGEOM *rtpoint_as_rtgeom(const RTPOINT *obj)
{
	if ( obj == NULL ) return NULL;
	return (RTGEOM *)obj;
}


/**
** Look-up for the correct MULTI* type promotion for singleton types.
*/
uint8_t RTMULTITYPE[RTNUMTYPES] =
{
	0,
	RTMULTIPOINTTYPE,        /*  1 */
	RTMULTILINETYPE,         /*  2 */
	RTMULTIPOLYGONTYPE,      /*  3 */
	0,0,0,0,
	RTMULTICURVETYPE,        /*  8 */
	RTMULTICURVETYPE,        /*  9 */
	RTMULTISURFACETYPE,      /* 10 */
	RTPOLYHEDRALSURFACETYPE, /* 11 */
	0, 0,
	RTTINTYPE,               /* 14 */
	0
};

/**
* Create a new RTGEOM of the appropriate MULTI* type.
*/
RTGEOM *
rtgeom_as_multi(const RTGEOM *rtgeom)
{
	RTGEOM **ogeoms;
	RTGEOM *ogeom = NULL;
	RTGBOX *box = NULL;
	int type;

	type = rtgeom->type;

	if ( ! RTMULTITYPE[type] ) return rtgeom_clone(rtgeom);

	if( rtgeom_is_empty(rtgeom) )
	{
		ogeom = (RTGEOM *)rtcollection_construct_empty(
			RTMULTITYPE[type],
			rtgeom->srid,
			FLAGS_GET_Z(rtgeom->flags),
			FLAGS_GET_M(rtgeom->flags)
		);
	}
	else
	{
		ogeoms = rtalloc(sizeof(RTGEOM*));
		ogeoms[0] = rtgeom_clone(rtgeom);

		/* Sub-geometries are not allowed to have bboxes or SRIDs, move the bbox to the collection */
		box = ogeoms[0]->bbox;
		ogeoms[0]->bbox = NULL;
		ogeoms[0]->srid = SRID_UNKNOWN;

		ogeom = (RTGEOM *)rtcollection_construct(RTMULTITYPE[type], rtgeom->srid, box, 1, ogeoms);
	}

	return ogeom;
}

/**
* Create a new RTGEOM of the appropriate CURVE* type.
*/
RTGEOM *
rtgeom_as_curve(const RTGEOM *rtgeom)
{
	RTGEOM *ogeom;
	int type = rtgeom->type;
	/*
	int hasz = FLAGS_GET_Z(rtgeom->flags);
	int hasm = FLAGS_GET_M(rtgeom->flags);
	int srid = rtgeom->srid;
	*/

	switch(type)
	{
		case RTLINETYPE:
			/* turn to COMPOUNDCURVE */
			ogeom = (RTGEOM*)rtcompound_construct_from_rtline((RTLINE*)rtgeom);
			break;
		case RTPOLYGONTYPE:
			ogeom = (RTGEOM*)rtcurvepoly_construct_from_rtpoly(rtgeom_as_rtpoly(rtgeom));
			break;
		case RTMULTILINETYPE:
			/* turn to MULTICURVE */
			ogeom = rtgeom_clone(rtgeom);
			ogeom->type = RTMULTICURVETYPE;
			break;
		case RTMULTIPOLYGONTYPE:
			/* turn to MULTISURFACE */
			ogeom = rtgeom_clone(rtgeom);
			ogeom->type = RTMULTISURFACETYPE;
			break;
		case RTCOLLECTIONTYPE:
		default:
			ogeom = rtgeom_clone(rtgeom);
			break;
	}

	/* TODO: copy bbox from input geom ? */

	return ogeom;
}


/**
* Free the containing RTGEOM and the associated BOX. Leave the underlying 
* geoms/points/point objects intact. Useful for functions that are stripping
* out subcomponents of complex objects, or building up new temporary objects
* on top of subcomponents.
*/
void
rtgeom_release(RTGEOM *rtgeom)
{
	if ( ! rtgeom )
		rterror("rtgeom_release: someone called on 0x0");

	RTDEBUGF(3, "releasing type %s", rttype_name(rtgeom->type));

	/* Drop bounding box (artays a copy) */
	if ( rtgeom->bbox )
	{
		RTDEBUGF(3, "rtgeom_release: releasing bbox. %p", rtgeom->bbox);
		rtfree(rtgeom->bbox);
	}
	rtfree(rtgeom);

}


/* @brief Clone RTGEOM object. Serialized point lists are not copied.
 *
 * @see ptarray_clone 
 */
RTGEOM *
rtgeom_clone(const RTGEOM *rtgeom)
{
	RTDEBUGF(2, "rtgeom_clone called with %p, %s",
	         rtgeom, rttype_name(rtgeom->type));

	switch (rtgeom->type)
	{
	case RTPOINTTYPE:
		return (RTGEOM *)rtpoint_clone((RTPOINT *)rtgeom);
	case RTLINETYPE:
		return (RTGEOM *)rtline_clone((RTLINE *)rtgeom);
	case RTCIRCSTRINGTYPE:
		return (RTGEOM *)rtcircstring_clone((RTCIRCSTRING *)rtgeom);
	case RTPOLYGONTYPE:
		return (RTGEOM *)rtpoly_clone((RTPOLY *)rtgeom);
	case RTTRIANGLETYPE:
		return (RTGEOM *)rttriangle_clone((RTTRIANGLE *)rtgeom);
	case RTCOMPOUNDTYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTICURVETYPE:
	case RTMULTISURFACETYPE:
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
		return (RTGEOM *)rtcollection_clone((RTCOLLECTION *)rtgeom);
	default:
		rterror("rtgeom_clone: Unknown geometry type: %s", rttype_name(rtgeom->type));
		return NULL;
	}
}

/** 
* Deep-clone an #RTGEOM object. #RTPOINTARRAY <em>are</em> copied. 
*/
RTGEOM *
rtgeom_clone_deep(const RTGEOM *rtgeom)
{
	RTDEBUGF(2, "rtgeom_clone called with %p, %s",
	         rtgeom, rttype_name(rtgeom->type));

	switch (rtgeom->type)
	{
	case RTPOINTTYPE:
	case RTLINETYPE:
	case RTCIRCSTRINGTYPE:
	case RTTRIANGLETYPE:
		return (RTGEOM *)rtline_clone_deep((RTLINE *)rtgeom);
	case RTPOLYGONTYPE:
		return (RTGEOM *)rtpoly_clone_deep((RTPOLY *)rtgeom);
	case RTCOMPOUNDTYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTICURVETYPE:
	case RTMULTISURFACETYPE:
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
		return (RTGEOM *)rtcollection_clone_deep((RTCOLLECTION *)rtgeom);
	default:
		rterror("rtgeom_clone_deep: Unknown geometry type: %s", rttype_name(rtgeom->type));
		return NULL;
	}
}


/**
 * Return an alloced string
 */
char*
rtgeom_to_ewkt(const RTGEOM *rtgeom)
{
	char* wkt = NULL;
	size_t wkt_size = 0;
	
	wkt = rtgeom_to_wkt(rtgeom, RTWKT_EXTENDED, 12, &wkt_size);

	if ( ! wkt )
	{
		rterror("Error writing geom %p to RTWKT", rtgeom);
	}

	return wkt;
}

/**
 * @brief geom1 same as geom2
 *  	iff
 *      	+ have same type
 *			+ have same # objects
 *      	+ have same bvol
 *      	+ each object in geom1 has a corresponding object in geom2 (see above)
 *	@param rtgeom1
 *	@param rtgeom2
 */
char
rtgeom_same(const RTGEOM *rtgeom1, const RTGEOM *rtgeom2)
{
	RTDEBUGF(2, "rtgeom_same(%s, %s) called",
	         rttype_name(rtgeom1->type),
	         rttype_name(rtgeom2->type));

	if ( rtgeom1->type != rtgeom2->type )
	{
		RTDEBUG(3, " type differ");

		return RT_FALSE;
	}

	if ( FLAGS_GET_ZM(rtgeom1->flags) != FLAGS_GET_ZM(rtgeom2->flags) )
	{
		RTDEBUG(3, " ZM flags differ");

		return RT_FALSE;
	}

	/* Check boxes if both already computed  */
	if ( rtgeom1->bbox && rtgeom2->bbox )
	{
		/*rtnotice("bbox1:%p, bbox2:%p", rtgeom1->bbox, rtgeom2->bbox);*/
		if ( ! gbox_same(rtgeom1->bbox, rtgeom2->bbox) )
		{
			RTDEBUG(3, " bounding boxes differ");

			return RT_FALSE;
		}
	}

	/* geoms have same type, invoke type-specific function */
	switch (rtgeom1->type)
	{
	case RTPOINTTYPE:
		return rtpoint_same((RTPOINT *)rtgeom1,
		                    (RTPOINT *)rtgeom2);
	case RTLINETYPE:
		return rtline_same((RTLINE *)rtgeom1,
		                   (RTLINE *)rtgeom2);
	case RTPOLYGONTYPE:
		return rtpoly_same((RTPOLY *)rtgeom1,
		                   (RTPOLY *)rtgeom2);
	case RTTRIANGLETYPE:
		return rttriangle_same((RTTRIANGLE *)rtgeom1,
		                       (RTTRIANGLE *)rtgeom2);
	case RTCIRCSTRINGTYPE:
		return rtcircstring_same((RTCIRCSTRING *)rtgeom1,
					 (RTCIRCSTRING *)rtgeom2);
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTMULTICURVETYPE:
	case RTMULTISURFACETYPE:
	case RTCOMPOUNDTYPE:
	case RTCURVEPOLYTYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
		return rtcollection_same((RTCOLLECTION *)rtgeom1,
		                         (RTCOLLECTION *)rtgeom2);
	default:
		rterror("rtgeom_same: unsupported geometry type: %s",
		        rttype_name(rtgeom1->type));
		return RT_FALSE;
	}

}

int
rtpoint_inside_circle(const RTPOINT *p, double cx, double cy, double rad)
{
	const RTPOINT2D *pt;
	RTPOINT2D center;

	if ( ! p || ! p->point )
		return RT_FALSE;
		
	pt = getPoint2d_cp(p->point, 0);

	center.x = cx;
	center.y = cy;

	if ( distance2d_pt_pt(pt, &center) < rad ) 
		return RT_TRUE;

	return RT_FALSE;
}

void
rtgeom_drop_bbox(RTGEOM *rtgeom)
{
	if ( rtgeom->bbox ) rtfree(rtgeom->bbox);
	rtgeom->bbox = NULL;
	FLAGS_SET_BBOX(rtgeom->flags, 0);
}

/**
 * Ensure there's a box in the RTGEOM.
 * If the box is already there just return,
 * else compute it.
 */
void
rtgeom_add_bbox(RTGEOM *rtgeom)
{
	/* an empty RTGEOM has no bbox */
	if ( rtgeom_is_empty(rtgeom) ) return;

	if ( rtgeom->bbox ) return;
	FLAGS_SET_BBOX(rtgeom->flags, 1);
	rtgeom->bbox = gbox_new(rtgeom->flags);
	rtgeom_calculate_gbox(rtgeom, rtgeom->bbox);
}

void 
rtgeom_add_bbox_deep(RTGEOM *rtgeom, RTGBOX *gbox)
{
	if ( rtgeom_is_empty(rtgeom) ) return;

	FLAGS_SET_BBOX(rtgeom->flags, 1);
	
	if ( ! ( gbox || rtgeom->bbox ) )
	{
		rtgeom->bbox = gbox_new(rtgeom->flags);
		rtgeom_calculate_gbox(rtgeom, rtgeom->bbox);		
	}
	else if ( gbox && ! rtgeom->bbox )
	{
		rtgeom->bbox = gbox_clone(gbox);
	}
	
	if ( rtgeom_is_collection(rtgeom) )
	{
		int i;
		RTCOLLECTION *rtcol = (RTCOLLECTION*)rtgeom;

		for ( i = 0; i < rtcol->ngeoms; i++ )
		{
			rtgeom_add_bbox_deep(rtcol->geoms[i], rtgeom->bbox);
		}
	}
}

const RTGBOX *
rtgeom_get_bbox(const RTGEOM *rtg)
{
	/* add it if not already there */
	rtgeom_add_bbox((RTGEOM *)rtg);
	return rtg->bbox;
}


/**
* Calculate the gbox for this goemetry, a cartesian box or
* geodetic box, depending on how it is flagged.
*/
int rtgeom_calculate_gbox(const RTGEOM *rtgeom, RTGBOX *gbox)
{
	gbox->flags = rtgeom->flags;
	if( FLAGS_GET_GEODETIC(rtgeom->flags) )
		return rtgeom_calculate_gbox_geodetic(rtgeom, gbox);
	else
		return rtgeom_calculate_gbox_cartesian(rtgeom, gbox);	
}

void
rtgeom_drop_srid(RTGEOM *rtgeom)
{
	rtgeom->srid = SRID_UNKNOWN;	/* TODO: To be changed to SRID_UNKNOWN */
}

RTGEOM *
rtgeom_segmentize2d(RTGEOM *rtgeom, double dist)
{
	switch (rtgeom->type)
	{
	case RTLINETYPE:
		return (RTGEOM *)rtline_segmentize2d((RTLINE *)rtgeom,
		                                     dist);
	case RTPOLYGONTYPE:
		return (RTGEOM *)rtpoly_segmentize2d((RTPOLY *)rtgeom,
		                                     dist);
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTCOLLECTIONTYPE:
		return (RTGEOM *)rtcollection_segmentize2d(
		           (RTCOLLECTION *)rtgeom, dist);

	default:
		return rtgeom_clone(rtgeom);
	}
}

RTGEOM*
rtgeom_force_2d(const RTGEOM *geom)
{	
	return rtgeom_force_dims(geom, 0, 0);
}

RTGEOM*
rtgeom_force_3dz(const RTGEOM *geom)
{	
	return rtgeom_force_dims(geom, 1, 0);
}

RTGEOM*
rtgeom_force_3dm(const RTGEOM *geom)
{	
	return rtgeom_force_dims(geom, 0, 1);
}

RTGEOM*
rtgeom_force_4d(const RTGEOM *geom)
{	
	return rtgeom_force_dims(geom, 1, 1);
}

RTGEOM*
rtgeom_force_dims(const RTGEOM *geom, int hasz, int hasm)
{	
	switch(geom->type)
	{
		case RTPOINTTYPE:
			return rtpoint_as_rtgeom(rtpoint_force_dims((RTPOINT*)geom, hasz, hasm));
		case RTCIRCSTRINGTYPE:
		case RTLINETYPE:
		case RTTRIANGLETYPE:
			return rtline_as_rtgeom(rtline_force_dims((RTLINE*)geom, hasz, hasm));
		case RTPOLYGONTYPE:
			return rtpoly_as_rtgeom(rtpoly_force_dims((RTPOLY*)geom, hasz, hasm));
		case RTCOMPOUNDTYPE:
		case RTCURVEPOLYTYPE:
		case RTMULTICURVETYPE:
		case RTMULTISURFACETYPE:
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTPOLYHEDRALSURFACETYPE:
		case RTTINTYPE:
		case RTCOLLECTIONTYPE:
			return rtcollection_as_rtgeom(rtcollection_force_dims((RTCOLLECTION*)geom, hasz, hasm));
		default:
			rterror("rtgeom_force_2d: unsupported geom type: %s", rttype_name(geom->type));
			return NULL;
	}
}

RTGEOM*
rtgeom_force_sfs(RTGEOM *geom, int version)
{	
	RTCOLLECTION *col;
	int i;
	RTGEOM *g;

	/* SFS 1.2 version */
	if (version == 120)
	{
		switch(geom->type)
		{
			/* SQL/MM types */
			case RTCIRCSTRINGTYPE:
			case RTCOMPOUNDTYPE:
			case RTCURVEPOLYTYPE:
			case RTMULTICURVETYPE:
			case RTMULTISURFACETYPE:
				return rtgeom_stroke(geom, 32);

			case RTCOLLECTIONTYPE:
				col = (RTCOLLECTION*)geom;
				for ( i = 0; i < col->ngeoms; i++ ) 
					col->geoms[i] = rtgeom_force_sfs((RTGEOM*)col->geoms[i], version);

				return rtcollection_as_rtgeom((RTCOLLECTION*)geom);

			default:
				return (RTGEOM *)geom;
		}
	}
	

	/* SFS 1.1 version */
	switch(geom->type)
	{
		/* SQL/MM types */
		case RTCIRCSTRINGTYPE:
		case RTCOMPOUNDTYPE:
		case RTCURVEPOLYTYPE:
		case RTMULTICURVETYPE:
		case RTMULTISURFACETYPE:
			return rtgeom_stroke(geom, 32);

		/* SFS 1.2 types */
		case RTTRIANGLETYPE:
			g = rtpoly_as_rtgeom(rtpoly_from_rtlines((RTLINE*)geom, 0, NULL));
			rtgeom_free(geom);
			return g;

		case RTTINTYPE:
			col = (RTCOLLECTION*) geom;
			for ( i = 0; i < col->ngeoms; i++ )
			{
				g = rtpoly_as_rtgeom(rtpoly_from_rtlines((RTLINE*)col->geoms[i], 0, NULL));
				rtgeom_free(col->geoms[i]);
				col->geoms[i] = g;
			}
			col->type = RTCOLLECTIONTYPE;
			return rtmpoly_as_rtgeom((RTMPOLY*)geom);
		
		case RTPOLYHEDRALSURFACETYPE:
			geom->type = RTCOLLECTIONTYPE;
			return (RTGEOM *)geom;

		/* Collection */
		case RTCOLLECTIONTYPE:
			col = (RTCOLLECTION*)geom;
			for ( i = 0; i < col->ngeoms; i++ ) 
				col->geoms[i] = rtgeom_force_sfs((RTGEOM*)col->geoms[i], version);

			return rtcollection_as_rtgeom((RTCOLLECTION*)geom);
		
		default:
			return (RTGEOM *)geom;
	}
}

int32_t 
rtgeom_get_srid(const RTGEOM *geom)
{
	if ( ! geom ) return SRID_UNKNOWN;
	return geom->srid;
}

uint32_t 
rtgeom_get_type(const RTGEOM *geom)
{
	if ( ! geom ) return 0;
	return geom->type;
}

int 
rtgeom_has_z(const RTGEOM *geom)
{
	if ( ! geom ) return RT_FALSE;
	return FLAGS_GET_Z(geom->flags);
}

int 
rtgeom_has_m(const RTGEOM *geom)
{
	if ( ! geom ) return RT_FALSE;
	return FLAGS_GET_M(geom->flags);
}

int 
rtgeom_ndims(const RTGEOM *geom)
{
	if ( ! geom ) return 0;
	return FLAGS_NDIMS(geom->flags);
}


void
rtgeom_set_geodetic(RTGEOM *geom, int value)
{
	RTPOINT *pt;
	RTLINE *ln;
	RTPOLY *ply;
	RTCOLLECTION *col;
	int i;
	
	FLAGS_SET_GEODETIC(geom->flags, value);
	if ( geom->bbox )
		FLAGS_SET_GEODETIC(geom->bbox->flags, value);
	
	switch(geom->type)
	{
		case RTPOINTTYPE:
			pt = (RTPOINT*)geom;
			if ( pt->point )
				FLAGS_SET_GEODETIC(pt->point->flags, value);
			break;
		case RTLINETYPE:
			ln = (RTLINE*)geom;
			if ( ln->points )
				FLAGS_SET_GEODETIC(ln->points->flags, value);
			break;
		case RTPOLYGONTYPE:
			ply = (RTPOLY*)geom;
			for ( i = 0; i < ply->nrings; i++ )
				FLAGS_SET_GEODETIC(ply->rings[i]->flags, value);
			break;
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTCOLLECTIONTYPE:
			col = (RTCOLLECTION*)geom;
			for ( i = 0; i < col->ngeoms; i++ )
				rtgeom_set_geodetic(col->geoms[i], value);
			break;
		default:
			rterror("rtgeom_set_geodetic: unsupported geom type: %s", rttype_name(geom->type));
			return;
	}
}

void
rtgeom_longitude_shift(RTGEOM *rtgeom)
{
	int i;
	switch (rtgeom->type)
	{
		RTPOINT *point;
		RTLINE *line;
		RTPOLY *poly;
		RTTRIANGLE *triangle;
		RTCOLLECTION *coll;

	case RTPOINTTYPE:
		point = (RTPOINT *)rtgeom;
		ptarray_longitude_shift(point->point);
		return;
	case RTLINETYPE:
		line = (RTLINE *)rtgeom;
		ptarray_longitude_shift(line->points);
		return;
	case RTPOLYGONTYPE:
		poly = (RTPOLY *)rtgeom;
		for (i=0; i<poly->nrings; i++)
			ptarray_longitude_shift(poly->rings[i]);
		return;
	case RTTRIANGLETYPE:
		triangle = (RTTRIANGLE *)rtgeom;
		ptarray_longitude_shift(triangle->points);
		return;
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
		coll = (RTCOLLECTION *)rtgeom;
		for (i=0; i<coll->ngeoms; i++)
			rtgeom_longitude_shift(coll->geoms[i]);
		return;
	default:
		rterror("rtgeom_longitude_shift: unsupported geom type: %s",
		        rttype_name(rtgeom->type));
	}
}

int 
rtgeom_is_closed(const RTGEOM *geom)
{
	int type = geom->type;
	
	if( rtgeom_is_empty(geom) )
		return RT_FALSE;
	
	/* Test linear types for closure */
	switch (type)
	{
	case RTLINETYPE:
		return rtline_is_closed((RTLINE*)geom);
	case RTPOLYGONTYPE:
		return rtpoly_is_closed((RTPOLY*)geom);
	case RTCIRCSTRINGTYPE:
		return rtcircstring_is_closed((RTCIRCSTRING*)geom);
	case RTCOMPOUNDTYPE:
		return rtcompound_is_closed((RTCOMPOUND*)geom);
	case RTTINTYPE:
		return rttin_is_closed((RTTIN*)geom);
	case RTPOLYHEDRALSURFACETYPE:
		return rtpsurface_is_closed((RTPSURFACE*)geom);
	}
	
	/* Recurse into collections and see if anything is not closed */
	if ( rtgeom_is_collection(geom) )
	{
		RTCOLLECTION *col = rtgeom_as_rtcollection(geom);
		int i;
		int closed;
		for ( i = 0; i < col->ngeoms; i++ ) 
		{
			closed = rtgeom_is_closed(col->geoms[i]);
			if ( ! closed ) 
				return RT_FALSE;
		}
		return RT_TRUE;
	}
	
	/* All non-linear non-collection types we will call closed */
	return RT_TRUE;
}

int 
rtgeom_is_collection(const RTGEOM *geom)
{
	if( ! geom ) return RT_FALSE;
	return rttype_is_collection(geom->type);
}

/** Return TRUE if the geometry may contain sub-geometries, i.e. it is a MULTI* or COMPOUNDCURVE */
int
rttype_is_collection(uint8_t type)
{

	switch (type)
	{
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTCOLLECTIONTYPE:
	case RTCURVEPOLYTYPE:
	case RTCOMPOUNDTYPE:
	case RTMULTICURVETYPE:
	case RTMULTISURFACETYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
		return RT_TRUE;
		break;

	default:
		return RT_FALSE;
	}
}

/**
* Given an rttype number, what homogeneous collection can hold it?
*/
int 
rttype_get_collectiontype(uint8_t type)
{
	switch (type)
	{
		case RTPOINTTYPE:
			return RTMULTIPOINTTYPE;
		case RTLINETYPE:
			return RTMULTILINETYPE;
		case RTPOLYGONTYPE:
			return RTMULTIPOLYGONTYPE;
		case RTCIRCSTRINGTYPE:
			return RTMULTICURVETYPE;
		case RTCOMPOUNDTYPE:
			return RTMULTICURVETYPE;
		case RTCURVEPOLYTYPE:
			return RTMULTISURFACETYPE;
		case RTTRIANGLETYPE:
			return RTTINTYPE;
		default:
			return RTCOLLECTIONTYPE;
	}
}


void rtgeom_free(RTGEOM *rtgeom)
{

	/* There's nothing here to free... */
	if( ! rtgeom ) return;

	RTDEBUGF(5,"freeing a %s",rttype_name(rtgeom->type));
	
	switch (rtgeom->type)
	{
	case RTPOINTTYPE:
		rtpoint_free((RTPOINT *)rtgeom);
		break;
	case RTLINETYPE:
		rtline_free((RTLINE *)rtgeom);
		break;
	case RTPOLYGONTYPE:
		rtpoly_free((RTPOLY *)rtgeom);
		break;
	case RTCIRCSTRINGTYPE:
		rtcircstring_free((RTCIRCSTRING *)rtgeom);
		break;
	case RTTRIANGLETYPE:
		rttriangle_free((RTTRIANGLE *)rtgeom);
		break;
	case RTMULTIPOINTTYPE:
		rtmpoint_free((RTMPOINT *)rtgeom);
		break;
	case RTMULTILINETYPE:
		rtmline_free((RTMLINE *)rtgeom);
		break;
	case RTMULTIPOLYGONTYPE:
		rtmpoly_free((RTMPOLY *)rtgeom);
		break;
	case RTPOLYHEDRALSURFACETYPE:
		rtpsurface_free((RTPSURFACE *)rtgeom);
		break;
	case RTTINTYPE:
		rttin_free((RTTIN *)rtgeom);
		break;
	case RTCURVEPOLYTYPE:
	case RTCOMPOUNDTYPE:
	case RTMULTICURVETYPE:
	case RTMULTISURFACETYPE:
	case RTCOLLECTIONTYPE:
		rtcollection_free((RTCOLLECTION *)rtgeom);
		break;
	default:
		rterror("rtgeom_free called with unknown type (%d) %s", rtgeom->type, rttype_name(rtgeom->type));
	}
	return;
}

int rtgeom_needs_bbox(const RTGEOM *geom)
{
	assert(geom);
	if ( geom->type == RTPOINTTYPE )
	{
		return RT_FALSE;
	}
	else if ( geom->type == RTLINETYPE )
	{
		if ( rtgeom_count_vertices(geom) <= 2 )
			return RT_FALSE;
		else
			return RT_TRUE;
	}
	else if ( geom->type == RTMULTIPOINTTYPE )
	{
		if ( ((RTCOLLECTION*)geom)->ngeoms == 1 )
			return RT_FALSE;
		else
			return RT_TRUE;
	}
	else if ( geom->type == RTMULTILINETYPE )
	{
		if ( ((RTCOLLECTION*)geom)->ngeoms == 1 && rtgeom_count_vertices(geom) <= 2 )
			return RT_FALSE;
		else
			return RT_TRUE;
	}
	else
	{
		return RT_TRUE;
	}
}

/**
* Count points in an #RTGEOM.
*/
int rtgeom_count_vertices(const RTGEOM *geom)
{
	int result = 0;
	
	/* Null? Zero. */
	if( ! geom ) return 0;
	
	RTDEBUGF(4, "rtgeom_count_vertices got type %s",
	         rttype_name(geom->type));

	/* Empty? Zero. */
	if( rtgeom_is_empty(geom) ) return 0;

	switch (geom->type)
	{
	case RTPOINTTYPE:
		result = 1;
		break;
	case RTTRIANGLETYPE:
	case RTCIRCSTRINGTYPE: 
	case RTLINETYPE:
		result = rtline_count_vertices((RTLINE *)geom);
		break;
	case RTPOLYGONTYPE:
		result = rtpoly_count_vertices((RTPOLY *)geom);
		break;
	case RTCOMPOUNDTYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTICURVETYPE:
	case RTMULTISURFACETYPE:
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
		result = rtcollection_count_vertices((RTCOLLECTION *)geom);
		break;
	default:
		rterror("%s: unsupported input geometry type: %s",
		        __func__, rttype_name(geom->type));
		break;
	}
	RTDEBUGF(3, "counted %d vertices", result);
	return result;
}

/**
* For an #RTGEOM, returns 0 for points, 1 for lines, 
* 2 for polygons, 3 for volume, and the max dimension 
* of a collection.
*/
int rtgeom_dimension(const RTGEOM *geom)
{

	/* Null? Zero. */
	if( ! geom ) return -1;
	
	RTDEBUGF(4, "rtgeom_dimension got type %s",
	         rttype_name(geom->type));

	/* Empty? Zero. */
	/* if( rtgeom_is_empty(geom) ) return 0; */

	switch (geom->type)
	{
	case RTPOINTTYPE:
	case RTMULTIPOINTTYPE:
		return 0;
	case RTCIRCSTRINGTYPE: 
	case RTLINETYPE:
	case RTCOMPOUNDTYPE:
	case RTMULTICURVETYPE:
	case RTMULTILINETYPE:
		return 1;
	case RTTRIANGLETYPE:
	case RTPOLYGONTYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTISURFACETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTTINTYPE:
		return 2;
	case RTPOLYHEDRALSURFACETYPE:
	{
		/* A closed polyhedral surface contains a volume. */
		int closed = rtpsurface_is_closed((RTPSURFACE*)geom);
		return ( closed ? 3 : 2 );
	}
	case RTCOLLECTIONTYPE:
	{
		int maxdim = 0, i;
		RTCOLLECTION *col = (RTCOLLECTION*)geom;
		for( i = 0; i < col->ngeoms; i++ )
		{
			int dim = rtgeom_dimension(col->geoms[i]);
			maxdim = ( dim > maxdim ? dim : maxdim );
		}
		return maxdim;
	}
	default:
		rterror("%s: unsupported input geometry type: %s",
		        __func__, rttype_name(geom->type));
	}
	return -1;
}

/**
* Count rings in an #RTGEOM.
*/
int rtgeom_count_rings(const RTGEOM *geom)
{
	int result = 0;
	
	/* Null? Empty? Zero. */
	if( ! geom || rtgeom_is_empty(geom) ) 
		return 0;

	switch (geom->type)
	{
	case RTPOINTTYPE:
	case RTCIRCSTRINGTYPE: 
	case RTCOMPOUNDTYPE:
	case RTMULTICURVETYPE:
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTLINETYPE:
		result = 0;
		break;
	case RTTRIANGLETYPE:
		result = 1;
		break;
	case RTPOLYGONTYPE:
		result = ((RTPOLY *)geom)->nrings;
		break;
	case RTCURVEPOLYTYPE:
		result = ((RTCURVEPOLY *)geom)->nrings;
		break;
	case RTMULTISURFACETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
	{
		RTCOLLECTION *col = (RTCOLLECTION*)geom;
		int i = 0;
		for( i = 0; i < col->ngeoms; i++ )
			result += rtgeom_count_rings(col->geoms[i]);
		break;
	}
	default:
		rterror("rtgeom_count_rings: unsupported input geometry type: %s", rttype_name(geom->type));
		break;
	}
	RTDEBUGF(3, "counted %d rings", result);
	return result;
}

int rtgeom_is_empty(const RTGEOM *geom)
{
	int result = RT_FALSE;
	RTDEBUGF(4, "rtgeom_is_empty: got type %s",
	         rttype_name(geom->type));

	switch (geom->type)
	{
	case RTPOINTTYPE:
		return rtpoint_is_empty((RTPOINT*)geom);
		break;
	case RTLINETYPE:
		return rtline_is_empty((RTLINE*)geom);
		break;
	case RTCIRCSTRINGTYPE:
		return rtcircstring_is_empty((RTCIRCSTRING*)geom);
		break;
	case RTPOLYGONTYPE:
		return rtpoly_is_empty((RTPOLY*)geom);
		break;
	case RTTRIANGLETYPE:
		return rttriangle_is_empty((RTTRIANGLE*)geom);
		break;
	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTCOMPOUNDTYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTICURVETYPE:
	case RTMULTISURFACETYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
	case RTCOLLECTIONTYPE:
		return rtcollection_is_empty((RTCOLLECTION *)geom);
		break;
	default:
		rterror("rtgeom_is_empty: unsupported input geometry type: %s",
		        rttype_name(geom->type));
		break;
	}
	return result;
}

int rtgeom_has_srid(const RTGEOM *geom)
{
	if ( geom->srid != SRID_UNKNOWN )
		return RT_TRUE;

	return RT_FALSE;
}


static int rtcollection_dimensionality(RTCOLLECTION *col)
{
	int i;
	int dimensionality = 0;
	for ( i = 0; i < col->ngeoms; i++ )
	{
		int d = rtgeom_dimensionality(col->geoms[i]);
		if ( d > dimensionality )
			dimensionality = d;
	}
	return dimensionality;
}

extern int rtgeom_dimensionality(RTGEOM *geom)
{
	int dim;

	RTDEBUGF(3, "rtgeom_dimensionality got type %s",
	         rttype_name(geom->type));

	switch (geom->type)
	{
	case RTPOINTTYPE:
	case RTMULTIPOINTTYPE:
		return 0;
		break;
	case RTLINETYPE:
	case RTCIRCSTRINGTYPE:
	case RTMULTILINETYPE:
	case RTCOMPOUNDTYPE:
	case RTMULTICURVETYPE:
		return 1;
		break;
	case RTPOLYGONTYPE:
	case RTTRIANGLETYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTIPOLYGONTYPE:
	case RTMULTISURFACETYPE:
		return 2;
		break;

	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
		dim = rtgeom_is_closed(geom)?3:2;
		return dim;
		break;

	case RTCOLLECTIONTYPE:
		return rtcollection_dimensionality((RTCOLLECTION *)geom);
		break;
	default:
		rterror("rtgeom_dimensionality: unsupported input geometry type: %s",
		        rttype_name(geom->type));
		break;
	}
	return 0;
}

extern RTGEOM* rtgeom_remove_repeated_points(const RTGEOM *in, double tolerance)
{
	RTDEBUGF(4, "rtgeom_remove_repeated_points got type %s",
	         rttype_name(in->type));

	if(rtgeom_is_empty(in)) 
	{
		return rtgeom_clone_deep(in);
	}

	switch (in->type)
	{
	case RTMULTIPOINTTYPE:
		return rtmpoint_remove_repeated_points((RTMPOINT*)in, tolerance);
		break;
	case RTLINETYPE:
		return rtline_remove_repeated_points((RTLINE*)in, tolerance);

	case RTMULTILINETYPE:
	case RTCOLLECTIONTYPE:
	case RTMULTIPOLYGONTYPE:
	case RTPOLYHEDRALSURFACETYPE:
		return rtcollection_remove_repeated_points((RTCOLLECTION *)in, tolerance);

	case RTPOLYGONTYPE:
		return rtpoly_remove_repeated_points((RTPOLY *)in, tolerance);
		break;

	case RTPOINTTYPE:
	case RTTRIANGLETYPE:
	case RTTINTYPE:
		/* No point is repeated for a single point, or for Triangle or TIN */
		return rtgeom_clone_deep(in);

	case RTCIRCSTRINGTYPE:
	case RTCOMPOUNDTYPE:
	case RTMULTICURVETYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTISURFACETYPE:
		/* Dunno how to handle these, will return untouched */
		return rtgeom_clone_deep(in);

	default:
		rtnotice("%s: unsupported geometry type: %s",
		         __func__, rttype_name(in->type));
		return rtgeom_clone_deep(in);
		break;
	}
	return 0;
}

RTGEOM* rtgeom_flip_coordinates(RTGEOM *in)
{
  rtgeom_swap_ordinates(in,RTORD_X,RTORD_Y);
  return in;
}

void rtgeom_swap_ordinates(RTGEOM *in, RTORD o1, RTORD o2)
{
	RTCOLLECTION *col;
	RTPOLY *poly;
	int i;

#if PARANOIA_LEVEL > 0
  assert(o1 < 4);
  assert(o2 < 4);
#endif

	if ( (!in) || rtgeom_is_empty(in) ) return;

  /* TODO: check for rtgeom NOT having the specified dimension ? */

	RTDEBUGF(4, "rtgeom_flip_coordinates, got type: %s",
	         rttype_name(in->type));

	switch (in->type)
	{
	case RTPOINTTYPE:
		ptarray_swap_ordinates(rtgeom_as_rtpoint(in)->point, o1, o2);
		break;

	case RTLINETYPE:
		ptarray_swap_ordinates(rtgeom_as_rtline(in)->points, o1, o2);
		break;

	case RTCIRCSTRINGTYPE:
		ptarray_swap_ordinates(rtgeom_as_rtcircstring(in)->points, o1, o2);
		break;

	case RTPOLYGONTYPE:
		poly = (RTPOLY *) in;
		for (i=0; i<poly->nrings; i++)
		{
			ptarray_swap_ordinates(poly->rings[i], o1, o2);
		}
		break;

	case RTTRIANGLETYPE:
		ptarray_swap_ordinates(rtgeom_as_rttriangle(in)->points, o1, o2);
		break;

	case RTMULTIPOINTTYPE:
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTCOLLECTIONTYPE:
	case RTCOMPOUNDTYPE:
	case RTCURVEPOLYTYPE:
	case RTMULTISURFACETYPE:
	case RTMULTICURVETYPE:
	case RTPOLYHEDRALSURFACETYPE:
	case RTTINTYPE:
		col = (RTCOLLECTION *) in;
		for (i=0; i<col->ngeoms; i++)
		{
			rtgeom_swap_ordinates(col->geoms[i], o1, o2);
		}
		break;

	default:
		rterror("rtgeom_swap_ordinates: unsupported geometry type: %s",
		        rttype_name(in->type));
		return;
	}

	/* only refresh bbox if X or Y changed */
	if ( in->bbox && (o1 < 2 || o2 < 2) ) 
	{
		rtgeom_drop_bbox(in);
		rtgeom_add_bbox(in);
	}
}

void rtgeom_set_srid(RTGEOM *geom, int32_t srid)
{
	int i;

	RTDEBUGF(4,"entered with srid=%d",srid);

	geom->srid = srid;

	if ( rtgeom_is_collection(geom) )
	{
		/* All the children are set to the same SRID value */
		RTCOLLECTION *col = rtgeom_as_rtcollection(geom);
		for ( i = 0; i < col->ngeoms; i++ )
		{
			rtgeom_set_srid(col->geoms[i], srid);
		}
	}
}

RTGEOM* rtgeom_simplify(const RTGEOM *igeom, double dist, int preserve_collapsed)
{
	switch (igeom->type)
	{
	case RTPOINTTYPE:
	case RTMULTIPOINTTYPE:
		return rtgeom_clone(igeom);
	case RTLINETYPE:
		return (RTGEOM*)rtline_simplify((RTLINE*)igeom, dist, preserve_collapsed);
	case RTPOLYGONTYPE:
		return (RTGEOM*)rtpoly_simplify((RTPOLY*)igeom, dist, preserve_collapsed);
	case RTMULTILINETYPE:
	case RTMULTIPOLYGONTYPE:
	case RTCOLLECTIONTYPE:
		return (RTGEOM*)rtcollection_simplify((RTCOLLECTION *)igeom, dist, preserve_collapsed);
	default:
		rterror("%s: unsupported geometry type: %s", __func__, rttype_name(igeom->type));
	}
	return NULL;
}

double rtgeom_area(const RTGEOM *geom)
{
	int type = geom->type;
	
	if ( type == RTPOLYGONTYPE )
		return rtpoly_area((RTPOLY*)geom);
	else if ( type == RTCURVEPOLYTYPE )
		return rtcurvepoly_area((RTCURVEPOLY*)geom);
	else if (type ==  RTTRIANGLETYPE )
		return rttriangle_area((RTTRIANGLE*)geom);
	else if ( rtgeom_is_collection(geom) )
	{
		double area = 0.0;
		int i;
		RTCOLLECTION *col = (RTCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			area += rtgeom_area(col->geoms[i]);
		return area;
	}
	else
		return 0.0;
}

double rtgeom_perimeter(const RTGEOM *geom)
{
	int type = geom->type;
	if ( type == RTPOLYGONTYPE )
		return rtpoly_perimeter((RTPOLY*)geom);
	else if ( type == RTCURVEPOLYTYPE )
		return rtcurvepoly_perimeter((RTCURVEPOLY*)geom);
	else if ( type == RTTRIANGLETYPE )
		return rttriangle_perimeter((RTTRIANGLE*)geom);
	else if ( rtgeom_is_collection(geom) )
	{
		double perimeter = 0.0;
		int i;
		RTCOLLECTION *col = (RTCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			perimeter += rtgeom_perimeter(col->geoms[i]);
		return perimeter;
	}
	else
		return 0.0;
}

double rtgeom_perimeter_2d(const RTGEOM *geom)
{
	int type = geom->type;
	if ( type == RTPOLYGONTYPE )
		return rtpoly_perimeter_2d((RTPOLY*)geom);
	else if ( type == RTCURVEPOLYTYPE )
		return rtcurvepoly_perimeter_2d((RTCURVEPOLY*)geom);
	else if ( type == RTTRIANGLETYPE )
		return rttriangle_perimeter_2d((RTTRIANGLE*)geom);
	else if ( rtgeom_is_collection(geom) )
	{
		double perimeter = 0.0;
		int i;
		RTCOLLECTION *col = (RTCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			perimeter += rtgeom_perimeter_2d(col->geoms[i]);
		return perimeter;
	}
	else
		return 0.0;
}

double rtgeom_length(const RTGEOM *geom)
{
	int type = geom->type;
	if ( type == RTLINETYPE )
		return rtline_length((RTLINE*)geom);
	else if ( type == RTCIRCSTRINGTYPE )
		return rtcircstring_length((RTCIRCSTRING*)geom);
	else if ( type == RTCOMPOUNDTYPE )
		return rtcompound_length((RTCOMPOUND*)geom);
	else if ( rtgeom_is_collection(geom) )
	{
		double length = 0.0;
		int i;
		RTCOLLECTION *col = (RTCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			length += rtgeom_length(col->geoms[i]);
		return length;
	}
	else
		return 0.0;
}

double rtgeom_length_2d(const RTGEOM *geom)
{
	int type = geom->type;
	if ( type == RTLINETYPE )
		return rtline_length_2d((RTLINE*)geom);
	else if ( type == RTCIRCSTRINGTYPE )
		return rtcircstring_length_2d((RTCIRCSTRING*)geom);
	else if ( type == RTCOMPOUNDTYPE )
		return rtcompound_length_2d((RTCOMPOUND*)geom);
	else if ( rtgeom_is_collection(geom) )
	{
		double length = 0.0;
		int i;
		RTCOLLECTION *col = (RTCOLLECTION*)geom;
		for ( i = 0; i < col->ngeoms; i++ )
			length += rtgeom_length_2d(col->geoms[i]);
		return length;
	}
	else
		return 0.0;
}

void
rtgeom_affine(RTGEOM *geom, const AFFINE *affine)
{
	int type = geom->type;
	int i;

	switch(type) 
	{
		/* Take advantage of fact tht pt/ln/circ/tri have same memory structure */
		case RTPOINTTYPE:
		case RTLINETYPE:
		case RTCIRCSTRINGTYPE:
		case RTTRIANGLETYPE:
		{
			RTLINE *l = (RTLINE*)geom;
			ptarray_affine(l->points, affine);
			break;
		}
		case RTPOLYGONTYPE:
		{
			RTPOLY *p = (RTPOLY*)geom;
			for( i = 0; i < p->nrings; i++ )
				ptarray_affine(p->rings[i], affine);
			break;
		}
		case RTCURVEPOLYTYPE:
		{
			RTCURVEPOLY *c = (RTCURVEPOLY*)geom;
			for( i = 0; i < c->nrings; i++ )
				rtgeom_affine(c->rings[i], affine);
			break;
		}
		default:
		{
			if( rtgeom_is_collection(geom) )
			{
				RTCOLLECTION *c = (RTCOLLECTION*)geom;
				for( i = 0; i < c->ngeoms; i++ )
				{
					rtgeom_affine(c->geoms[i], affine);
				}
			}
			else 
			{
				rterror("rtgeom_affine: unable to handle type '%s'", rttype_name(type));
			}
		}
	}

}

void
rtgeom_scale(RTGEOM *geom, const RTPOINT4D *factor)
{
	int type = geom->type;
	int i;

	switch(type) 
	{
		/* Take advantage of fact tht pt/ln/circ/tri have same memory structure */
		case RTPOINTTYPE:
		case RTLINETYPE:
		case RTCIRCSTRINGTYPE:
		case RTTRIANGLETYPE:
		{
			RTLINE *l = (RTLINE*)geom;
			ptarray_scale(l->points, factor);
			break;
		}
		case RTPOLYGONTYPE:
		{
			RTPOLY *p = (RTPOLY*)geom;
			for( i = 0; i < p->nrings; i++ )
				ptarray_scale(p->rings[i], factor);
			break;
		}
		case RTCURVEPOLYTYPE:
		{
			RTCURVEPOLY *c = (RTCURVEPOLY*)geom;
			for( i = 0; i < c->nrings; i++ )
				rtgeom_scale(c->rings[i], factor);
			break;
		}
		default:
		{
			if( rtgeom_is_collection(geom) )
			{
				RTCOLLECTION *c = (RTCOLLECTION*)geom;
				for( i = 0; i < c->ngeoms; i++ )
				{
					rtgeom_scale(c->geoms[i], factor);
				}
			}
			else 
			{
				rterror("rtgeom_scale: unable to handle type '%s'", rttype_name(type));
			}
		}
	}

	/* Recompute bbox if needed */

	if ( geom->bbox ) 
	{
		/* TODO: expose a gbox_scale function */
		geom->bbox->xmin *= factor->x;
		geom->bbox->xmax *= factor->x;
		geom->bbox->ymin *= factor->y;
		geom->bbox->ymax *= factor->y;
		geom->bbox->zmin *= factor->z;
		geom->bbox->zmax *= factor->z;
		geom->bbox->mmin *= factor->m;
		geom->bbox->mmax *= factor->m;
	}
}

RTGEOM*
rtgeom_construct_empty(uint8_t type, int srid, char hasz, char hasm)
{
	switch(type) 
	{
		case RTPOINTTYPE:
			return rtpoint_as_rtgeom(rtpoint_construct_empty(srid, hasz, hasm));
		case RTLINETYPE:
			return rtline_as_rtgeom(rtline_construct_empty(srid, hasz, hasm));
		case RTPOLYGONTYPE:
			return rtpoly_as_rtgeom(rtpoly_construct_empty(srid, hasz, hasm));
		case RTCURVEPOLYTYPE:
			return rtcurvepoly_as_rtgeom(rtcurvepoly_construct_empty(srid, hasz, hasm));
		case RTCIRCSTRINGTYPE:
			return rtcircstring_as_rtgeom(rtcircstring_construct_empty(srid, hasz, hasm));
		case RTTRIANGLETYPE:
			return rttriangle_as_rtgeom(rttriangle_construct_empty(srid, hasz, hasm));
		case RTCOMPOUNDTYPE:
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTCOLLECTIONTYPE:
			return rtcollection_as_rtgeom(rtcollection_construct_empty(type, srid, hasz, hasm));
		default:
			rterror("rtgeom_construct_empty: unsupported geometry type: %s",
		        	rttype_name(type));
			return NULL;
	}
}

int
rtgeom_startpoint(const RTGEOM* rtgeom, RTPOINT4D* pt)
{
	if ( ! rtgeom )
		return RT_FAILURE;
		
	switch( rtgeom->type ) 
	{
		case RTPOINTTYPE:
			return ptarray_startpoint(((RTPOINT*)rtgeom)->point, pt);
		case RTTRIANGLETYPE:
		case RTCIRCSTRINGTYPE:
		case RTLINETYPE:
			return ptarray_startpoint(((RTLINE*)rtgeom)->points, pt);
		case RTPOLYGONTYPE:
			return rtpoly_startpoint((RTPOLY*)rtgeom, pt);
		case RTCURVEPOLYTYPE:
		case RTCOMPOUNDTYPE:
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTCOLLECTIONTYPE:
			return rtcollection_startpoint((RTCOLLECTION*)rtgeom, pt);
		default:
			rterror("int: unsupported geometry type: %s",
		        	rttype_name(rtgeom->type));
			return RT_FAILURE;
	}
}


RTGEOM *
rtgeom_grid(const RTGEOM *rtgeom, const gridspec *grid)
{
	switch ( rtgeom->type )
	{
		case RTPOINTTYPE:
			return (RTGEOM *)rtpoint_grid((RTPOINT *)rtgeom, grid);
		case RTLINETYPE:
			return (RTGEOM *)rtline_grid((RTLINE *)rtgeom, grid);
		case RTPOLYGONTYPE:
			return (RTGEOM *)rtpoly_grid((RTPOLY *)rtgeom, grid);
		case RTMULTIPOINTTYPE:
		case RTMULTILINETYPE:
		case RTMULTIPOLYGONTYPE:
		case RTCOLLECTIONTYPE:
		case RTCOMPOUNDTYPE:
			return (RTGEOM *)rtcollection_grid((RTCOLLECTION *)rtgeom, grid);
		case RTCIRCSTRINGTYPE:
			return (RTGEOM *)rtcircstring_grid((RTCIRCSTRING *)rtgeom, grid);
		default:
			rterror("rtgeom_grid: Unsupported geometry type: %s",
			        rttype_name(rtgeom->type));
			return NULL;
	}
}


/* Prototype for recursion */
static int 
rtgeom_subdivide_recursive(const RTGEOM *geom, int maxvertices, int depth, RTCOLLECTION *col, const RTGBOX *clip);

static int
rtgeom_subdivide_recursive(const RTGEOM *geom, int maxvertices, int depth, RTCOLLECTION *col, const RTGBOX *clip)
{
	const int maxdepth = 50;
	int nvertices = 0;
	int i, n = 0;
	double width = clip->xmax - clip->xmin;
	double height = clip->ymax - clip->ymin;
	RTGBOX subbox1, subbox2;
	RTGEOM *clipped1, *clipped2;
	
	if ( geom->type == RTPOLYHEDRALSURFACETYPE || geom->type == RTTINTYPE )
	{
		rterror("%s: unsupported geometry type '%s'", __func__, rttype_name(geom->type));
	}
	
	if ( width == 0.0 && height == 0.0 )
		return 0;
	
	/* Artays just recurse into collections */
	if ( rtgeom_is_collection(geom) )
	{
		RTCOLLECTION *incol = (RTCOLLECTION*)geom;
		int n = 0;
		for ( i = 0; i < incol->ngeoms; i++ )
		{
			/* Don't increment depth yet, since we aren't actually subdividing geomtries yet */
			n += rtgeom_subdivide_recursive(incol->geoms[i], maxvertices, depth, col, clip);
		}
		return n;
	}
	
	/* But don't go too far. 2^25 = 33M, that's enough subdivision */
	/* Signal callers above that we depth'ed out with a negative */
	/* return value */
	if ( depth > maxdepth )
	{
		return 0;
	}
	
	nvertices = rtgeom_count_vertices(geom);
	/* Skip empties entirely */
	if ( nvertices == 0 )
	{
		return 0;
	}
	
	/* If it is under the vertex tolerance, just add it, we're done */
	if ( nvertices < maxvertices )
	{
		rtcollection_add_rtgeom(col, rtgeom_clone_deep(geom));
		return 1;
	}
	
	subbox1 = subbox2 = *clip;
	if ( width > height )
	{
		subbox1.xmax = subbox2.xmin = (clip->xmin + clip->xmax)/2;
	}
	else
	{
		subbox1.ymax = subbox2.ymin = (clip->ymin + clip->ymax)/2;
	}
	
	if ( height == 0 )
	{
		subbox1.ymax += FP_TOLERANCE;
		subbox2.ymax += FP_TOLERANCE;
		subbox1.ymin -= FP_TOLERANCE;
		subbox2.ymin -= FP_TOLERANCE;
	}

	if ( width == 0 )
	{
		subbox1.xmax += FP_TOLERANCE;
		subbox2.xmax += FP_TOLERANCE;
		subbox1.xmin -= FP_TOLERANCE;
		subbox2.xmin -= FP_TOLERANCE;
	}
		
	clipped1 = rtgeom_clip_by_rect(geom, subbox1.xmin, subbox1.ymin, subbox1.xmax, subbox1.ymax);
	clipped2 = rtgeom_clip_by_rect(geom, subbox2.xmin, subbox2.ymin, subbox2.xmax, subbox2.ymax);
	
	if ( clipped1 )
	{
		n += rtgeom_subdivide_recursive(clipped1, maxvertices, ++depth, col, &subbox1);
		rtgeom_free(clipped1);
	}

	if ( clipped2 )
	{
		n += rtgeom_subdivide_recursive(clipped2, maxvertices, ++depth, col, &subbox2);
		rtgeom_free(clipped2);
	}
	
	return n;
	
}

RTCOLLECTION *
rtgeom_subdivide(const RTGEOM *geom, int maxvertices)
{
	static int startdepth = 0;
	static int minmaxvertices = 8;
	RTCOLLECTION *col;
	RTGBOX clip;
	
	col = rtcollection_construct_empty(RTCOLLECTIONTYPE, geom->srid, rtgeom_has_z(geom), rtgeom_has_m(geom));

	if ( rtgeom_is_empty(geom) )
		return col;

	if ( maxvertices < minmaxvertices )
	{
		rtcollection_free(col);
		rterror("%s: cannot subdivide to fewer than %d vertices per output", __func__, minmaxvertices);
	}
	
	clip = *(rtgeom_get_bbox(geom));
	rtgeom_subdivide_recursive(geom, maxvertices, startdepth, col, &clip);
	rtgeom_set_srid((RTGEOM*)col, geom->srid);
	return col;
}


int
rtgeom_is_trajectory(const RTGEOM *geom)
{
	int type = geom->type;

	if( type != RTLINETYPE ) 
	{
		rtnotice("Geometry is not a LINESTRING");
		return RT_FALSE;
	}
	return rtline_is_trajectory((RTLINE*)geom);
}

