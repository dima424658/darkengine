// $Header: x:/prj/tech/libsrc/r3d/RCS/clipprim.h 1.10 1998/06/18 11:25:50 JAEMZ Exp $   Internal clipping of primitives

#ifndef _CLIPPRIM_H
#define _CLIPPRIM_H

#include <r3ds.h>

// clip a polygon against the active clip planes
EXTERN int r3_clip_polygon(int n,r3s_phandle *src,r3s_phandle **dest);

// generate an or'd together clipcode of the polygon.
// r3d_ccodes_or & r3d_ccodes_and are updated as well; see below
EXTERN int r3_code_polygon(int n, r3s_phandle *src);

// clip a polygon against the view planes regardless of clip mode
EXTERN int r3_raw_clip_polygon(int n,r3s_phandle *src,r3s_phandle **dest);

// clip a line against the view planes
EXTERN bool r3_clip_line(r3s_phandle *src, r3s_phandle **dest);

// The following values are stuffed, but never cleared, in
// certain cases.  Basically, the user clears them, calls
// one or more functions that cumulatively stuffs them, then
// does whatever with the result.

EXTERN ulong r3d_ccodes_or, r3d_ccodes_and;
// Here are the functions that stuff them:
//
//  r3_ccodes_or:
//     r3_transform_block() in project space only
//     r3_code_polygon()
//    *r3_block_or 
//    *r3_block_or_and
//
//  r3_ccodes_and:
//     r3_code_polygon()
//    *r3_block_and
//    *r3_block_or_and
//
//  Functions marked [*] aren't written yet.


// This routine clips a bounding box in world space. 
// It returns the and_ccode of all the clip codes and
// stuffs the or_code.  Since it returns and_ccode you can use it as a
// branch for trivial reject if non-zero.  
EXTERN int r3_clip_bbox(mxs_vector *bmin,mxs_vector *bmax,int *or_code);


// This routine clips a bouding sphere in world space.
// It returns the and_ccode of all the clip codes and
// stuffs the or_code.  Since it returns and_ccode you can use it as a
// branch for trivial reject if non-zero.  
EXTERN int r3_clip_sphere(mxs_vector *cen,float rad,int *or_code);

// This routine clip codes a point in world space.
// It returns the code
EXTERN int r3_clip_point(mxs_vector *cen);

// why is this here?
EXTERN void r3_compute_camera_planes(void);

#endif






