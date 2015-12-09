/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 * Copyright 2014 Nicklas Avén
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/
 #ifndef _EFFECTIVEAREA_H
#define _EFFECTIVEAREA_H 1


 #include "librtgeom_internal.h"
 #include "rtgeom_log.h"


/**

This structure is placed in an array with one member per point. 
It has links into the minheap rtee and kepps track of eliminated points
*/
typedef struct
{
	double area;
	int treeindex;
	int prev;
	int next;
} areanode;


/**

This structure holds a minheap tree that is used to keep track of what points that has the smallest effective area.
When elliminating points the neighbor points has its effective area affected and the minheap helps to resort efficient.
*/
typedef struct
{		
	int maxSize;
	int usedSize;
	areanode **key_array;
} MINHEAP;


/**

Structure to hold pointarray and it's arealist
*/
typedef struct
{
	const RTPOINTARRAY *inpts;
	areanode *initial_arealist;
	double *res_arealist;
} EFFECTIVE_AREAS;


EFFECTIVE_AREAS* initiate_effectivearea(const RTPOINTARRAY *inpts);

void destroy_effectivearea(EFFECTIVE_AREAS *ea);

void ptarray_calc_areas(EFFECTIVE_AREAS *ea,int avoid_collaps, int set_area, double trshld);

#endif /* _EFFECTIVEAREA_H */
