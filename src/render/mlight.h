/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mlight.h,v 1.12 2000/01/29 13:39:15 adurant Exp $
#pragma once

#ifndef __MLIGHT_H
#define __MLIGHT_H

#include <matrixs.h>

#ifdef RGB_LIGHTING
typedef mxs_vector rgb_vector;
#endif

typedef struct
{
   mxs_vector loc;
   mxs_vector dir;
#ifdef RGB_LIGHTING
   rgb_vector bright;
#else
   float bright;
#endif
   float inner,outer;
   float radius;
} mls_multi_light;   // 32 bytes or 48 bytes

// ambient base for next object
EXTERN float mld_multi_ambient;

#ifdef RGB_LIGHTING
EXTERN rgb_vector mld_multi_rgb_ambient;
#endif

// should the next object be unlit - ie. full-brite
EXTERN BOOL mld_multi_unlit;

EXTERN BOOL mld_multi_ambient_only;

// by what amount do you want the next object "hilit"
EXTERN float mld_multi_hilight;

EXTERN bool mld_multi_rgb;

// How should the self illumination get scaled
// for self illuminated models
EXTERN float mld_illum_scale;

#ifdef RGB_LIGHTING
// Set this if you want a colored light in the bulb, rather
// than a white light.  It will scale accordingly
// For instance, if you had a green light, set to 0,1,0
// or something like that
EXTERN mxs_vector mld_illum_rgb;
#endif


// initialize mlight system
EXTERN void ml_init_multi_lighting(void);

EXTERN int ml_multi_set_lights_for_object(int num, mls_multi_light *lights,
         mxs_vector *obj_center, float obj_radius);

// compute lighting due to a single light, useful for non-rendering
// information about light on an object
EXTERN float ml_multi_light_on_object(mls_multi_light *inp,
         mxs_vector *obj_center);


#endif
