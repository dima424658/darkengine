/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

/*
 * $Source: x:/prj/tech/libsrc/md/RCS/mdutil.h $
 * $Revision: 1.14 $
 * $Author: alique $
 * $Date: 1970/01/01 00:00:00 $
 *
 * Model Library prototypes
 *
 */


#ifndef __MDUTIL_H
#define __MDUTIL_H
#pragma once

#include <mds.h>
#include <r3ds.h>

typedef unsigned short ID; // to avoid including RES.h

// Useful Utilities Functions

// Locks down textures using a resource offset
// This assumes that the texture handles are set
// to be added to a resource offset to get their
// ResID.  So if you do sneakiness, this won't work
// Call this just before rendering the model
// Assumes textures are stored as .btm

EXTERN void md_mat_textures_lock(mds_model *m,ID base);

// resource flags to set when locking down textures
EXTERN void md_set_texture_lock_flags(ulong flags);

// Sets the colors of a model, should work in both
// 8 bit mode (with grd_ipal set) and in 16 bit or
// 24 bit mode.  Call this just before rendering the
// model

EXTERN void md_mat_colors_set(mds_model *m);

// Just like md_mat_textures_lock, only it unlocks
// the textures.

EXTERN void md_mat_textures_unlock(mds_model *m,ID base);


// Returns the maximum pixel error in the biggest
// texture map in the model

EXTERN float md_tmap_err(mds_model *m);


// Pixel error below which you use linear texture
// maps used by md_fancy_render_model. 1 means pixel
// perfect.  Defaults to 1

EXTERN void md_set_linear_breakpoint(float b);

// Renders model just like fancy_render, but always clips
EXTERN void md_fancy_cliphappy_render_model(mds_model *m,mds_parm parms[]);

// Renders the model by first checking the bounding
// box, turning on clipping if it needs to be clipped,
// not if not, does either linear or perspective texturing
// depending on where the linear breakpoint is

EXTERN void md_fancy_render_model(mds_model *m,mds_parm parms[]);

// Does all the setup of md_fancy_render_model without
// actually bothering to render it (so for example you
// can use the multi-stage rendering interface)
EXTERN BOOL md_fancy_setup_model(mds_model *m,mds_parm parms[]);

// Checks whether the segment a-b intersects the model.
// it fills in hit, hit_normal, and r appropriately.
EXTERN void md_segment_hit_detect(mds_model *m, mds_segment_hit_detect_info *hdi);
EXTERN void md_msh_segment_hit_detect(mds_model *m, mds_segment_hit_detect_info *hdi);
// Checks whether sphere intersects model.  Must allocate enough
// room in hdi->polys for entire model worth of poly refs.
EXTERN void md_sphere_hit_detect(mds_model *m, mds_sphere_hit_detect_info *hdi);

// Mip Mapping routines.
// Mip maps are just r3s_textures, but instead of pointing to
// one grs_bitmap, they point to an array of them.  All are allocated
// as one block, and so are freed as one.  By definition, these
// mip maps go down to 1x1, and that's when you know you are done.
// Seans definition has a mipmap ending with a grs_bitmap with zeros
// someday we should reconcile the two.

// Returns size of an already existing mipmap
EXTERN int md_sizeof_mipmap(r3s_texture mmap);

// Given an already existing mipmap, returns how big all the bits are
EXTERN int md_sizeof_mipmap_bits(r3s_texture mmap);

// Given a BMT type, w and h, returns how big the mipmap would be
// and stuffs the number of levels
EXTERN int md_sizeof_mipmap_type(int *pNumLevels,int type,int w,int h);

// Render mip map levels upper left going down
// Just like gr_bitmap, but shows you all the levels
EXTERN void md_mipmap_render(r3s_texture tmap,int x,int y);

// Like a bitmap alloc, but with mip maps, free the whole at once
// Creates it all in one big block, copies srcbits
// if pal is set, uses it, else uses the current ipal.
// ignores if srcbits NULL
// Uses pMem if non-null, else allocates on its own
EXTERN r3s_texture md_mipmap_alloc(uchar *pMem,uchar *srcbits,int type,int flags,int w,int h,uchar *pal);

// Used for the callback when you set
// md_set_render_pgon_callback
// hooks transparently onto models if you have the mip maps set
// known bugs are that it mostly works if the whole object is unclipped,
// and if it's relatively non-perspective
EXTERN void md_mipmap_render_pgon(mds_pgon *p, r3s_phandle *v,grs_bitmap *bm, ulong color, ulong type);

// under construction
EXTERN void md_rg_mipmap_render_pgon(mds_pgon *p, r3s_phandle *v,grs_bitmap *bm, ulong color, ulong type);

// Higher detail = use higher res mip.  default = 1.0.
EXTERN void md_mipmap_set_detail(float detail);
EXTERN float md_mipmap_get_detail(void);

#endif // __MDUTIL_H