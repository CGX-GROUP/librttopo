
#ifndef _RTGEODETIC_TREE_H
#define _RTGEODETIC_TREE_H 1

#include "rtgeodetic.h"

#define CIRC_NODE_SIZE 8

/**
* Note that p1 and p2 are pointers into an independent RTPOINTARRAY, do not free them.
*/
typedef struct circ_node
{
	GEOGRAPHIC_POINT center;
	double radius;
	int num_nodes;
	struct circ_node** nodes;
	int edge_num;
    int geom_type;
    RTPOINT2D pt_outside;
	RTPOINT2D* p1;
	RTPOINT2D* p2;
} CIRC_NODE;

void circ_tree_print(const CIRC_NODE* node, int depth);
CIRC_NODE* circ_tree_new(const RTPOINTARRAY* pa);
void circ_tree_free(CIRC_NODE* node);
int circ_tree_contains_point(const CIRC_NODE* node, const RTPOINT2D* pt, const RTPOINT2D* pt_outside, int* on_boundary);
double circ_tree_distance_tree(const CIRC_NODE* n1, const CIRC_NODE* n2, const SPHEROID *spheroid, double threshold);
CIRC_NODE* rtgeom_calculate_circ_tree(const RTGEOM* rtgeom);
int circ_tree_get_point(const CIRC_NODE* node, RTPOINT2D* pt);

#endif /* _RTGEODETIC_TREE_H */


