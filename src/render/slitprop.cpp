/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/slitprop.cpp,v 1.15 1999/03/04 17:30:37 XEMU Exp $

#include <propert_.h>
#include <propface.h>
#include <objslit.h>
#include <slitprop.h>
#include <objlight.h>
#include <objpos.h>
#include <rendprop.h>
#include <stdlib.h>
#include <math.h>
#include <osysbase.h>
#include <portal.h>
#include <litprop.h>


// Must be last header 
#include <dbmem.h>


////////////////////////////////////////////////////////////
// SELF-ILLUMINATION PROP API 
//


////////////////////////////////////////////////////////////
// SELF-ILLUMINATION PROPERTY CREATION 
//

static IIntProperty* gSelfLitProp = NULL;

static sPropertyDesc selflitprop_desc  = 
{
   PROP_SELF_LIT_NAME,
   kPropertyNoInherit|kPropertyInstantiate,
   NULL, 0, 0, 
   { "Renderer", "Dynamic Light" }, 
};

void SelfLitPropInit(void)
{
   gSelfLitProp = CreateIntProperty(&selflitprop_desc, kPropertyImplDense);
}


BOOL ObjGetSelfLit(ObjID obj, int *psl)
{
   if (gSelfLitProp)
   {
      BOOL retval = gSelfLitProp->Get(obj,psl);
      return retval;
   }
   return FALSE;
}

BOOL ObjSetSelfLit(ObjID obj, int sl)
{

   if (gSelfLitProp)
   {
      gSelfLitProp->Set(obj,sl);
      return TRUE;
   }
   return FALSE;
}


//
// Update all self-lit objects
//

extern "C" BOOL g_lgd3d;

void SelfLitUpdateAll(void)
{
   if (!gSelfLitProp) 
      return; 

   IIntProperty* prop = gSelfLitProp; 
   sPropertyObjIter iter; 
   ObjID obj;  
   int light; 

   prop->IterStart(&iter);
   while(prop->IterNextValue(&iter,&obj,&light))
      if (OBJ_IS_CONCRETE(obj) && ObjHasRefs(obj))
      {
         ObjPos* pos = ObjPosGet(obj);
         // @HACK for EEE: larger radius in hardware
#ifdef RGB_LIGHTING
         {
            float hue,saturation;
            int r,g,b;
            ObjColorGet(obj, &hue, &saturation);
            portal_convert_hsb_to_rgb(&r,&g,&b,hue,saturation);
            portal_set_normalized_color(r,g,b);
         }
#endif
         add_dynamic_light(&pos->loc, light, g_lgd3d ? 10.0 : 4.0);
      }
   prop->IterStop(&iter); 
}


static IIntProperty* g_pShadowProp = NULL;

static sPropertyDesc shadow_prop_desc  = 
{
   PROP_SHADOW_NAME,
   kPropertyNoInherit|kPropertyInstantiate,
   NULL, 0, 0, 
   { "Renderer", "Shadow" }, 
};

void ShadowPropInit(void)
{
   g_pShadowProp = CreateIntProperty(&shadow_prop_desc,
                                     kPropertyImplLlist);
}


BOOL ObjGetShadow(ObjID obj, int *s)
{
   if (g_pShadowProp) {
      BOOL retval = g_pShadowProp->Get(obj, s);
      return retval;
   }
   return FALSE;
}

BOOL ObjSetShadow(ObjID obj, int s)
{
   if (g_pShadowProp)
   {
      g_pShadowProp->Set(obj, s);
      return TRUE;
   }
   return FALSE;
}


//
// Update all objects with shadows
//

// Up to this distance from the object's center, the shadow is
// at full strength.
#define kShadowCutoff 6.0
// If the floor is further than this, there's no shadow.
#define kShadowMaxDist 15.0

void ShadowUpdateAll(void)
{
   if (!g_pShadowProp) 
      return; 

   IIntProperty* prop = g_pShadowProp; 
   sPropertyObjIter iter;
   ObjID obj;
   int iLevel;
   float fIntensity;
   float fZDiff;
   ObjPos *pos;
   Location EndLoc, HitLoc;

   prop->IterStart(&iter);
   while(prop->IterNextValue(&iter, &obj, &iLevel))
      if (OBJ_IS_CONCRETE(obj) && ObjHasRefs(obj)) {
         fIntensity = iLevel;
         pos = ObjPosGet(obj);
         EndLoc = pos->loc;
         HitLoc = EndLoc;
         EndLoc.vec.z -= kShadowMaxDist;

         if (!PortalRaycast(&pos->loc, &EndLoc, &HitLoc, 0)) {
            fZDiff = pos->loc.vec.z - HitLoc.vec.z;
            if (fZDiff > kShadowCutoff)
               fIntensity *= (kShadowMaxDist - fZDiff)
                           / (kShadowMaxDist - kShadowCutoff);

            HitLoc.vec.z += .1;
            portal_add_simple_dynamic_dark(fIntensity, fIntensity,
                                           &HitLoc, 8.0);
         }
      }
   prop->IterStop(&iter); 
}

