/*
 * $Source: x:/prj/tech/libsrc/md/RCS/md.h $
 * $Revision: 1.25 $
 * $Author: buzzard $
 * $Date: 1998/06/30 18:56:11 $
 *
 * Model Library prototypes
 *
 */


#ifndef __MD_H
#define __MD_H
#pragma once

#include <mds.h>

#include <r3ds.h>

// Normal way to render a model.  Pass in pointer to the model and parms
// list
EXTERN void md_render_model(mds_model *m,mds_parm parms[]);

// Call this if you want to render the model from a vcall
EXTERN void md_render_vcall(mds_model *m);

// Given a model, returns the needed size of the buffer for it
EXTERN int md_buffsize(mds_model *m);

// Use this buffer for the next model rendered
EXTERN void md_set_buff(mds_model *m,void *buff);

// Only transforms points, polygon normals, and lighting values
// into the buffer.  Does not render.
EXTERN void md_transform_only(mds_model *m,mds_parm parms[]);

// Only render the model, assumes it has been transformed, and in fact,
// only works then.
EXTERN void md_render_only(mds_model *m,mds_parm parms[]);

// Renders only a subobject, nothing else. 
EXTERN void md_render_subobj(mds_model *m,mds_parm parms[],int sub);

// Return a pointer to the material list.
#define md_mat_list(m) ((mds_mat *)((uchar *)(m)+((m)->mat_off)))

// Return a point to the subobject list
#define md_subobj_list(m) ((mds_subobj *)((uchar *)(m)+((m)->subobj_off)))

// Return a pointer to the vhot list
#define md_vhot_list(m) ((mds_vhot*)((uchar *)(m)+((m)->vhot_off)))

// Return a pointer to the light list
#define md_light_list(m) ((mds_light*)((uchar *)(m)+((m)->light_off)))

// Return a pointer to the point list
#define md_point_list(m) ((mxs_vector*)((uchar *)(m)+((m)->point_off)))

// Return a pointer to the normal list
#define md_norm_list(m) ((mxs_vector*)((uchar *)m+m->norm_off))

// Just evaluate the vhots, stuffing them into their positions.
EXTERN void md_eval_vhots(mds_model *m,mds_parm parms[]);

// These tables are for the vcolors, textures, vcalls, and for reading the result of vhots.  The vhots are in 
// world coords.  You set the vcolor to the color index (if 8 bit) or color (if 16 bit or higher).  You set the 
// vtext tab to the handle of the texture.

#define MD_TAB_SIZE 128

// What is a vcall?
typedef void (* mdf_vcall)(mds_node_vcall *v);

EXTERN int         *mdd_vcolor_tab;
EXTERN r3s_texture *mdd_vtext_tab;
EXTERN mdf_vcall   *mdd_vcall_tab;
EXTERN mxs_vector  *mdd_vhot_tab;

#define md_set_table_entry(index, val, table) \
do {                                                     \
   int __mdmac_index = (index);                          \
   AssertMsg1(((__mdmac_index < MD_TAB_SIZE)&&           \
               (__mdmac_index >= 0)),                    \
            "MD library table index out of range: %i\n", \
            __mdmac_index);                              \
   table[__mdmac_index] = (val);                         \
} while (0)

#define md_set_vtext(index, val)  \
         md_set_table_entry(index, val, mdd_vtext_tab)

#define md_set_vcolor(index, val) \
         md_set_table_entry(index, val, mdd_vcolor_tab)

#define md_set_vcall(index, val)  \
         md_set_table_entry(index, val, mdd_vcall_tab)

#define md_set_vhot(index, val)   \
         md_set_table_entry(index, val, mdd_vhot_tab)

// Typedef defining a function to render a polygon.  This is used as 
// the callback type.

typedef void (* mdf_pgon_cback)(mds_pgon *pgon);

// Set the polygon callback function.  Gets reset at the end of the
// next rendering.  Returns current one so you can chain if you want.
EXTERN mdf_pgon_cback md_set_pgon_callback(mdf_pgon_cback func);

