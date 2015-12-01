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


#include "librtgeom_internal.h"
#include <SFCGAL/capi/sfcgal_c.h>


/* return SFCGAL version string */
const char*
rtgeom_sfcgal_version(void);

/* Convert SFCGAL structure to rtgeom PostGIS */
RTGEOM*
SFCGAL2RTGEOM(const sfcgal_geometry_t* geom, int force3D, int SRID);

/* Convert rtgeom PostGIS to SFCGAL structure */
sfcgal_geometry_t*
RTGEOM2SFCGAL(const RTGEOM* geom);

/* No Operation SFCGAL function, used (only) for cunit tests
 * Take a PostGIS geometry, send it to SFCGAL and return it unchanged
 */ 
RTGEOM*
rtgeom_sfcgal_noop(const RTGEOM* geom_in);
