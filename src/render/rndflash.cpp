/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/rndflash.cpp,v 1.6 1999/10/26 21:00:50 adurant Exp $

#include <property.h>
#include <propert_.h>
#include <propface.h>
#include <propfac_.h>
#include <dataops_.h>
#include <propsprs.h>
#include <propbase.h>
#include <proplist.h>
#include <objsys.h>
#include <objtype.h>
#include <sdesbase.h>
#include <sdestool.h>

#include <rndflash.h>
#include <simtime.h>
#include <posprop.h>
#include <wrtype.h>
#include <objpos.h>
#include <math.h>

//spew
#include <config.h>
#include <cfgdbg.h>

// Must be last header 
#include <dbmem.h>


struct FlashbombRender
{
   uchar r,g,b;      // flash bomb color
   bool worldlit;    // whether or not we've lit the world currently
   int world_flash_duration;
   int screen_flash_duration;
   int after_effect; // duration
   float intensity;  // intensity of effect on player (0 means no effect)
   float range;      // how far the effect reaches
   int start_time;
};

class cRendFlashOps    : public cClassDataOps<FlashbombRender> { }; 
class cRendFlashStore  : public cListPropertyStore<cRendFlashOps> { }; 

typedef cSpecificProperty<IRendFlashProperty,
                       &IID_IRendFlashProperty,
                       FlashbombRender*,cRendFlashStore>
         cRendFlashParent; 

static void world_light_on_load(ObjID obj);
static void world_unlight_on_unset(ObjID obj);

class cRendFlashProperty: public cRendFlashParent
{

public: 
   cRendFlashProperty(const sPropertyDesc* desc):cRendFlashParent(desc){ }
   STANDARD_DESCRIBE_TYPE(FlashbombRender);

   void OnListenMsg(ePropertyListenMsg msg, ObjID obj,
                      uPropListenerValue val)
   {
      cRendFlashParent::OnListenMsg(msg, obj, val);

      if (msg & kListenPropLoad)
         world_light_on_load(obj);

      if (msg & kListenPropUnset)
         world_unlight_on_unset(obj);
   };

}; 

static sPropertyConstraint rndflash_constraint[] =
{
   { kPropertyRequires, PROP_POSITION_NAME },
   { kPropertyNullConstraint }
};

static sPropertyDesc rend_flash_prop_desc=
{
   PROP_REND_FLASH_NAME,
   kPropertyInstantiate,
   rndflash_constraint,
   0,0, // version
   { "SFX", "FlashBombInfo" }
};

#define FL(x)    FieldLocation(FlashbombRender, x)

// structure descriptor fun
static sFieldDesc rndflash_fields [] =
{
   { "flash color r", kFieldTypeInt, FL(r) },
   { "flash color g", kFieldTypeInt, FL(g) },
   { "flash color b", kFieldTypeInt, FL(b) },
   { "world flash duration", kFieldTypeInt, FL(world_flash_duration) },
   { "max screen flash duration (ms)",kFieldTypeInt,FL(screen_flash_duration)},
   { "after-effect duration (ms)", kFieldTypeInt, FL(after_effect) },
   { "range", kFieldTypeFloat, FL(range) },
};
#undef FL

static sStructDesc rndflash_desc = StructDescBuild(FlashbombRender,
                              kStructFlagNone,rndflash_fields);

static IRendFlashProperty  *rndflash_prop = NULL;

BOOL RendFlashPropInit(void)
{
   AutoAppIPtr_(StructDescTools,pTools);
   pTools->Register(&rndflash_desc);
   rndflash_prop  = new cRendFlashProperty(&rend_flash_prop_desc);
   return TRUE;
}

void RendFlashPropShutdown(void)
{
   SafeRelease(rndflash_prop);
}

EXTERN void portal_add_light_bright(Location *loc, float radius, BOOL light);
static void world_light(ObjID obj, FlashbombRender *fr, bool light)
{
   Position *pos = ObjPosGet(obj);

   // do updating of world rep
   if (pos)
      portal_add_light_bright(&pos->loc, fr->range, light);

   // set flag to right state
   fr->worldlit = light;
}

