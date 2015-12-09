#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "librtgeom_internal.h"

#ifndef EPSILON
#define EPSILON        1.0E-06
#endif
#ifndef FPeq
#define FPeq(A,B)     (fabs((A) - (B)) <= EPSILON)
#endif



RTGBOX *
box2d_clone(const RTGBOX *in)
{
	RTGBOX *ret = rtalloc(sizeof(RTGBOX));
	memcpy(ret, in, sizeof(RTGBOX));
	return ret;
}
