//
//
//

#ifndef MPUTIL_H
#define MPUTIL_H

//

#include <stdlib.h>		// min() and max().

// I hate macros.
#define clamp(x, lower, upper)	min(max(x, lower), upper)

void build_ramp(float * ramp, int size);

//

#endif