static void world_light_on_load(ObjID obj)
{
   FlashbombRender *fr = NULL;
   rndflash_prop->Get(obj, &fr);
   if (fr != NULL && fr->worldlit) // if it is currently lit, update world rep
      world_light(obj, fr, TRUE);
}

static void world_unlight_on_unset(ObjID obj)
{
   FlashbombRender *fr = NULL;
   rndflash_prop->Get(obj, &fr);
   if (fr != NULL && fr->worldlit) // if it was lit, remove it
      world_light(obj, fr, FALSE);
}

void startFlashbombRender(ObjID obj, float intensity)
{
   FlashbombRender *fr = NULL;
   rndflash_prop->Get(obj, &fr);
   if (fr == NULL) {
      Warning(("Tried to startFlashbombRender on an object with no RendFlash property.\n"));
      return;
   }
   if (fr->worldlit) {
      Warning(("Tried to startFlashbombRender more than once.\n"));
      return;
   }

   fr->intensity = intensity;
   fr->start_time = GetSimTime();
   world_light(obj, fr, TRUE);
}

#define FADE_TO_BLACK 500

//making flash time much longer to allow more variety
int flash_clamp_time=40000;

static float process_object(ObjID obj, FlashbombRender *fr, uchar *rgb)
{
   // determine what phase of the animation we're in
   ulong tm = GetSimTime() - fr->start_time;
   BOOL world_done;  // compute how many systems claim they're done

   float alpha;

   world_done = FALSE;

   if (tm >= fr->world_flash_duration) {
      // remove the light from the world if it's too old
      if (fr->worldlit)
         world_light(obj, fr, FALSE); // darken
      world_done = TRUE;
   }

   if (tm < fr->screen_flash_duration) {
      // initial white phase
      alpha = fr->intensity;
      rgb[0] = fr->r;
      rgb[1] = fr->g;
      rgb[2] = fr->b;
      ConfigSpew("FlashSpew",("FlashSpew: First Alpha %d %g\n",tm,alpha));
   } else if (tm < fr->screen_flash_duration + FADE_TO_BLACK) {
      tm -= fr->screen_flash_duration;
      alpha = 1 - (float) tm / FADE_TO_BLACK;  // 1..0
      rgb[0] = fr->r * alpha;  // crossfade from white to black
      rgb[1] = fr->g * alpha;
      rgb[2] = fr->b * alpha;
      alpha = 1 - (1-alpha)*.05;  // reshift from 1..0 to 1.. 0.95
      if (fr->intensity < 1.0)
	alpha *= fr->intensity;
      ConfigSpew("FlashSpew",("FlashSpew: Second Alpha %d %g\n",tm,alpha));
   } else {
      int max_time = fr->after_effect;
      tm -= fr->screen_flash_duration + FADE_TO_BLACK;
      // shorten the aftereffect by the intensity
      max_time *= fr->intensity;
      if (max_time > flash_clamp_time) max_time = flash_clamp_time;
      if (tm <= max_time) {
         // ramp intensity down from .95 to 0 and stay black
         // tm/max_time --> 0 .. 1
         alpha = 1 - (float) tm / max_time;
         // now bias it to fade slower
         alpha *= 0.95;
         rgb[0] = rgb[1] = rgb[2] = 0;
         if (fr->intensity < 1.0)
	   alpha *= fr->intensity;
         ConfigSpew("FlashSpew",("FlashSpew: Third Alpha %d %g\n",tm,alpha));
      } else {
         // done with the player vision effect
         if (world_done) {
            // done with both, delete!
            DestroyObject(obj);
         }
         alpha = 0;
      }
   }
   return alpha;
}

float compute_flashbomb_effects(uchar *rgb)
{
   sPropertyObjIter iter;
   FlashbombRender *fr;
   ObjID obj;
   float alpha=0;

   rndflash_prop->IterStart(&iter);
   while (rndflash_prop->IterNextValue(&iter, &obj, &fr)) {
      if (OBJ_IS_CONCRETE(obj)) {
         float intensity;
         uchar color[3];
         intensity = process_object(obj, fr, color);
         // the brightest intensity wins
         if (intensity > alpha) {
            alpha = intensity;
            rgb[0] = color[0]; rgb[1] = color[1]; rgb[2] = color[2];
         }
      }
   }
   rndflash_prop->IterStop(&iter);

   return alpha;
}

