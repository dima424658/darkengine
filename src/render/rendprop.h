/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rendprop.h,v 1.17 2000/02/18 14:17:49 MAT Exp $
#pragma once

#ifndef __RENDPROP_H
#define __RENDPROP_H

#include <objtype.h>
#include <partgrp.h>

#include <property.h>
#include <propface.h>

//
// 'Has Refs' Property
//

#define PROP_REF_NAME "HasRefs"

EXTERN BOOL ObjHasRefs(ObjID obj);
EXTERN void ObjSetHasRefs(ObjID obj, BOOL hasrefs);

// 'render transparency' property

#define PROP_ALPHA_NAME "RenderAlpha"

EXTERN IFloatProperty *gAlphaRenderProp;

//
// 'Self illumination' Property
//

#define PROP_SELF_ILLUM_NAME "SelfIllum"

EXTERN IFloatProperty *g_pSelfIlluminationProp;


//
// 'Object Shadow' Property
//

#define PROP_OBJSHAD_NAME "ObjShad"

EXTERN IBoolProperty *g_pObjShadowProp;


//
// 'Render Type' Property
//

#define PROP_RENDER_TYPE_NAME "RenderType"

enum eRenderType_
{
   kRenderNormally,
   kRenderNotAtAll,
   kRenderUnlit,
   kRenderEditorOnly,
};

typedef int eRenderType;

EXTERN eRenderType ObjRenderType(ObjID obj);
EXTERN void        ObjSetRenderType(ObjID obj, eRenderType rtype);

//
// Joint Pos Property
//

#define PROP_JOINTPOS_TYPE_NAME "JointPos"

#define MAX_REND_JOINTS 6

typedef struct {
   float el[MAX_REND_JOINTS];
} sJointPos;

#undef INTERFACE
#define INTERFACE IJointPosProperty
DECLARE_PROPERTY_INTERFACE(IJointPosProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sJointPos*);
};


EXTERN float *ObjJointPos(ObjID obj);
EXTERN void   ObjSetJointPos(ObjID obj, float *parms);

//
// Bitmap worldspace property
// indicates that a bitmap model is to be drawn in worldspace rather
// than always facing the camera
//

#define PROP_BITMAP_WORLDSPACE_NAME "BitmapWorldspace"

typedef struct {
   float m_fXSize;      // dimensions in worldspace
   float m_fYSize;
   float m_fXFeetPerTile;
   float m_fYFeetPerTile;
} sBitmapWorldspace;

EXTERN BOOL ObjBitmapWorldspace(ObjID obj, sBitmapWorldspace **ppBWS);


//
// Bitmap animation property
//

#define PROP_BITMAP_ANIMATION_NAME "BitmapAnimation"

#define kKillOnComplete  1

typedef int BitmapFlags;

EXTERN BitmapFlags ObjBitmapFlags(ObjID obj);


/////////////////

EXTERN IIntProperty *g_pIsInvisibleProperty;

/////////////////////////////
// Initialization

EXTERN void RenderPropsInit(void);
EXTERN void RenderPropsShutdown(void);

#endif // __RENDPROP_H
