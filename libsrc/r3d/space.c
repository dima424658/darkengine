
/*
 * $Source: x:/prj/tech/libsrc/r3d/RCS/space.c $
 * $Revision: 1.38 $
 * $Author: TOML $
 * $Date: 1997/10/15 12:19:52 $
 *
 * Camera space related functions
 *
 */

#include <dev2d.h>

#include <r3ds.h>
#include <xfrmtab.h>
#include <_xfrm.h>
#include <clipprim.h>
#include <matrix.h>
#include <math.h>
#include <clipcon_.h>

// For the support of the HACK HACK HACK
#include <prim.h>

#define FIX_CONV 65536.0

// globals for fast access from assembly
fix r3d_x_off, r3d_y_off;
fix r3d_c_w, r3d_c_h;
fix r3d_c_w_24_8, r3d_c_h_24_8;
fix r3d_x_off_24_8, r3d_y_off_24_8;
float r3d_x_clip, r3d_y_clip, r3d_near, r3d_fast_z=0.5;

// boy, this really has little excuse to be here, but we wouldn't want to sully
// ctxtguts with any real calculation or information
// Note this is only called once from ctxtguts

void _r3_trans_alloc(r3s_xform_context *trans)
{
   // set from the defaults
   trans->world = r3d_state.world;
   trans->cspace = r3d_state.space;

   // create the object space xform stack
   trans->stack_max = r3d_state.trans_depth;
   trans->stack_base = Malloc(sizeof(r3s_trans)*trans->stack_max);

   // set stack base and current, and set view
   trans->stack_cur = trans->stack_base;
   trans->stack_size = 0;
   mx_identity_trans(X2TRANS(&trans->o2w));

   // initialize view
   mx_identity_trans(X2TRANS(&trans->view_trans));
   trans->view_ang.tx = 0.0;
   trans->view_ang.ty = 0.0;
   trans->view_ang.tz = 0.0;

   // initialize w2v, just like recalc_w2v
   mx_mult_trans(X2TRANS(&(trans->w2v)),X2TRANS(&(trans->world)),X2TRANS(&(trans->view_trans)));

   // default zoom is 90 degrees
   trans->zoom = 1.0;

   // declare everyone invalid.
   trans->w2c_invalid = TRUE;
   trans->o2c_invalid = TRUE;
   trans->stride = r3d_state.stride;
}

// This is so lame.
// Called once from ctxtguts

void _r3_trans_free(r3s_xform_context *trans)
{
   Free(trans->stack_base);
}

// need to recalculate anytime the center changes in world
// space or the world changes or aspect, w,h,zoom
void _r3_recalc_linear()
{
   mxs_vector vcen;   // view cen
   mxs_matrix *lm;
   mxs_real kx,ky;       // projection constants
   mxs_real wcen;  // center w

   TEST_STATE("_r3_recalc_linear");

   // linearize about a point if using lin_center or just immediately set
   // to the center of the screen at the right distance if not
   if (cx.lin_width != 0) {
      // transform world center to view center in world space
      // vcen is screen space about which we get deltas
      vcen.x = 0.0;
      vcen.y = 0.0;
      vcen.z = cx.lin_width/2.0;
   } else {
      mx_trans_mul_vec(&vcen,X2TRANS(&cx.w2v),&cx.lin_cen);
   }

   // set projection constants
   kx = FIX_CONV * cx.zoom * (mxs_real)(cur_canv->bm.w)/2.0;
   ky = FIX_CONV * cx.zoom * (mxs_real)(cur_canv->bm.w)/(2.0 * cx.aspect);

   // set inverse z in cam space
   wcen = cx.lin_w = 1.0/vcen.z;

   // build linear transform
   lm = X2MAT(&cx.lin_trans);

   lm->m0 = kx * wcen;                    // dsx/dx
   lm->m3 = 0;                            // dsx/dy
   lm->m6 = -kx*vcen.x*wcen*wcen;         // dsx/dz

   lm->m1 = 0;                            // dsy/dx
   lm->m4 = ky*wcen  ;                    // dsy/dy
   lm->m7 = -ky*vcen.y*wcen*wcen;         // dsy/dz

   lm->m2 = 0;                            // dsz/dx
   lm->m5 = 0;                            // dsz/dy
   lm->m8 = 0;                            // dsz/dz

   // note that these first two are just the sx and sy at the linearized center
   X2VEC(&cx.lin_trans)->x = kx*vcen.x*wcen + FIX_CONV * cur_canv->bm.w/2.0;
   X2VEC(&cx.lin_trans)->y = ky*vcen.y*wcen + FIX_CONV * cur_canv->bm.h/2.0;
   X2VEC(&cx.lin_trans)->z = vcen.z;   // all z's will be this
}

