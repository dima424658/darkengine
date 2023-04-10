/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/r3dutil.h $
 * $Revision: 1.4 $
 * $Author: jaemz $
 * $Date: 1998/11/18 17:25:29 $
 *
 * View point related functions
 *
 */

#ifndef __UTILS_H
#define __UTILS_H
#pragma once

#include <matrixs.h>

// Render a wire frame of radius rad.
// The caller is responsible for setting
// object space and color and block
EXTERN void r3_wire_cube(float rad);

// Draws a vector at position pos, and extending towards vec.
// You can scale it and set the maximum size of the vector
// The user is responsible for setting color and block and frame
EXTERN void r3_render_vector(mxs_vector pos,mxs_vector vec,float scale,float maxv);

// The caller is responsible for all that jazz
// even setting object space and color and block
EXTERN void r3_wire_sphere(float rad);

// The caller is responsible for all that jazz
// even setting object space and color and block
// h is height of cylinder
EXTERN void r3_wire_cylinder(float hrad,float h);

// Pass it a bounding box
EXTERN void r3_wire_bbox(mxs_vector *bmin,mxs_vector *bmax);

// Render a wire frame plane with radius
EXTERN void r3_wire_plane(float rad);


#endif // UTILS