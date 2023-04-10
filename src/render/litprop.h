/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/litprop.h,v 1.13 1999/12/17 17:28:31 BFarquha Exp $

#pragma once

#ifndef _LITPROP_H_
#define _LITPROP_H_

#include <property.h>
#include <litpropt.h>
#include <matrixs.h>

#define PROP_SPOTLIGHT_NAME "Spotlight"
#define PROP_SPOTLIGHTAMB_NAME "SpotAmb"

/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\
   regular (not animated) light property
\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#undef INTERFACE
#define INTERFACE ILightProperty
DECLARE_PROPERTY_INTERFACE(ILightProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sLightProp*);
};


#define PROP_LIGHT_DATA_NAME "Light"

EXTERN BOOL LightPropInit(void);
EXTERN void LightPropRelease(void);
EXTERN BOOL ObjLightGet(ObjID obj, sLightProp **light);
EXTERN BOOL ObjLightSet(ObjID obj, sLightProp *light);
EXTERN BOOL ObjLightGetBrightness(ObjID obj, float *brightness);
EXTERN BOOL ObjGetSpotlight(ObjID obj, mxs_vector **data);
#ifdef EDITOR
EXTERN void ObjLightShine(ObjID obj);
#endif // EDITOR

// This iterates over all objects with the light property and shines
// them in the level.
EXTERN int ObjLightLightLevel(uchar lighting_type);


typedef struct
{
   float hue;
   float saturation;
} ColorInfo;

#define PROP_LIGHT_COLOR_NAME "LightColor"

#undef INTERFACE
#define INTERFACE IColorProperty
DECLARE_PROPERTY_INTERFACE(IColorProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(ColorInfo*);
};

EXTERN BOOL ObjColorGet(ObjID obj, float *hue, float *saturation);


/* ----- /-/-/-/-/-/-/-/-/ <<< (((((( /\ )))))) >>> \-\-\-\-\-\-\-\-\ ----- *\

   animated light property

\* ----- \-\-\-\-\-\-\-\-\ <<< (((((( \/ )))))) >>> /-/-/-/-/-/-/-/-/ ----- */

#undef INTERFACE
#define INTERFACE IAnimLightProperty
DECLARE_PROPERTY_INTERFACE(IAnimLightProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sAnimLightProp*);
};


#define PROP_ANIM_LIGHT_DATA_NAME "AnimLight"

EXTERN BOOL AnimLightPropInit(void);
EXTERN void AnimLightPropRelease(void);
EXTERN BOOL ObjAnimLightGet(ObjID obj, sAnimLightProp **anim_light);
EXTERN BOOL ObjAnimLightSet(ObjID obj, sAnimLightProp *anim_light);
EXTERN BOOL ObjAnimLightGetBrightness(ObjID obj, float *brightness);
#ifdef EDITOR
EXTERN void ObjAnimLightShine(ObjID obj);
#endif // EDITOR

// This iterates over all the animated lights and shines them into
// the level.
EXTERN int ObjAnimLightLightLevel(uchar lighting_type);

// Portal uses this as a callback every frame to advance the animated
// lights.
EXTERN void ObjAnimLightFrameUpdate(long time_change);

// call this to reset the states of all walls/cells to current prop vals
// i hope
EXTERN void ObjAnimLightReset(void);

EXTERN ILightProperty *g_LightProp;
EXTERN IAnimLightProperty *g_AnimLightProp;


#endif  // ~_LITPROP_H_
