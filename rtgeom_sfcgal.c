/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Wrapper around SFCGAL for 3D functions
 *
 * Copyright 2012-2013 Oslandia <infos@oslandia.com>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#include "rtgeom_sfcgal.h"

static int SFCGAL_type_to_rtgeom_type(sfcgal_geometry_type_t type);
static POINTARRAY* ptarray_from_SFCGAL(const sfcgal_geometry_t* geom, int force3D);
static sfcgal_geometry_t* ptarray_to_SFCGAL(const POINTARRAY* pa, int type);



/* Return SFCGAL version string */
const char*
rtgeom_sfcgal_version()
{
        const char *version = sfcgal_version();
        return version;
}


/*
 * Mapping between SFCGAL and PostGIS types
 *
 * Throw an error if type is unsupported
 */
static int
SFCGAL_type_to_rtgeom_type(sfcgal_geometry_type_t type)
{
	switch (type)
	{
	case SFCGAL_TYPE_POINT:
		return POINTTYPE;

	case SFCGAL_TYPE_LINESTRING:
		return LINETYPE;

	case SFCGAL_TYPE_POLYGON:
		return POLYGONTYPE;

	case SFCGAL_TYPE_MULTIPOINT:
		return MULTIPOINTTYPE;

	case SFCGAL_TYPE_MULTILINESTRING:
		return MULTILINETYPE;

	case SFCGAL_TYPE_MULTIPOLYGON:
		return MULTIPOLYGONTYPE;

	case SFCGAL_TYPE_MULTISOLID:
		return COLLECTIONTYPE;  /* Nota: PolyhedralSurface closed inside
             				   aim is to use true solid type as soon
	           			   as available in OGC SFS */

	case SFCGAL_TYPE_GEOMETRYCOLLECTION:
		return COLLECTIONTYPE;

#if 0
	case SFCGAL_TYPE_CIRCULARSTRING:
		return CIRCSTRINGTYPE;

	case SFCGAL_TYPE_COMPOUNDCURVE:
		return COMPOUNDTYPE;

	case SFCGAL_TYPE_CURVEPOLYGON:
		return CURVEPOLYTYPE;

	case SFCGAL_TYPE_MULTICURVE:
		return MULTICURVETYPE;

	case SFCGAL_TYPE_MULTISURFACE:
		return MULTISURFACETYPE;
#endif

	case SFCGAL_TYPE_POLYHEDRALSURFACE:
		return POLYHEDRALSURFACETYPE;

	case SFCGAL_TYPE_TRIANGULATEDSURFACE:
		return TINTYPE;

	case SFCGAL_TYPE_TRIANGLE:
		return TRIANGLETYPE;

	default:
		rterror("SFCGAL_type_to_rtgeom_type: Unknown Type");
		return 0;
	}
}


/*
 * Return a PostGIS pointarray from a simple SFCGAL geometry:
 * POINT, LINESTRING or TRIANGLE
 *
 * Trought an error on others types
 */
