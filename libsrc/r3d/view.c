/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/view.c $
 * $Revision: 1.23 $
 * $Author: KEVIN $
 * $Date: 1997/10/01 13:33:25 $
 *
 * View point related functions
 *
 */

#include <matrix.h>
#include <dev2d.h>

#include <r3ds.h>
#include <_xfrm.h>
#include <view.h>

#define FIRST_ANG(order)  (((order) & 0x30)>>4)
#define SECOND_ANG(order) (((order) & 0x0C)>>2)
#define THIRD_ANG(order)   ((order) & 0x03)



// recalculates the world pyramids.  Note that this
// must be recalculated anytime the world coords change
// or the aspect ratio, zoom, or view position or
// orientation

void _r3_recalc_world_pyr()
{
   r3_get_view_pyr_planes((r3d_state.cur_con->clip.world_clip));
}

// Sets the view position, position only.  Note that orientationless cameras
// and objects all are assumed to be heading==0 pitch==0 bank==0, meaning that
// under the default world space, you all point down the x a Also, 
// o2c_invalid, and w2c_invalid get set.

// the start of frame resets these to zero and identity
// cx is current transform context
// X2VEC gets you the vector address from a r3s_trans

// immediately conctatenate with world to get world2view
void r3_set_view(mxs_vector *v)
{
   TEST_STATE("r3_set_view");

   mx_copy_vec(X2VEC(&cx.view_trans),v);
   _r3_recalc_w2v();
   _r3_recalc_world_pyr();
}

mxs_trans *r3_get_view_trans()
{
   TEST_IN_CONTEXT("r3_get_view_trans");

   return X2TRANS(&cx.view_trans);
}

// Sets the view transform pretty damn directly.  The meaning of it is the
// position in world space, and the columns of the matrix point along the
// viewer x, y, and z directions.

void r3_set_view_trans(mxs_trans *t)
{
   TEST_STATE("r3_set_view_trans");

   mx_copy_trans(X2TRANS(&cx.view_trans),t);
   _r3_recalc_w2v();
   _r3_recalc_world_pyr();
}


void _r3_concat_matrix(mxs_matrix *m, mxs_angvec *a,r3e_order o)
{
   mxs_matrix m1,m2;

   switch (THIRD_ANG(o)) {
      case 0: mx_mk_rot_x_mat(&m1,a->tx); break;
      case 1: mx_mk_rot_y_mat(&m1,a->ty); break;
      case 2: mx_mk_rot_z_mat(&m1,a->tz); break;
   }

   switch (SECOND_ANG(o)) {
      case 0: mx_rot_x_mat(&m2,&m1,a->tx); break;
      case 1: mx_rot_y_mat(&m2,&m1,a->ty); break;
      case 2: mx_rot_z_mat(&m2,&m1,a->tz); break;
   }

   switch (FIRST_ANG(o)) {
      case 0: mx_rot_x_mat(m,&m2,a->tx); break;
      case 1: mx_rot_y_mat(m,&m2,a->ty); break;
      case 2: mx_rot_z_mat(m,&m2,a->tz); break;
   }
}


// Takes an angvec for angles about the x,y,z axes, and and order which
// indicates from left to right how the angles should be applied.  There
// is a default, which is XYZ which means bank the thing first, pitch i

void r3_set_view_angles(mxs_vector *v, mxs_angvec *a,r3e_order o)
{
   mxs_matrix *m = X2MAT(&cx.view_trans);

   TEST_STATE("r3_set_view_angles");

   cx.view_ang.tx = a->tx;
   cx.view_ang.ty = a->ty;
   cx.view_ang.tz = a->tz;
                       
   mx_copy_vec(X2VEC(&cx.view_trans),v);

   _r3_concat_matrix(m,a,o);
   _r3_recalc_w2v();
   _r3_recalc_world_pyr();
}

// Gets the view position in world space, and the angles in world space

mxs_vector *r3_get_view_pos()
{
   TEST_IN_CONTEXT("r3_get_view_pos");

   return X2VEC(&cx.view_trans);
}

mxs_angvec *r3_get_view_ang()
{
   TEST_IN_CONTEXT("r3_get_view_ang");

   return &cx.view_ang;
}

// This gives you the current viewer position in object space.   This
// must be in a block.  Or at least after a block and before any other
// space changing call. 

mxs_vector *r3_get_view_in_obj()
{
   TEST_IN_CONTEXT("r3_get_view_in_obj");
   return &cx.vino;
}

// The slew function gets you the slew matrix of the viewer.  That is,
// the first column, would be the vector to move you one unit in the
// viewer x, ie forward, the second column is y, ie, left, the third
// column is z, ie, up
// but don't mess with it.

mxs_matrix *r3_get_slew_matrix()
{
   TEST_IN_CONTEXT("r3_get_slew_matrix");
   return X2MAT(&cx.view_trans);
}

mxs_vector *r3_get_forward_slew()
{
   TEST_IN_CONTEXT("r3_get_forward_slew");
   return &(X2MAT(&cx.view_trans)->vec[0]);
}

mxs_vector *r3_get_left_slew()
{
   TEST_IN_CONTEXT("r3_get_left_slew");
   return &(X2MAT(&cx.view_trans)->vec[1]);
}

mxs_vector *r3_get_up_slew()
{
   TEST_IN_CONTEXT("r3_get_up_slew");
   return &(X2MAT(&cx.view_trans)->vec[2]);
}

// You know, this is stored in world space all the time.  Hmmm.