// recalculate w2v from trans and world tform
void _r3_recalc_w2v()
{
   // concatenate world and inverse view
   mx_mult_trans(X2TRANS(&cx.w2v),X2TRANS(&cx.world),X2TRANS(&cx.view_trans));
   cx.w2c_invalid = TRUE;
   cx.o2c_invalid = TRUE;
}


// recalculate csp and projection constants.
// Note that these depend on:
// space, aspect, w, h, and zoom
// or in short, space, canv, zoom

void _r3_recalc_csp()
{
   TEST_STATE("_r3_recalc_csp");

   #ifdef DBG_ON
      if (r3d_state.cur_con->cnv == NULL) {
         Warning(("r3d:_r3_recalc_csp: current canvas hasn't been set!\n"));
         return;
      }
   #endif

   // Same for all spaces
   // Add in fixed point offset to screen x, can we add in bias here?

   r3d_glob.x_off = FIX_CONV * (cur_canv->bm.w/2.0 -.5);
   r3d_glob.y_off = FIX_CONV * (cur_canv->bm.h/2.0 -.5);

   // set csp
   switch(r3d_glob.cur_space) {
      case R3_CLIPPING_SPACE:
         cx.cspace_vec.x = cx.zoom;
         cx.cspace_vec.y = cx.zoom*(mxs_real)(cur_canv->bm.w)/((mxs_real)(cur_canv->bm.h) * cx.aspect);
         cx.cspace_vec.z = 1.0;

         r3d_glob.x_prj = FIX_CONV * cur_canv->bm.w/2.0;
         r3d_glob.y_prj = FIX_CONV * cur_canv->bm.h/2.0;

         // r3d_glob.x_clip = (FIX_CONV * cur_canv->bm.w / 2.0) / r3d_glob.x_prj

         r3d_glob.x_clip = 1.0;
         r3d_glob.y_clip = 1.0;

         // conversion is
         // (x/z + 1)*x_prj;
         // (y/z + 1)*y_prj;

      break;
      case R3_PROJECT_SPACE:
         cx.cspace_vec.x = FIX_CONV * cx.zoom *(mxs_real)(cur_canv->bm.w)/2.0;
         cx.cspace_vec.y = FIX_CONV * cx.zoom *(mxs_real)(cur_canv->bm.w)/(2.0 * cx.aspect);
         cx.cspace_vec.z = 1.0;

         //   r3d_glob.x_clip = (FIX_CONV * cur_canv->bm.w / 2.0) / r3d_glob.x_prj
         r3d_glob.x_clip = FIX_CONV * cur_canv->bm.w / 2.0;
         r3d_glob.y_clip = FIX_CONV * cur_canv->bm.h / 2.0;

         // conversion is
         // (x/z) + x_off
         // (y/z) + y_off

      break;
      case R3_UNSCALED_SPACE:
         cx.cspace_vec.x = 1.0;
         cx.cspace_vec.y = 1.0;
         cx.cspace_vec.z = 1.0;

         r3d_glob.x_prj = FIX_CONV * cx.zoom * (mxs_real)(cur_canv->bm.w)/2.0;
         r3d_glob.y_prj = FIX_CONV * cx.zoom * (mxs_real)(cur_canv->bm.w)/(2.0 * cx.aspect);

         //   r3d_glob.x_clip = (FIX_CONV * cur_canv->bm.w / 2.0) / r3d_glob.x_prj
         r3d_glob.x_clip = 1.0 / cx.zoom;
         r3d_glob.y_clip = (cx.aspect * (mxs_real) cur_canv->bm.h) / ((mxs_real) cur_canv->bm.w * cx.zoom);

         // conversion is
         // (x_prj*x/z) + x_off;
         // (y_prj*y/z) + y_off;
      break;

      // note this is different in that a simple scaling
      // is not sufficient, we need a whole new transform
      // and in fact, this is not the place to do it.

      case R3_LINEAR_SPACE:
      break;
   }

   // stuff globals used by assembly
   r3d_x_off = r3d_glob.x_off;
   r3d_y_off = r3d_glob.y_off;
   r3d_x_off_24_8 = r3d_glob.x_off >> 8;
   r3d_y_off_24_8 = r3d_glob.y_off >> 8;
   r3d_c_w = fix_make(cur_canv->bm.w,0);
   r3d_c_h = fix_make(cur_canv->bm.h,0);
   r3d_c_w_24_8 = fix_make(cur_canv->bm.w,0) >> 8;
   r3d_c_h_24_8 = fix_make(cur_canv->bm.h,0) >> 8;
   r3d_near = r3d_near_plane;
   r3d_fast_z = 0.5;  // set as low as possible without overflowing
   r3d_x_clip = r3d_glob.x_clip;
   r3d_y_clip = r3d_glob.y_clip;

   cx.o2c_invalid = TRUE;
   cx.w2c_invalid = TRUE;

   // This needs to know the above two thangs are invalid
   r3_compute_camera_planes();
}