static POINTARRAY*
ptarray_from_SFCGAL(const sfcgal_geometry_t* geom, int want3d)
{
	POINT4D point;
	uint32_t i, npoints;
	POINTARRAY* pa = NULL;

	assert(geom);

	switch (sfcgal_geometry_type_id(geom))
	{
	case SFCGAL_TYPE_POINT:
	{
		pa = ptarray_construct(want3d, 0, 1);
		point.x = sfcgal_point_x(geom);
		point.y = sfcgal_point_y(geom);

		if (sfcgal_geometry_is_3d(geom))
			point.z = sfcgal_point_z(geom);
		else if (want3d)
			point.z = 0.0;

		ptarray_set_point4d(pa, 0, &point);
	}
	break;

	case SFCGAL_TYPE_LINESTRING:
	{
		npoints = sfcgal_linestring_num_points(geom);
		pa = ptarray_construct(want3d, 0, npoints);

		for (i = 0; i < npoints; i++)
		{
			const sfcgal_geometry_t* pt = sfcgal_linestring_point_n(geom, i);
			point.x = sfcgal_point_x(pt);
			point.y = sfcgal_point_y(pt);

			if (sfcgal_geometry_is_3d(geom))
				point.z = sfcgal_point_z(pt);
			else if (want3d)
				point.z = 0.0;

			ptarray_set_point4d(pa, i, &point);
		}
	}
	break;

	case SFCGAL_TYPE_TRIANGLE:
	{
		pa = ptarray_construct(want3d, 0, 4);

		for (i = 0; i < 4; i++)
		{
			const sfcgal_geometry_t* pt = sfcgal_triangle_vertex(geom, (i%3));
			point.x = sfcgal_point_x(pt);
			point.y = sfcgal_point_y(pt);

			if ( sfcgal_geometry_is_3d(geom))
				point.z = sfcgal_point_z(pt);
			else if (want3d)
				point.z = 0.0;

			ptarray_set_point4d(pa, i, &point);
		}
	}
	break;

	/* Other types should not be called directly ... */
	default:
		rterror("ptarray_from_SFCGAL: Unknown Type");
		break;
	}
	return pa;
}


/*
 * Convert a PostGIS pointarray to SFCGAL structure
 *
 * Used for simple RTGEOM geometry POINT, LINESTRING, TRIANGLE
 * and POLYGON rings
 */
static sfcgal_geometry_t*
ptarray_to_SFCGAL(const POINTARRAY* pa, int type)
{
	POINT3DZ point;
	int is_3d;
	uint32_t i;

	assert(pa);

	is_3d = FLAGS_GET_Z(pa->flags) != 0;

	switch (type)
	{
	case POINTTYPE:
	{
		getPoint3dz_p(pa, 0, &point);
		if (is_3d) return sfcgal_point_create_from_xyz(point.x, point.y, point.z);
		else       return sfcgal_point_create_from_xy(point.x, point.y);
	}
	break;

	case LINETYPE:
	{
		sfcgal_geometry_t* line = sfcgal_linestring_create();

		for (i = 0; i < pa->npoints; i++)
		{
			getPoint3dz_p(pa, i, &point);
			if (is_3d)
			{
				sfcgal_linestring_add_point(line,
				                            sfcgal_point_create_from_xyz(point.x, point.y, point.z));
			}
			else
			{
				sfcgal_linestring_add_point(line,
				                            sfcgal_point_create_from_xy(point.x, point.y));
			}
		}

		return line;
	}
	break;

	case TRIANGLETYPE:
	{
		sfcgal_geometry_t* triangle = sfcgal_triangle_create();

		getPoint3dz_p(pa, 0, &point);
		if (is_3d) sfcgal_triangle_set_vertex_from_xyz(triangle, 0, point.x, point.y, point.z);
		else       sfcgal_triangle_set_vertex_from_xy (triangle, 0, point.x, point.y);

		getPoint3dz_p(pa, 1, &point);
		if (is_3d) sfcgal_triangle_set_vertex_from_xyz(triangle, 1, point.x, point.y, point.z);
		else       sfcgal_triangle_set_vertex_from_xy (triangle, 1, point.x, point.y);

		getPoint3dz_p(pa, 2, &point);
		if (is_3d) sfcgal_triangle_set_vertex_from_xyz(triangle, 2, point.x, point.y, point.z);
		else       sfcgal_triangle_set_vertex_from_xy (triangle, 2, point.x, point.y);

		return triangle;
	}
	break;

	/* Other SFCGAL types should not be called directly ... */
	default:
		rterror("ptarray_from_SFCGAL: Unknown Type");
		return NULL;
	}
}


/*
 * Convert a SFCGAL structure to PostGIS RTGEOM
 *
 * Throws an error on unsupported type
 */
