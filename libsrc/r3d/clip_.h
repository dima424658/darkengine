// $Header: x:/prj/tech/libsrc/r3d/RCS/clip_.h 1.3 1996/05/27 19:51:46 buzzard Exp $

#include <r3ds.h>

  // number of standard codes in ccodes (user planes follow)
#define NUM_STD_CCODES 5

  // number of allowed user planes
#define NUM_USER_PLANES (32 - NUM_STD_CCODES)

extern void r3_std_code_points(int n, r3s_point *src);
