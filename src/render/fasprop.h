/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/fasprop.h,v 1.2 2000/01/29 13:38:47 adurant Exp $
#pragma once

#ifndef __FASPROP_H
#define __FASPROP_H

#include <fas.h>
#include <property.h>
#include <propface.h>

#define PROP_FAS_STATE_NAME "FrameAniState"
#define PROP_FAS_CONFIG_NAME "FrameAniConfig"

EXTERN FrameAnimationState *ObjGetFrameAnimationState(ObjID obj);
EXTERN FrameAnimationConfig *ObjGetFrameAnimationConfig(ObjID obj);
EXTERN void ObjSetFrameAnimationState(ObjID obj, FrameAnimationState *ba);
EXTERN void ObjSetFrameAnimationConfig(ObjID obj, FrameAnimationConfig *ba);

EXTERN BOOL FASPropInit(void);
EXTERN void FASPropShutdown(void);

#undef INTERFACE
#define INTERFACE IFrameAnimationStateProperty
DECLARE_PROPERTY_INTERFACE(IFrameAnimationStateProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(FrameAnimationState*); 
}; 

#undef INTERFACE
#define INTERFACE IFrameAnimationConfigProperty
DECLARE_PROPERTY_INTERFACE(IFrameAnimationConfigProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE(); 
   DECLARE_PROPERTY_ACCESSORS(FrameAnimationConfig*); 
}; 

#endif