// Changes the world coordinate system to the one pointed at.  This is for
// losers who want to change for a given context after it's built, or for doing
// wacky world inversion or maybe mirrors.  The handedness bit

void r3_set_world(mxs_trans *w)
{
   TEST_STATE("r3_set_world");
   mx_copy_trans(X2TRANS(&cx.world),w);
   _r3_recalc_w2v();
   _r3_recalc_world_pyr();
}

mxs_trans *r3_get_world()
{
   TEST_IN_CONTEXT("r3_get_world");
   return X2TRANS(&cx.world);
}


// sets the linear space center in world space
// can call anytime you have a context
void r3_set_linear_center(mxs_vector *v)
{
   TEST_IN_CONTEXT("r3_get_world");

   cx.lin_cen = *v;
   cx.lin_width = 0;
   cx.w2c_invalid = TRUE;
}


// Routine to set the linear space projected from the center of the
// screen making the screen width be world space size w
// used for top down or anything like that view
void r3_set_linear_screen_width(mxs_real w)
{
   TEST_IN_CONTEXT("r3_get_world");

   cx.lin_width = w;
   cx.w2c_invalid = TRUE;
}


// given a center and radius, say how accurate it is, ie, how many pixels
// different is the front to the back?
// All this is is (kx*r*(1/(zcen + r) - 1/(zcen -r)))
// takes a wcen in world coords, and rad in world real
mxs_real r3_linear_accuracy(mxs_vector *wcen,mxs_real rad)
{
   mxs_real kx;    // projection factor for sx
   mxs_vector vcen; // center in cam space

   TEST_IN_CONTEXT("r3_lin_accuracy");

   // notice, don't use FIX_CONV in this case
   kx = cx.zoom * (mxs_real)(cur_canv->bm.w)/2.0;

   // transform world center to view center in world space
   // vcen is screen space about which we get deltas

   mx_trans_mul_vec(&vcen,X2TRANS(&cx.w2v),wcen);

   return kx * rad * (1.0/(vcen.z - rad) - 1.0/(vcen.z + rad));
}


// get the pixel size of something x wide at z distance from the camera,
// assumes it's centered, h means horizontal.  You can get z from a
// transformed points z, since we're so groovy and don't scale it
mxs_real r3_get_hsize(mxs_real z,mxs_real h)
{
   return h*(mxs_real)(cur_canv->bm.w)*cx.zoom/(2.0*z);
}

mxs_real r3_get_vsize(mxs_real z,mxs_real v)
{
   return v*(mxs_real)(cur_canv->bm.w)*cx.zoom/(2.0*z*cx.aspect);
}

// Return the scaling factor with which you get screen size.  You multiply
// by height and divide by distance to get pixel size
mxs_real r3_get_hscale()
{
   return (mxs_real)(cur_canv->bm.w)*cx.zoom/(2.0);
}

mxs_real r3_get_vscale()
{
   return (mxs_real)(cur_canv->bm.w)*cx.zoom/(2.0*cx.aspect);
}



// Unproject points into world space, just give it screen coords
// and the distance at which you want it from the camera
void r3_unproject(mxs_vector *v,float z,float sx,float sy)
{
   v->x = z;

   v->y = - 2.0 * z * (sx - (r3d_glob.x_off/FIX_CONV))/(cx.zoom * (mxs_real)(cur_canv->bm.w));
   v->z = - 2.0 * cx.aspect * z * (sy - (r3d_glob.y_off/FIX_CONV))/(cx.zoom * (mxs_real)(cur_canv->bm.w));
}



// Takes field of view in degrees, and the screen width in pixels
mxs_real r3_fov_2_zoom(mxs_real degrees)
{
   return 1.0/tan(MX_REAL_PI*degrees/360);
}


