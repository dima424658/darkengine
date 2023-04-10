/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/elitprop.h,v 1.2 2000/01/29 13:38:44 adurant Exp $
#pragma once

/* --<<= --/-/-/-/-/-/-/ <<< ((( ((( /\ ))) ))) >>> \-\-\-\-\-\-\-- =>>-- *\
   elitprop.h

   sExtraLightProp

   This is a property for messing with object lighting on-the-fly.
   It can either slam the light to a given level [0..1] or modify it
   [-1..1].

\* --<<= --\-\-\-\-\-\-\ <<< ((( ((( \/ ))) ))) >>> /-/-/-/-/-/-/-- =>>-- */


#ifndef _OLITPROP_H_
#define _OLITPROP_H_

#include <matrixs.h>
#include <property.h>

typedef struct sExtraLightProp
{
   float m_fValue;
   BOOL m_bAdditive;
} sExtraLightProp;


#undef  INTERFACE
#define INTERFACE IExtraLightProperty
DECLARE_PROPERTY_INTERFACE(IExtraLightProperty)
{
   DECLARE_UNKNOWN_PURE(); 
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(sExtraLightProp*);
};

#define PROP_EXTRA_LIGHT_NAME "ExtraLight"


EXTERN BOOL ExtraLightPropInit();

EXTERN BOOL ObjExtraLightGet(ObjID obj, sExtraLightProp **ppLight);
EXTERN BOOL ObjExtraLightSet(ObjID obj, sExtraLightProp *pLight);

#endif // _OLITPROP_H_
