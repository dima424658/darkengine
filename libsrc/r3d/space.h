// $Header: x:/prj/tech/libsrc/r3d/RCS/space.h 1.15 1998/06/18 11:27:14 JAEMZ Exp $
// Camera Space related functions

#ifndef __SPACE_H
#define __SPACE_H

#include <r3ds.h>
#include <ctxts.h>

// Changes the world coordinate system to the one pointed at.  This is for
// losers who want to change for a given context after its built, or for doing
// wacky world inversion or maybe mirrors.  The handedness bit

EXTERN void r3_set_world(mxs_trans *w);
EXTERN mxs_trans *r3_get_world();

// Changes all the function pointers for xform and clipping, sets cspace and
// cspace_vec, and sets o2c_invalid, and w2c_invalid.  

EXTERN void r3_set_space(r3e_space s);
EXTERN r3e_space r3_get_space();

// Let's the user sneakily change the space for their own custom
// transform wackiness.   Note that this does not do anything to the
// clippers or any of that, so they better set it back again.

EXTERN void r3_set_space_vector(mxs_vector *v);
EXTERN mxs_vector *r3_get_space_vector();

// Routine to set the linear space mode center in world space
EXTERN void r3_set_linear_center(mxs_vector *v);

// Routine to set the linear space projected from the center of the
// screen making the screen width be world space size w
// used for top down or anything like that view
EXTERN void r3_set_linear_screen_width(mxs_real w);

// get the pixel size of something x wide at z distance from the camera,
// assumes it's centered, h means horizontal.  You can get z from a 
// transformed points z, since we're so groovy and don't scale it
// Useful for LOD calculations
EXTERN mxs_real r3_get_hsize(mxs_real z,mxs_real h);
EXTERN mxs_real r3_get_vsize(mxs_real z,mxs_real v);


// Return the scaling factor with which you get screen size.  You multiply
// by height and divide by distance to get pixel size
// Useful for LOD calculations
EXTERN mxs_real r3_get_hscale();
EXTERN mxs_real r3_get_vscale();


// Unproject points into camera space, just give it screen coords
// and the distance at which you want it from the camera
EXTERN void r3_unproject(mxs_vector *v,float z,float sx,float sy);


// Takes field of view in degrees, and the screen width in pixels
EXTERN mxs_real r3_fov_2_zoom(mxs_real degrees);

// Returns necessary zoom to get the number of pixels for that 
// width and distance.  Basically opposite of r3_get_hsize
// z and w are in world coords, pixw is in screen pixels
EXTERN mxs_real r3_hsize_2_zoom(mxs_real z,mxs_real w,mxs_real pixw);


// Get the pixel accuracy of linear space about a world point
// and radius.  This represents worst possible popping in screen
// pixels
EXTERN mxs_real r3_linear_accuracy(mxs_vector *wcen,mxs_real rad);

// Transform a plane from world space to world space, preserving dot product
// equals zero.  Assumes w2v is unitary, but the csp vector doesn't have to 
// be
// Also, you must be outside of a block
EXTERN void r3_w2c_plane(mxs_plane *dst,mxs_plane *src);

// Returns the mag2 of a rotated/tranformed point
// ie, a point in camera space
EXTERN float r3_camera_mag2(r3s_point *p);


#endif // SPACE



