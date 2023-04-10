// $Header: x:/prj/tech/libsrc/mm/RCS/mm.h 1.10 1999/03/01 16:05:09 Zarko Exp $

#ifndef __MM_H
#define __MM_H

#include <lg.h>
#include <mms.h>
#include <r3ds.h>
#include <qt.h>

////// global setup
// 

void mm_init();

void mm_close();

void mm_debug_toggle();

////// set callbacks
//

typedef void (*mmf_joint_pos_cback)(mms_model *m,int joint_id,
                                    mxs_trans *trans);

// set the joint position callback function.  Should provide the joint position
// and orientation in world coordinates.
// function returns previous joint pos callback
EXTERN mmf_joint_pos_cback mm_set_joint_pos_cback(mmf_joint_pos_cback func);

typedef void (*mmf_stretchy_joint_cback)(mms_model *m, int joint_id,
                                         mxs_trans *trans, quat *joint);

// set the stretchy joint callback function.  
// this callback should fill in the quaternion for the joint's
// orientation relative to its parent.
// function returns previous stretchy joint callback
EXTERN mmf_stretchy_joint_cback mm_set_stretchy_joint_cback(mmf_stretchy_joint_cback func);


typedef void (*mmf_block_render_cback)(mms_pgon *pgons, int num);

// set the pgon block render function
// this callback should render the array of polygons provides as arguments in
// the order provided.   
// function returns previous block render callback
EXTERN mmf_block_render_cback mm_set_block_render_cback(mmf_block_render_cback func);

// We render our attachments through this callback.
typedef void (*mmf_attachment_cback)(mxs_trans *trans,
                                     mms_attachment *attachment);
EXTERN mmf_attachment_cback mm_set_attachment_cback(mmf_attachment_cback func);

// Sets the light callback, and gets the old one back
// lighting callback for per object setup
typedef void (*mmf_light_setup_cback)(mxs_vector *bmin,mxs_vector *bmax);

// lighting callback for setting the lighting per mesh segment
typedef void (*mmf_light_cback)(int num,float *,mms_uvn *uvns,mxs_vector *pts);

EXTERN mmf_light_setup_cback mm_set_light_setup_callback(mmf_light_setup_cback);
EXTERN mmf_light_cback mm_set_light_callback(mmf_light_cback);

////// rendering stuff
//

enum {
   MM_RPIPE_POLYSORT,
   MM_RPIPE_ZBUFFER,
   MM_RPIPE_HARDWARE,
   MM_NUM_RPIPES,
};
// set renderer pipeline used when mm_render_model is called
// can render using poly sort, software z-buffer, or
// using hardware.  
EXTERN void mm_set_render_pipeline(ubyte pipe);

// You can call the whole pipeline at once, or pieces.  Pass in NULL
// for the custom data, for any of these functions, if you don't have
// any.
EXTERN void mm_render_model(mms_model *m, mms_custom_data *d, ulong flags);

// flags to be passed to mm_render_model()
#define MMF_XFORM_STRETCHY_UNWEIGHTED  1
#define MMF_INDEXED                    2

EXTERN int mm_buffsize(mms_model *m, mms_custom_data *d);
EXTERN void mm_set_buff(mms_model *m, mms_custom_data *d, void *buff);
EXTERN void mm_sort_only(mms_model *m, ulong flags);
EXTERN void mm_transform_only(mms_model *m, ulong flags);
EXTERN void mm_render_only(mms_model *m);

// You should set to R3_PL_TEXTURE or R3_PL_TEXTURE_LINEAR
EXTERN void mm_set_tmap_mode(ulong mode);

// default pgon renderer.  gets called automatically by render_model and
// render_only
EXTERN void mm_render_pgon_block(mms_pgon *pgons,int num);

// not supported yet
EXTERN void mm_render_smatr(mms_smatr *r);
EXTERN void mm_render_segment(mms_segment *s);


/////// non-rendering (still in render.c for access to internals)

