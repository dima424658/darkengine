/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/ged_csg.h,v 1.10 2000/01/29 13:12:09 adurant Exp $
#pragma once

#ifndef __GED_CSG_H
#define __GED_CSG_H

#include <matrixs.h>
#include <editbr_.h>  // for TexInfo

#include <objtype.h>
#include <mediaop.h>

//////////////////////
// interface to csg brush callbacks

EXTERN int gedcsgGetFaceTexture(int x, int y);
EXTERN TexInfo *gedcsgGetFaceTexInfo(int x, int y);
EXTERN BOOL gedcsgGetFaceTextureAligned(int x, int y, mxs_vector *u, mxs_vector *v);
EXTERN BOOL gedcsgGetFaceTextureSelfLit(int x, int y);
EXTERN float gedcsgGetFaceScale(int x, int y);

/////////////////////
// tools for csg to inform game systems about problems
EXTERN void gedcsgHilightBr(int br_id); // hilight brush br_id
EXTERN void gedcsgClrHilights(void);    // clear out old hilights
EXTERN void gedcsgCheckHilights(void);  // check to see if there are any, if so show them

/////////////////////
// internal state/control/protos for ged_Csg itself
// invocation/creation of portalized world rep

// current state of level build and lighting

#ifdef EDITOR 

EXTERN BOOL gedcsg_level_valid, gedcsg_lighting_ok;
#define gedcsg_lighting_changed() gedcsg_lighting_ok = FALSE

#define ged_validate_level(b) \
do { gedit_editted = !(b); gedcsg_level_valid = (b); gedcsg_lighting_ok = (b); } while(0)

#else 

#define gedcsg_level_valid TRUE
#define gedcsg_lighting_ok TRUE
#define gedcsg_lighting_changed() 
#define ged_validate_level(b) 
#endif 


// level lighting flags
EXTERN BOOL gedcsg_light_objcast, gedcsg_light_raycast, gedcsg_light_quad;

// mostly internal incremental do one light operation
EXTERN BOOL gedcsg_parse_light_brush(editBrush *us, uchar dynamic);

// do the whole thing, relight, build, or optimize it all
EXTERN void gedcsg_relight_level(void);
EXTERN void build_level(void);
EXTERN void optimize_level(void);

////////////////////
// mediaop info for others
EXTERN const char *mediaop_names[]; // if you want to name media-ops
EXTERN int   num_media_ops;

// register commands for the system
EXTERN void gedcsgCommandRegister(void);

EXTERN ObjID *g_pObjCastList;
EXTERN int    g_ObjCastListSize;

EXTERN BOOL doing_opt_pass1;

#endif  // __GED_CSG_H






