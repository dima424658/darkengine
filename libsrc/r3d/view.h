/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/view.h $
 * $Revision: 1.7 $
 * $Author: JAEMZ $
 * $Date: 1998/06/18 11:27:15 $
 *
 * View point related functions
 *
 */

#ifndef __VIEW_H
#define __VIEW_H

#include <r3ds.h>


// Sets the view position, position only.  Note that orientationless cameras
// and objects all are assumed to be heading==0 pitch==0 bank==0, meaning that
// under the default world space, you all point down the x a Also, 
// o2c_invalid, and w2c_invalid get set.

EXTERN void r3_set_view(mxs_vector *v);
EXTERN mxs_trans *r3_get_view_trans();

// Sets the view transform pretty damn directly.  The meaning of it is the
// position in world space, and the columns of the matrix point along the
// viewer x, y, and z directions.

EXTERN void r3_set_view_trans(mxs_trans *t);

// Takes an angvec for angles about the x,y,z axes, and and order which
// indicates from left to right how the angles should be applied.  There
// is a default, which is XYZ which means bank the thing first, pitch i

EXTERN void r3_set_view_angles(mxs_vector *v, mxs_angvec *a,r3e_order o);

// Gets the view position in world space, and the angles in world space

EXTERN mxs_vector *r3_get_view_pos();
EXTERN mxs_angvec *r3_get_view_ang();

// This gives you the current viewer position in object space.   This
// must be in a block.  Or at least after a block and before any other
// space changing call. 

EXTERN mxs_vector *r3_get_view_in_obj();

// These are always true, don't need to be in or out of a block
// The slew function gets you the slew matrix of the viewer.  That is,
// the first column, would be the vector to move you one unit in the
// viewer x, ie forward, the second column is y, ie, left, the third
// column.

EXTERN mxs_matrix *r3_get_slew_matrix();
EXTERN mxs_vector *r3_get_forward_slew();
EXTERN mxs_vector *r3_get_left_slew();
EXTERN mxs_vector *r3_get_up_slew();

// get the view pyramid in world space
// as a set of planes that get stuffed
// you need space for five planes,
// left,top,right,bottom, front

EXTERN void r3_get_view_pyr_planes(mxs_plane *planes);

// get the view pyramid as 4 vectors
// upper left, upper right, lower right, lower left
// Note that these are deltas in world space off of the viewer position

EXTERN void r3_get_view_pyr_vecs(mxs_vector *vecs);

// Stuff a world space vector into dst describing a vector
// at screen coordinates, sx, and sy.  dst->z is equal to one
// can call at any time during a context
EXTERN void r3_get_view_vec(mxs_vector *dst,mxs_real sx,mxs_real sy);

// get and set zoom factors, defaults to 1.0 = 90 degrees
// need a current context, not in a block

EXTERN void r3_set_zoom(mxs_real zoom);
EXTERN mxs_real r3_get_zoom();

// set and get aspect ratio, since you may want to change
// it, and the 2d has no aspect per canvas
// note aspect ratio is defined as height of pixel over
// width of pixel.

EXTERN void r3_set_aspect(mxs_real aspect);
EXTERN mxs_real r3_get_aspect();

// set and get the current view canvas, this is also called internally
// assumes in context, and not in block.

EXTERN void r3_set_view_canvas(grs_canvas *canv);
EXTERN grs_canvas *r3_get_view_canvas();

#endif // VIEW