// Checks whether the segment a-b intersects the model.
// it fills in hit, hit_normal, and r appropriately.
EXTERN BOOL mm_ray_hit_detect(mms_model *model,
                              mms_ray_hit_detect_info *detect_info);

/////// utility functions to help app parse mesh model format

// returns the first segment with the given joing id, or -1 if
// there isn't one
EXTERN int mm_segment_from_joint(mms_model *m, uint joint_index);
EXTERN int mm_rigid_segment_from_joint(mms_model *m, uint joint_index);


/////// internals
//

// Internally, the library must keep several pointers around after getting 
// a pointer to the model.  We expose these for adventurous users
// Carpe diem, er.. Cave Canem, uh.. something like that.
// Caveat Emptor!          [Fleming, 7/96]
// These are:
EXTERN mms_model   *mmd_model;    // pointer to the model
EXTERN mms_smatr   *mmd_smatrs;   // single material regions list
EXTERN mms_segment *mmd_segs;
EXTERN mms_mapping *mmd_mappings;
EXTERN mms_smatseg *mmd_smatsegs;
EXTERN mms_uvn     *mmd_vert_uvns;     // uv and normal list
EXTERN mxs_vector  *mmd_vert_vecs;    // vertex list
EXTERN mms_pgon    *mmd_pgons;    // pgon list
EXTERN mxs_vector  *mmd_norms;       // pgon normal list
EXTERN mms_weight  *mmd_weights;     // weight list
EXTERN ulong       mmd_tmap_mode;  // texture mapping mode

EXTERN bool        mmd_rgb_lighting;

// This is much like the pointers above but is passed in separately
// rather than being derived from the model.
EXTERN mms_custom_data *mmd_custom_data; // things for the given model

// globals for object buffer
EXTERN void        *mmd_buff;
EXTERN r3s_point   *mmd_buff_points;
EXTERN float       *mmd_buff_norms;   // a pointer to the normal dot product results
EXTERN mms_pgon    *mmd_buff_sorted_pgons;

#define mm_smatr_list(m) ((mms_smatr *)((uchar *)(m)+((m)->smatr_off)))
#define mm_segment_list(m) ((mms_segment *)((uchar *)(m)+((m)->seg_off)))
#define mm_mapping_list(m) ((mms_mapping *)((uchar *)(m)+((m)->map_off)))
#define mm_smatseg_list(m) ((mms_smatseg *)((uchar *)(m)+((m)->smatseg_off)))
#define mm_vert_uvn_list(m) ((mms_uvn *)((uchar *)(m)+((m)->vert_uvn_off)))
#define mm_vert_vec_list(m) ((mxs_vector *)((uchar *)(m)+((m)->vert_vec_off)))
#define mm_pgon_list(m) ((mms_pgon *)((uchar *)(m)+((m)->pgon_off)))
#define mm_norm_list(m) ((mxs_vector *)((uchar *)(m)+((m)->norm_off)))
#define mm_weight_list(m) ((mms_weight *)((uchar *)(m)+((m)->weight_off)))

#define mmd_buff_point(i) \
   ((r3s_point *)(((uchar *)mmd_buff_points) + r3d_glob.cur_stride * (i)))


//// alpha stuff

// Sets the global alpha that is fixin' to be applied to the whole model.
// any per material alpha will be scaled with this value. The default is 1.0f
// Return: the previous value;
// Scale [0.0f,1.0f)
EXTERN float mm_set_global_alpha( float fNewGlobalAlpha ); 


//// self illumination 
//
//  self-allumination ( Can *your* drink do it!?)  is computed as
//  
// glow = material_glow_level * (global)_glow_factor + global_glow_level
//

// adds light level to the whole object
EXTERN float mm_set_global_glow_level( float fNewGlowLevel ); 

//affects (infects? pardon my Latin) only materials with glow level
EXTERN float mm_set_global_glow_factors( float fNewGlowLevel ); 

#endif