RTGEOM*
SFCGAL2RTGEOM(const sfcgal_geometry_t* geom, int force3D, int srid)
{
	uint32_t ngeoms, nshells, nsolids;
	uint32_t i, j, k;
	int want3d;

	assert(geom);

	want3d = force3D || sfcgal_geometry_is_3d(geom);

	switch (sfcgal_geometry_type_id(geom))
	{
	case SFCGAL_TYPE_POINT:
	{
		if (sfcgal_geometry_is_empty(geom))
			return (RTGEOM*) rtpoint_construct_empty(srid, want3d, 0);

		POINTARRAY* pa = ptarray_from_SFCGAL(geom, want3d);
		return (RTGEOM*) rtpoint_construct(srid, NULL, pa);
	}

	case SFCGAL_TYPE_LINESTRING:
	{
		if (sfcgal_geometry_is_empty(geom))
			return (RTGEOM*) rtline_construct_empty(srid, want3d, 0);

		POINTARRAY* pa = ptarray_from_SFCGAL(geom, want3d);
		return (RTGEOM*) rtline_construct(srid, NULL, pa);
	}

	case SFCGAL_TYPE_TRIANGLE:
	{
		if (sfcgal_geometry_is_empty(geom))
			return (RTGEOM*) rttriangle_construct_empty(srid, want3d, 0);

		POINTARRAY* pa = ptarray_from_SFCGAL(geom, want3d);
		return (RTGEOM*) rttriangle_construct(srid, NULL, pa);
	}

	case SFCGAL_TYPE_POLYGON:
	{
		if (sfcgal_geometry_is_empty(geom))
			return (RTGEOM*) rtpoly_construct_empty(srid, want3d, 0);

		uint32_t nrings = sfcgal_polygon_num_interior_rings(geom) + 1;
		POINTARRAY** pa = (POINTARRAY**) rtalloc(sizeof(POINTARRAY*) * nrings);

		pa[0] = ptarray_from_SFCGAL(sfcgal_polygon_exterior_ring(geom), want3d);
		for (i = 1; i < nrings; i++)
			pa[i] = ptarray_from_SFCGAL(sfcgal_polygon_interior_ring_n(geom, i-1), want3d);

		return (RTGEOM*) rtpoly_construct(srid, NULL, nrings, pa);
	}

	case SFCGAL_TYPE_MULTIPOINT:
	case SFCGAL_TYPE_MULTILINESTRING:
	case SFCGAL_TYPE_MULTIPOLYGON:
	case SFCGAL_TYPE_MULTISOLID:
	case SFCGAL_TYPE_GEOMETRYCOLLECTION:
	{
		ngeoms = sfcgal_geometry_collection_num_geometries(geom);
		RTGEOM** geoms = NULL;
		if (ngeoms)
		{
			nsolids = 0;
			geoms = (RTGEOM**) rtalloc(sizeof(RTGEOM*) * ngeoms);
			for (i = 0; i < ngeoms; i++)
			{
				const sfcgal_geometry_t* g = sfcgal_geometry_collection_geometry_n(geom, i);
				geoms[i] = SFCGAL2RTGEOM(g, 0, srid);
				if ( FLAGS_GET_SOLID( geoms[i]->flags ) ) ++nsolids;
			}
			geoms = (RTGEOM**) rtrealloc(geoms, sizeof(RTGEOM*) * ngeoms);
		}
		RTGEOM* rgeom = (RTGEOM*) rtcollection_construct(SFCGAL_type_to_rtgeom_type(
				sfcgal_geometry_type_id(geom)), srid, NULL, ngeoms, geoms);
		if ( ngeoms )
		{
		        if ( ngeoms == nsolids ) FLAGS_SET_SOLID( rgeom->flags, 1);
			else if ( nsolids ) rtnotice("SFCGAL2RTGEOM: SOLID in heterogeneous collection will be treated as a POLYHEDRALSURFACETYPE");
		}
		return rgeom;
	}

#if 0
	case SFCGAL_TYPE_CIRCULARSTRING:
	case SFCGAL_TYPE_COMPOUNDCURVE:
	case SFCGAL_TYPE_CURVEPOLYGON:
	case SFCGAL_TYPE_MULTICURVE:
	case SFCGAL_TYPE_MULTISURFACE:
	case SFCGAL_TYPE_CURVE:
	case SFCGAL_TYPE_SURFACE:

	/* TODO curve types handling */
#endif

	case SFCGAL_TYPE_POLYHEDRALSURFACE:
	{
		ngeoms = sfcgal_polyhedral_surface_num_polygons(geom);

		RTGEOM** geoms = NULL;
		if (ngeoms)
		{
			geoms = (RTGEOM**) rtalloc(sizeof(RTGEOM*) * ngeoms);
			for (i = 0; i < ngeoms; i++)
			{
				const sfcgal_geometry_t* g = sfcgal_polyhedral_surface_polygon_n( geom, i );
				geoms[i] = SFCGAL2RTGEOM(g, 0, srid);
			}
		}
		return (RTGEOM*)rtcollection_construct(POLYHEDRALSURFACETYPE, srid, NULL, ngeoms, geoms);
	}

	/* Solid is map as a closed PolyhedralSurface (for now) */
	case SFCGAL_TYPE_SOLID:
	{
		nshells = sfcgal_solid_num_shells(geom);

		for (ngeoms = 0, i = 0; i < nshells; i++)
			ngeoms += sfcgal_polyhedral_surface_num_polygons(sfcgal_solid_shell_n(geom, i));

		RTGEOM** geoms = 0;
		if (ngeoms)
		{
			geoms = (RTGEOM**) rtalloc( sizeof(RTGEOM*) * ngeoms);
			for (i = 0, k =0 ; i < nshells; i++)
			{
				const sfcgal_geometry_t* shell = sfcgal_solid_shell_n(geom, i);
				ngeoms = sfcgal_polyhedral_surface_num_polygons(shell);

				for (j = 0; j < ngeoms; j++)
				{
					const sfcgal_geometry_t* g = sfcgal_polyhedral_surface_polygon_n(shell, j);
					geoms[k] = SFCGAL2RTGEOM(g, 1, srid);
					k++;
				}
			}
		}
		RTGEOM* rgeom =  (RTGEOM*) rtcollection_construct(POLYHEDRALSURFACETYPE, srid, NULL, ngeoms, geoms);
		if (ngeoms) FLAGS_SET_SOLID( rgeom->flags, 1);
		return rgeom;
	}

	case SFCGAL_TYPE_TRIANGULATEDSURFACE:
	{
		ngeoms = sfcgal_triangulated_surface_num_triangles(geom);
		RTGEOM** geoms = NULL;
		if (ngeoms)
		{
			geoms = (RTGEOM**) rtalloc(sizeof(RTGEOM*) * ngeoms);
			for (i = 0; i < ngeoms; i++)
			{
				const sfcgal_geometry_t* g = sfcgal_triangulated_surface_triangle_n(geom, i);
				geoms[i] = SFCGAL2RTGEOM(g, 0, srid);
			}
		}
		return (RTGEOM*) rtcollection_construct(TINTYPE, srid, NULL, ngeoms, geoms);
	}

	default:
		rterror("SFCGAL2RTGEOM: Unknown Type");
		return NULL;
	}
}