// Returns necessary zoom to get the number of pixels for that
// width and distance.  Basically opposite of r3_get_hsize
// z and w are in world coords, pixw is in screen pixels
mxs_real r3_hsize_2_zoom(mxs_real z,mxs_real w,mxs_real pixw)
{
   return (2.0 * z * pixw)/(w*(mxs_real)(cur_canv->bm.w));
}


// Changes all the function pointers for xform and clipping, sets cspace and
// cspace_vec, and sets o2c_invalid, and w2c_invalid.

// If linear, turns off 3d clipping and turns on 2d clipping for now.
// this is a HACK, HACK HACK HACK, and should be remedied by making the
// 3d clipper in this mode BE the 2d clipper.
// because otherwise, it will break eventually.  All you'd have to do
// is turn on 3d clipping in linear, and it will break

static bool r3d_sucks=FALSE;

void r3_set_space(r3e_space s)
{
   TEST_STATE("r3_set_space");

   #ifdef DBG_ON
   if (s<0 || s>=R3_NUM_SPACE) {
      Warning(("r3_set_space: space out of range\n"));
      return;
   }
   #endif

   // return if nothing to do
   if (s == r3d_glob.cur_space) return;

   // copy xform function tabs
   r3d_glob.xform_tab = *r3d_xform_ftabs[s];

   // copy clipping function tabs
   // set_the_clipping_tables();

   r3d_glob.cur_space = s;

   // HACK HACK HACK
   if (s == R3_LINEAR_SPACE) {
      if (!r3d_sucks) {
         r3_set_2d_clip(TRUE);
         r3_set_clipmode(R3_NO_CLIP);
         r3d_sucks = TRUE;
      }
   } else {
      if (r3d_sucks) {
         r3_set_2d_clip(FALSE);
         r3_set_clipmode(R3_CLIP);
         r3d_sucks = FALSE;
      }
   }

   // recalculate the csp given zoom and space and canvas
   _r3_recalc_csp();
}

r3e_space r3_get_space()
{
   TEST_IN_CONTEXT("r3_get_space");
   return r3d_glob.cur_space;
}

// Let's the user sneakily change the space for their own custom
// transform wackiness.   Note that this does not do anything to the
// clippers or any of that, so they better set it back again.

void r3_set_space_vector(mxs_vector *v)
{
   TEST_STATE("r3_set_space_vector");

   mx_copy_vec(&cx.cspace_vec,v);

   cx.o2c_invalid = TRUE;
   cx.w2c_invalid = TRUE;
}

mxs_vector *r3_get_space_vector()
{
   TEST_IN_CONTEXT("r3_get_space_vector");
   return &cx.cspace_vec;
}


// Transform a plane from world space to world space, preserving dot product
// equals zero.  Assumes w2v is unitary, but the csp vector doesn't have to
// be
// Also, you must be outside of a block
void r3_w2c_plane(mxs_plane *dst,mxs_plane *src)
{
   // be in a context
   TEST_IN_CONTEXT("r3_w2c_plane");

   if (r3d_glob.cur_space != R3_LINEAR_SPACE) {
      // first transform the vector portion of the plane into the unitary
      // version of it.  Only muliply by the matrix, cause its a vector,
      // not a point
      mx_mat_mul_vec(&(dst->v),X2MAT(&cx.w2v),&(src->v));

      // now multiply by the inverse of csp
      // This should be a matrix op
      dst->x /= cx.cspace_vec.x;
      dst->y /= cx.cspace_vec.y;
      dst->z /= cx.cspace_vec.z;

      // need w2c, so calucate if necessary.
      if (cx.w2c_invalid) {
         cx.w2c = cx.w2v;
         mx_mat_eltmuleq_vec(X2MAT(&cx.w2c),&cx.cspace_vec);
         mx_elmuleq_vec(X2VEC(&cx.w2c),&cx.cspace_vec);
         cx.w2c_invalid = FALSE;
      }
      dst->d = src->d - mx_dot_vec(&(dst->v),X2VEC(&cx.w2c));
   }
}



// Returns the mag2 of a rotated/tranformed point
// ie, a point in camera space
float r3_camera_mag2(r3s_point *p)
{
   mxs_vector v;

   v.x =  p->p.x/cx.cspace_vec.x;
   v.y =  p->p.y/cx.cspace_vec.y;
   v.z =  p->p.z/cx.cspace_vec.z;
   return mx_mag2_vec(&v);
}