// get the view pyramid in world space
// as a set of planes that get stuffed
// you need space for five planes,
// left,top,right,bottom,front

// The plane normals are normalized.

// Does this work if you set CSP yourself?  I think not.  Hmm.

void r3_get_view_pyr_planes(mxs_plane *planes)
{
   mxs_vector vec[4];
   mxs_vector tmp;   // points in to the center

   TEST_IN_CONTEXT("r3_get_view_pyr_planes");

   // get the corner vectors
   r3_get_view_pyr_vecs(vec);

   // take the cross products of each one, and they all go through the
   // viewer.

   // Left plane
   mx_cross_norm_vec(&(planes[0].v),&vec[3],&vec[0]);
   planes[0].d = -mx_dot_vec(&(planes[0].v),X2VEC(&cx.view_trans));

   // right plane
   mx_cross_norm_vec(&(planes[1].v),&vec[1],&vec[2]);
   planes[1].d = -mx_dot_vec(&(planes[1].v),X2VEC(&cx.view_trans));

   // top plane
   mx_cross_norm_vec(&(planes[2].v),&vec[0],&vec[1]);
   planes[2].d = -mx_dot_vec(&(planes[2].v),X2VEC(&cx.view_trans));

   // bottom plane
   mx_cross_norm_vec(&(planes[3].v),&vec[2],&vec[3]);
   planes[3].d = -mx_dot_vec(&(planes[3].v),X2VEC(&cx.view_trans));

   // Near plane
   // Get the forward vector, just the bottom row of the matrix
   // this is already normalized
   tmp.x = X2MAT(&cx.w2v)->m2;
   tmp.y = X2MAT(&cx.w2v)->m5;
   tmp.z = X2MAT(&cx.w2v)->m8;

   planes[4].v = tmp;
   planes[4].d = -mx_dot_vec(&tmp,X2VEC(&cx.view_trans));
   // adjust for r3d_state.cur_con->clip.near_plane
   // Note that the mag2 of vec is probably simply 1.  But hey.  Just in case.
   planes[4].d -= mx_dot_vec(&tmp,&tmp) * r3d_state.cur_con->clip.near_plane;
}

// get the view pyramid as 4 vectors
// upper left, upper right, lower right, lower left
// note that these are deltas from the view pos

void r3_get_view_pyr_vecs(mxs_vector *vecs)
{
   mxs_real xleft,xright,ytop,ybot;
   mxs_vector tmp[4];
   int i;

   TEST_IN_CONTEXT("r3_get_view_pyr_vecs");

   // These in view space, not bad, only divides
   // the z's are all one
   xright = 1.0/cx.zoom;
   xleft  = -xright;

   ybot = (cur_canv->bm.h*cx.aspect)/(cur_canv->bm.w*cx.zoom) ;
   ytop = -ybot;

   tmp[0].x = xleft; tmp[0].y = ytop; tmp[0].z = 1.0;
   tmp[1].x = xright; tmp[1].y = ytop; tmp[1].z = 1.0;
   tmp[2].x = xright; tmp[2].y = ybot; tmp[2].z = 1.0;
   tmp[3].x = xleft; tmp[3].y = ybot; tmp[3].z = 1.0;

   // stuff vecs with the results
   for (i=0;i<4;++i) {
      mx_mat_tmul_vec(&vecs[i],X2MAT(&cx.w2v),&tmp[i]);
   }

}


// Stuff a world space vector into dst describing a vector
// at screen coordinates, sx, and sy.  dst->z is equal to one
// can call at any time during a context
void r3_get_view_vec(mxs_vector *dst,mxs_real sx,mxs_real sy)
{
   mxs_vector tmp;

   TEST_IN_CONTEXT("r3_get_view_vec");

   tmp.x = (2*sx/(cur_canv->bm.w) - 1.0)/cx.zoom;
   tmp.y = cx.aspect * (2*sy - cur_canv->bm.h)/(cur_canv->bm.w*cx.zoom);
   tmp.z = 1.0;

   mx_mat_tmul_vec(dst,X2MAT(&cx.w2v),&tmp);
}


// set and get zoom

void r3_set_zoom(mxs_real zoom)
{
   TEST_STATE("r3_set_zoom");

   cx.zoom = zoom;
   _r3_recalc_csp();
   _r3_recalc_world_pyr();
}

mxs_real r3_get_zoom()
{
   TEST_IN_CONTEXT("r3_get_zoom");

   return cx.zoom;
}

// set and get aspect ratio to override 2d default

void r3_set_aspect(mxs_real aspect)
{
   TEST_STATE("r3_set_aspect");
   cx.aspect = aspect;

   _r3_recalc_csp();
   _r3_recalc_world_pyr();
}


mxs_real r3_get_aspect()
{
   TEST_IN_CONTEXT("r3_get_aspect");
   return cx.aspect;
}

// set and get the current view canvas, this is also called internally
// assumes in context, and not in block.

void r3_set_view_canvas(grs_canvas *canv)
{
   TEST_STATE("r3_set_view_canvas");
   cur_canv = canv;

   // set the current 2d aspect ratio, which you probably want, but can override
   cx.aspect = (mxs_real)fix_float(grd_cap->aspect);

   // christ, since canvii don't have aspect ratio, this could be zero if the 2d 
   // has not had a mode set and all that jazz
   if (cx.aspect == 0) cx.aspect = 1.0;

   _r3_recalc_csp();
   _r3_recalc_world_pyr();
}

grs_canvas *r3_get_view_canvas()
{
   TEST_IN_CONTEXT("r3_get_view_canvas");
   return cur_canv;
}
