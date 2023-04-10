/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendobj.h,v 1.21 2000/02/24 23:42:47 mahk Exp $
// render obj setup and code
#pragma once

#ifndef __RENDOBJ_H
#define __RENDOBJ_H

#include <objtype.h>
#include <matrixs.h>
#include <robjtype.h>

// scaling system
EXTERN float       rendobjScaleFac;
EXTERN mxs_vector  rendobjUnitScale;
EXTERN mxs_vector *rendobjScaleVec;

// publicly available what obj we last saw
EXTERN int  rendobj_seen;

// show names in objects - if color != 0 show names in that color
EXTERN int   rendobj_name_color;
// only name particular types of objects
EXTERN int   rendobj_name_types;
#define kRendObjNameAI      (1<<0)
#define kRendObjNameAnyPhys (1<<1)
#define kRendObjNameActPhys (1<<2)
#define kRendObjNameAll     (0xff)

#ifdef NEW_NETWORK_ENABLED
EXTERN BOOL rendobj_net_name_coloring;        // do network special coloring
// and the specific colors for various net status
EXTERN int  rendobj_local_name_color;
EXTERN int  rendobj_hosted_name_color;
EXTERN int  rendobj_proxy_name_color;
#endif

// for now, rendering calls
EXTERN void render_wedge(uchar color, uchar *clut);

EXTERN void rendobj_render_object(ObjID obj, uchar *clut, ulong fragment);

EXTERN model_funcs *rendobj_get_model_funcs(ObjID obj_id, int *idx);

// and setup
EXTERN void init_object_rendering(void);

// setup per image--clears gaRendObjVisible
EXTERN void rendobj_init_frame(void);

// when we close the game
EXTERN void rendobj_exit(void);


// draw a bounding box
EXTERN void draw_bbox(mxs_vector *loc, mxs_angvec *fac, mxs_vector *bmin, mxs_vector *bmax);
EXTERN void draw_line(mxs_vector *start, mxs_vector *end);
EXTERN void draw_poly(int n, mxs_vector *vert);

// Was this object seen last frame
EXTERN BOOL rendobj_object_visible(ObjID obj); 

#endif // __RENDOBJ_H