typedef void (* mdf_render_pgon_cback)(mds_pgon *p, r3s_phandle *vlist,
      grs_bitmap *bm, ulong color, ulong type);

// Set a lower-level polygon rendering callback.  This gets passed
// in most of the stuff that's passed to the r3d.  It returns the
// current *mdf_pgon_cback* which you can chain.
EXTERN mdf_pgon_cback md_set_render_pgon_callback(mdf_render_pgon_cback func);

// Exposed so people doing pgon callbacks
// can pre or post render
EXTERN void md_render_pgon(mds_pgon *p);

// By default is set true, you can set it false here, and change the 
// render order for doing craziness like span 
// clipping, and potentially Z buffer.
EXTERN void md_render_back_to_front(bool f);

// By default is set false.  If set true, evaluates a bounding sphere 
// at each node to determine if the node needs
// to be rendered or not.
EXTERN void md_check_bounding_sphere(bool f);

// Converts a normal to a compressed light value
EXTERN void md_norm2light(ulong *l,mxs_vector *n);

// set the render mode
enum {
   MD_RMODE_NORMAL,
   MD_RMODE_SOLID,
   MD_RMODE_WIRE
};

// render normally, as solid, or wire
// let's you change the primitive
// Also has the effect of eliminating color setting
EXTERN void md_set_render_prim(ubyte mode);

// to light, or not to light.
EXTERN void md_set_render_light(bool l);


EXTERN bool       mdd_rgb_lighting;

// Sets the per subobject callback and returns the old one.
mdf_subobj_cback md_set_subobj_callback(mdf_subobj_cback);

// Sets the light callback, and gets the old one back
mdf_light_setup_cback md_set_light_setup_callback(mdf_light_setup_cback);
mdf_light_cback md_set_light_callback(mdf_light_cback);
mdf_light_obj_cback md_set_light_obj_callback(mdf_light_obj_cback);

// This is the default implementaton of lighting, just
// call md_light_init() to install, and use it
// like the old 3d library lighting

#define MD_LT_NONE    0
#define MD_LT_AMB     1
#define MD_LT_DIFF    2
#define MD_LT_SPEC    4

// Set the light type
EXTERN void md_light_set_type(uchar type);

// Recomputes constants based on type of lighting
// call explicitly if you change object frame and 
// need to recompute lights.
EXTERN void md_light_recompute();

// Call this to install the default
// lighter
EXTERN void md_light_init();

// Just set these directly
EXTERN mxs_vector mdd_sun_vec;
EXTERN float mdd_lt_amb;    // ambient value
EXTERN float mdd_lt_diff;   // diffuse value
EXTERN float mdd_lt_spec;   // specular value

// Set true to get linear, false to get perspective
// This is superceded and overwritten if you use the
// fancy util renderer

EXTERN void md_set_tmap_linear(bool l);


// Scale a model according to the scaling vector s.  The scaling is 
// done in source space.  If the model *dst is NULL, it allocates memory
// for it
// light is whether or not to retransform the lighting vectors.  Depending,
// you may actually want it to be the same, plus it's the slowest thing to
// do
EXTERN mds_model *md_scale_model(mds_model *dst,mds_model *src,mxs_vector *s,bool light);

// Shear a model according to the shearing factor s.  The shearing is 
// done in source space, in the ax_src-ax_targ plane.
// For example, if ax_targ=2 (z-axis) and ax_src=1 (y-axis), then
// the transformation is x->x, y->y, z->z+sy.  The result is that a rectangle in
// the y-z plane centered at the origin will become a parallelogram with 
// two vertical sides, and two sides with slope s.
// 
// If the model *dst is NULL, it allocates memory for it
// light is whether or not to retransform the lighting vectors.  Depending,
// you may actually want it to be the same, plus it's the slowest thing to
// do
EXTERN mds_model *md_shear_model(mds_model *dst,mds_model *src,int ax_src,int ax_targ,mxs_real s,bool light);

#endif // __MD_H
