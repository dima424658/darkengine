/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/r3ds.h $
 * $Revision: 1.13 $
 * $Author: KEVIN $
 * $Date: 1998/12/21 16:31:05 $
 *
 * Base structure and type definitions
 *
 */

#ifndef __R3DS_H
#define __R3DS_H
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <grspoint.h>
#include <matrixs.h>

// The big guy himself.  This may change over time
typedef struct _r3s_point {
   mxs_vector p;
   ulong ccodes;
   grs_point grp;
} r3s_point;

// sadly, a grs_point is not large enough to hold RGB or RGBA info:
// here are the point sizes to use to get this functionsality.
// You will then want to cast grp to a g2s_point to set the appropriate values.

#define R3D_RGB_POINT_SIZE (sizeof(r3s_point) + 2*sizeof(mxs_real))
#define R3D_RGBA_POINT_SIZE (R3D_RGB_POINT_SIZE + sizeof(mxs_real))

typedef r3s_point * r3s_phandle;

typedef struct r3ixs_info r3ixs_info;

struct r3ixs_info {
   union {
      ulong value;
      struct {
         short index;
         uchar flags;
         uchar pad;
      };
   };
};

#ifdef __cplusplus
}
#endif
#endif // __R3DS_H