sfcgal_geometry_t*
RTGEOM2SFCGAL(const RTGEOM* geom)
{
	uint32_t i;
	sfcgal_geometry_t* ret_geom = NULL;

	assert(geom);

	switch (geom->type)
	{
	case POINTTYPE:
	{
		const RTPOINT* rtp = (const RTPOINT*) geom;
		if (rtgeom_is_empty(geom)) return sfcgal_point_create();

		return ptarray_to_SFCGAL(rtp->point, POINTTYPE);
	}
	break;

	case LINETYPE:
	{
		const RTLINE* line = (const RTLINE*) geom;
		if (rtgeom_is_empty(geom)) return sfcgal_linestring_create();

		return ptarray_to_SFCGAL(line->points, LINETYPE);
	}
	break;

	case TRIANGLETYPE:
	{
		const RTTRIANGLE* triangle = (const RTTRIANGLE*) geom;
		if (rtgeom_is_empty(geom)) return sfcgal_triangle_create();
		return ptarray_to_SFCGAL(triangle->points, TRIANGLETYPE);
	}
	break;

	case POLYGONTYPE:
	{
		const RTPOLY* poly = (const RTPOLY*) geom;
		uint32_t nrings = poly->nrings - 1;

		if (rtgeom_is_empty(geom)) return sfcgal_polygon_create();

		sfcgal_geometry_t* exterior_ring = ptarray_to_SFCGAL(poly->rings[0], LINETYPE);
		ret_geom = sfcgal_polygon_create_from_exterior_ring(exterior_ring);

		for (i = 0; i < nrings; i++)
		{
			sfcgal_geometry_t* ring = ptarray_to_SFCGAL(poly->rings[i + 1], LINETYPE);
			sfcgal_polygon_add_interior_ring(ret_geom, ring);
		}
		return ret_geom;
	}
	break;

	case MULTIPOINTTYPE:
	case MULTILINETYPE:
	case MULTIPOLYGONTYPE:
	case COLLECTIONTYPE:
	{
		if (geom->type == MULTIPOINTTYPE)        ret_geom = sfcgal_multi_point_create();
		else if (geom->type == MULTILINETYPE)    ret_geom = sfcgal_multi_linestring_create();
		else if (geom->type == MULTIPOLYGONTYPE) ret_geom = sfcgal_multi_polygon_create();
		else                                     ret_geom = sfcgal_geometry_collection_create();

		const RTCOLLECTION* rtc = (const RTCOLLECTION*)geom;
		for (i = 0; i < rtc->ngeoms; i++)
		{
			sfcgal_geometry_t* g = RTGEOM2SFCGAL(rtc->geoms[i]);
			sfcgal_geometry_collection_add_geometry(ret_geom, g);
		}

		return ret_geom;
	}
	break;

	case POLYHEDRALSURFACETYPE:
	{
		const RTPSURFACE* rtp = (const RTPSURFACE*) geom;
		ret_geom = sfcgal_polyhedral_surface_create();

		for (i = 0; i < rtp->ngeoms; i++)
		{
			sfcgal_geometry_t* g = RTGEOM2SFCGAL((const RTGEOM*) rtp->geoms[i]);
			sfcgal_polyhedral_surface_add_polygon(ret_geom, g);
		}
		/* We treat polyhedral surface as the only exterior shell,
		   since we can't distinguish exterior from interior shells ... */
		if (FLAGS_GET_SOLID(rtp->flags))
		{
			return sfcgal_solid_create_from_exterior_shell(ret_geom);
		}

		return ret_geom;
	}
	break;

	case TINTYPE:
	{
		const RTTIN* rtp = (const RTTIN*) geom;
		ret_geom = sfcgal_triangulated_surface_create();

		for (i = 0; i < rtp->ngeoms; i++)
		{
			sfcgal_geometry_t* g = RTGEOM2SFCGAL((const RTGEOM*) rtp->geoms[i]);
			sfcgal_triangulated_surface_add_triangle(ret_geom, g);
		}

		return ret_geom;
	}
	break;

	default:
		rterror("RTGEOM2SFCGAL: Unknown geometry type !");
		return NULL;
	}
}


/*
 * No Operation SFCGAL function, used (only) for cunit tests
 * Take a PostGIS geometry, send it to SFCGAL and return it unchanged (in theory)
 */
RTGEOM* rtgeom_sfcgal_noop(const RTGEOM* geom_in)
{
	sfcgal_geometry_t* converted;

	assert(geom_in);

	converted = RTGEOM2SFCGAL(geom_in);
	assert(converted);

	RTGEOM* geom_out = SFCGAL2RTGEOM(converted, 0, SRID_UNKNOWN);
	sfcgal_geometry_delete(converted);

	/* copy SRID (SFCGAL does not store the SRID) */
	geom_out->srid = geom_in->srid;
	return geom_out;
}
