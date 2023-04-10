/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/objlight.h,v 1.16 1999/06/01 09:09:05 Zarko Exp $
//

#pragma once

#ifndef __OBJLIGHT_H
#define __OBJLIGHT_H

#include <osystype.h>
#include <matrixs.h>
#include <wrtype.h>
#include <mlight.h>

// objID of object to be highlighted in the world
EXTERN ObjID highlit_obj;

EXTERN void reset_object_lighting(void);
EXTERN void reset_dynamic_lighting(void);

// add lights to world
#ifdef RGB_LIGHTING
EXTERN int add_object_light(mxs_vector *where, rgb_vector *bright, float radius);
EXTERN int add_object_spotlight(mxs_vector *where, rgb_vector *bright, mxs_vector *dir, float inner, float outer, float radius);
#else
EXTERN int add_object_light(mxs_vector *where, float bright, float radius);
EXTERN int add_object_spotlight(mxs_vector *where, float bright, mxs_vector *dir, float inner, float outer, float radius);
#endif

EXTERN void add_dynamic_light(Location *loc, float bright,
                              float clamp_radius);
// set a light we've already created
#ifdef RGB_LIGHTING //zb
EXTERN void set_object_light(int light_index, float bright, rgb_vector* p_color);
#else
EXTERN void set_object_light(int light_index, float bright);
#endif
// does a light of this light ID already exist?
EXTERN BOOL is_object_light_defined(int handle);

// various setup stuff
EXTERN void setup_object_lighting(ObjID o, mxs_vector *newlight, float bright);

// compute correct lighting
EXTERN float compute_object_lighting(ObjID o);

// for highlighting currently focused object
EXTERN float objlight_highlight_level;

// when an object moves, shadows on it may change...
EXTERN void objShadowUncache(ObjID o);
EXTERN void ObjShadowInit(void);
EXTERN void ObjShadowTerm(void);
EXTERN void ObjShadowClearCache(void);

#define MAX_STATIC   768
#define MAX_DYNAMIC  32

// if you want to see what is lighting a given object (listed on the mono)
#ifndef SHIP
EXTERN float blame_object_lighting(ObjID o);
#endif

#endif  // __OBJLIGHT_H
