/**********************************************************************
 *
 * rttopo - topology library
 * http://gitlab.com/rttopo/rttopo
 *
 * Copyright (C) 2015 Sandro Santilli <strk@keybit.net>
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU General Public Licence. See the COPYING file.
 *
 **********************************************************************/

#ifndef LIBRTGEOM_TOPO_INTERNAL_H
#define LIBRTGEOM_TOPO_INTERNAL_H 1

#include "rttopo_config.h"

#include "librtgeom.h"
#include "librtgeom_topo.h"

/************************************************************************
 *
 * Generic SQL handler
 *
 ************************************************************************/

struct RTT_BE_IFACE_T
{
  const RTT_BE_DATA *data;
  const RTT_BE_CALLBACKS *cb;
  const RTCTX *ctx;
};

const char* rtt_be_lastErrorMessage(const RTT_BE_IFACE* be);

RTT_BE_TOPOLOGY * rtt_be_loadTopologyByName(RTT_BE_IFACE *be, const char *name);

int rtt_be_freeTopology(RTT_TOPOLOGY *topo);

RTT_ISO_NODE* rtt_be_getNodeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt, double dist, int* numelems, int fields, int limit);

RTT_ISO_NODE* rtt_be_getNodeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids, int* numelems, int fields);

int rtt_be_ExistsCoincidentNode(RTT_TOPOLOGY* topo, RTPOINT* pt);
int rtt_be_insertNodes(RTT_TOPOLOGY* topo, RTT_ISO_NODE* node, int numelems);

int rtt_be_ExistsEdgeIntersectingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt);

RTT_ELEMID rtt_be_getNextEdgeId(RTT_TOPOLOGY* topo);
RTT_ISO_EDGE* rtt_be_getEdgeById(RTT_TOPOLOGY* topo, const RTT_ELEMID* ids,
                               int* numelems, int fields);
RTT_ISO_EDGE* rtt_be_getEdgeWithinDistance2D(RTT_TOPOLOGY* topo, RTPOINT* pt,
                               double dist, int* numelems, int fields,
                               int limit);
int
rtt_be_insertEdges(RTT_TOPOLOGY* topo, RTT_ISO_EDGE* edge, int numelems);
int
rtt_be_updateEdges(RTT_TOPOLOGY* topo, const RTT_ISO_EDGE* sel_edge, int sel_fields, const RTT_ISO_EDGE* upd_edge, int upd_fields, const RTT_ISO_EDGE* exc_edge, int exc_fields);
int
rtt_be_deleteEdges(RTT_TOPOLOGY* topo, const RTT_ISO_EDGE* sel_edge, int sel_fields);

RTT_ELEMID rtt_be_getFaceContainingPoint(RTT_TOPOLOGY* topo, RTPOINT* pt);

int rtt_be_updateTopoGeomEdgeSplit(RTT_TOPOLOGY* topo, RTT_ELEMID split_edge, RTT_ELEMID new_edge1, RTT_ELEMID new_edge2);


/************************************************************************
 *
 * Internal objects
 *
 ************************************************************************/

struct RTT_TOPOLOGY_T
{
  const RTT_BE_IFACE *be_iface;
  RTT_BE_TOPOLOGY *be_topo;
  int srid;
  double precision;
  int hasZ;
};

#endif /* LIBRTGEOM_TOPO_INTERNAL_H */
