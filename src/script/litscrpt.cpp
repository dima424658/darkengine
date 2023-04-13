/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/script/litscrpt.cpp,v 1.4 1999/05/30 16:14:23 XEMU Exp $
//
// litscrpt.cpp

#include <lg.h>
#include <mprintf.h>
#include <appagg.h>

#include <iobjsys.h>
#include <scrptapi.h>
#include <scrptbas.h>
#include <scrptsrv.h>

#include <osystype.h>
#include <config.h>
#include <cfgdbg.h>
#include <litprops.h>
#include <litprop.h>
#include <litnet.h>

#include <litscrpt.h>

// Include this last, please.
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// Message implementations
//


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cLightScrSrv
//


DECLARE_SCRIPT_SERVICE_IMPL(cLightScrSrv, Light)
{
public:

   STDMETHOD_ (void, Set)(const object& obj, int mode,
                          float min_brightness, float max_brightness)
   {
      sAnimLightProp *pAnimLight;

      if (GetAnimLight(obj, &pAnimLight))
         SetCore(obj, pAnimLight, mode, min_brightness, max_brightness);
   }

   STDMETHOD_ (void, SetMode)(const object& obj, int mode)
   {
      sAnimLightProp *pAnimLight;

      if (GetAnimLight(obj, &pAnimLight))
         SetCore(obj, pAnimLight, mode, pAnimLight->animation.min_brightness,
                 pAnimLight->animation.max_brightness);
   }

   STDMETHOD_(int, GetMode)(const object& obj)
   {
      sAnimLightProp *pAnimLight;

      if (GetAnimLight(obj, &pAnimLight))
         return(pAnimLight->animation.mode);
      else
         return(-1);
   }

   STDMETHOD_ (void, Activate)(const object& obj)
   {
      sAnimLightProp *pAnimLight;

      if (GetAnimLight(obj, &pAnimLight))
         pAnimLight->animation.inactive = FALSE;
   }

   STDMETHOD_ (void, Deactivate)(const object& obj)
   {
      sAnimLightProp *pAnimLight;

      if (GetAnimLight(obj, &pAnimLight))
         pAnimLight->animation.inactive = TRUE;
   }

   STDMETHOD_ (void, Subscribe)(const object& obj)
   {
      sAnimLightProp *pAnimLight;

      if (GetAnimLight(obj, &pAnimLight))
        pAnimLight->notify_script = obj;
   }

   STDMETHOD_ (void, Unsubscribe)(const object& obj)
   {
      sAnimLightProp *pAnimLight;

      if (GetAnimLight(obj, &pAnimLight))
        pAnimLight->notify_script = OBJ_NULL;
   }

private:

   // simple wrapper to spew if prop not on object
   // 5/29/99 Xemu: changed to spew
   BOOL GetAnimLight(ObjID obj, sAnimLightProp **pAnimLight)
   {
      if (!(g_AnimLightProp->Get(obj, pAnimLight))) {
         ConfigSpew("LightSpew",("Light script service: no anim light property on %d\n",
                  obj));
         return FALSE;
      }
      return TRUE;
   }


   // internal: assumes valid prop struct
   void SetCore(ObjID obj, sAnimLightProp *pAnimLight, int mode,
                float min_brightness, float max_brightness)
   {
      int countdown = pAnimLight->animation.countdown_ms;
      BOOL isRising = pAnimLight->animation.is_rising;

#ifdef NEW_NETWORK_ENABLED
      if (pAnimLight->animation.mode != mode) {
         // Tell the clients that we're changing the light:
         BroadcastAnimLightMode(obj, mode);
      }
#endif

      // We slam the countdown to a value which preserves the light
      // level.  We also slam the isRising field for the whacky modes.
      if ((mode == ANIM_LIGHT_MODE_SMOOTH && isRising)
       || (mode == ANIM_LIGHT_MODE_SMOOTH_BRIGHTEN)) {
         countdown = TimeLeft(pAnimLight->animation.brightness,
                              max_brightness, min_brightness,
                              pAnimLight->animation.time_rising_ms);
         pAnimLight->animation.is_rising = TRUE;
      }

      if ((mode == ANIM_LIGHT_MODE_SMOOTH && !isRising)
       || (mode == ANIM_LIGHT_MODE_SMOOTH_DIM)) {
         countdown = TimeLeft(pAnimLight->animation.brightness,
                              min_brightness, max_brightness,
                              pAnimLight->animation.time_falling_ms);
         pAnimLight->animation.is_rising = FALSE;
      }

      pAnimLight->animation.countdown_ms = countdown;
      pAnimLight->animation.mode = mode;
      pAnimLight->animation.inactive = FALSE;
   }


   int TimeLeft(float current, float start, float end, int ms)
   {
      float ratio = (current - start) / (end - start);  // how far along time
      if (ratio > 1.0)  // clamp to given start & end
         return ms;
      if (ratio < 0.0)
         return 0;
      return (int) (ratio * (float) ms);
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cLightScrSrv, Light);

///////////////////////////////////////////////////////////////////////////////

