/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vbr_math.h,v 1.3 2000/01/29 13:13:24 adurant Exp $
// math ops for virtual brush manipulation fun (tm)
#pragma once

#ifndef __VBR_MATH_H
#define __VBR_MATH_H

#include <matrixs.h>
#include <matrixds.h>
#include <editbrs.h>

// we need a better way to do this, but for now
EXTERN bool vBrush_relative_motion;
EXTERN bool vBrush_axial_scale;

////////////
// misc exposed utility math things

// given v1 and v2, basically builds mx_eldiveq_vec
EXTERN BOOL compute_scale_fac(mxs_vector *scale, mxs_vector *old_v, mxs_vector *new_v);

// well, we dont have angvec funcs, so this is targ=v1-v2
EXTERN BOOL sub_angvecs(mxs_angvec *targ, mxs_angvec *v1, mxs_angvec *v2);

// this takes two angvecs, quantizes, subs into diff, and returns if diff is non-zero
EXTERN BOOL ang_quantize_and_sub(mxds_vector *diff, mxs_angvec *v1, mxs_angvec *v2);

// find the origin of a brush
EXTERN BOOL find_brush_origin(editBrush *us, mxs_vector *origin, mxs_angvec *ang_in);

///////////
// operators

// actual operator setup calls
EXTERN void br_translate_setup(mxs_vector *raw_vec);
EXTERN void br_scale_setup(mxs_vector *raw_vec);
EXTERN void br_rotate_setup(mxs_angvec *rot);
EXTERN void br_rotate_rad_setup(mxds_vector *real_rot);
   
// actual operators
EXTERN void br_translate(editBrush *us);
EXTERN void br_scale(editBrush *us);
EXTERN void br_rotate(editBrush *us);

// expose a parameter or two for now for the load hacks
// it would be nice to modularize this and get rid of it
EXTERN mxs_vector *br_trans_vec;

/////////////
// crazy "back and store" of brush vecs system
EXTERN BOOL store_BrushVecs(void);
EXTERN void clear_BrushVecs(void);
EXTERN void restore_BrushVecs(int which);
#define brVec_NONE (-1)
#define brVec_ANG  1
#define brVec_POS  2
#define brVec_SZ   4

#endif  // __VBR_MATH_H
