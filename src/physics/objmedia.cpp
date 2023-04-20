/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/objmedia.cpp,v 1.6 1999/03/31 17:00:10 Justin Exp $
//
// Object media stuff
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <config.h>
#include <objtype.h>
#include <wrfunc.h>

#include <phcore.h>
#include <phmods.h>
#include <phmod.h>

#include <property.h>
#include <propman.h>
#include <flowarch.h>

#include <ctagset.h>
#include <esnd.h>
#include <psndapi.h>

#include <objpos.h>
#include <playrobj.h>
#include <plyrmode.h>
#include <phmedtr.h>
#include <media.h>

#include <objmedia.h>

#include <mprintf.h>

// Must be last header
#include <dbmem.h>

////////////////////////////////////////

static int g_MediaAmbHandle = -1;
static const char *g_MediaAmbWater = "underwat";


void ObjMediaTrans(ObjID objID, int cellID, eMediaState old_medium, eMediaState new_medium)
{
   #ifndef SHIP
   if (config_is_defined("MediaSpew"))
      mprintf("[%d] Media Trans (%d -> %d)\n", objID, old_medium, new_medium);
   #endif

   // Get the new flow obj
   AutoAppIPtr_(PropertyManager, pPropMan);
   IFlowGroupProperty *pFlowProp = (IFlowGroupProperty *)pPropMan->GetPropertyNamed(PROP_FLOW_GROUP_NAME);

   ObjID flow_arch;

   if (cellID != CELL_INVALID)
      flow_arch = pFlowProp->GetObj(WR_CELL(cellID)->motion_index);
   else
      flow_arch = OBJ_NULL;

   // Get the object's velocity
   mxs_vector velocity;

   cPhysModel *pModel = g_PhysModels.Get(objID);
   if (pModel)
      velocity = pModel->GetVelocity();
   else
      mx_zero_vec(&velocity);

   if (PlayerObjectExists() && (objID == PlayerObject()))
   {
      // Surfacing
      if ((old_medium == kMS_Liquid_Submerged) && (new_medium != kMS_Liquid_Submerged))
      {
         #ifndef SHIP
         if (config_is_defined("MediaSpew"))
            mprintf("  surfacing\n");
         #endif

         // stop underwater sample
         if (g_MediaAmbHandle != -1)
         {
            SoundHalt(g_MediaAmbHandle);
            g_MediaAmbHandle = -1;
         }

         // surfacing schema
         cTagSet SoundEvent("Event MediaTrans");
         SoundEvent.Add(cTag("MedTransDir", "Exit"));
         SoundEvent.Add(cTag("MediaLevel", "Head"));

         ESndPlayLoc(&SoundEvent, objID, flow_arch, &ObjPosGet(objID)->loc.vec);
      }

      // Submerging
      if ((old_medium != kMS_Liquid_Submerged) && (new_medium == kMS_Liquid_Submerged))
      {
         #ifndef SHIP
         if (config_is_defined("MediaSpew"))
            mprintf("  submerging\n");
         #endif

         // play underwater sample
         sfx_parm loop_parm = { 64, SFXFLG_LOOP, 0, 0, 0, -2000, 0, 0, 0, 0, NULL, NULL };

         if (g_MediaAmbHandle != -1)
            SoundHalt(g_MediaAmbHandle);
         g_MediaAmbHandle = GenerateSound(g_MediaAmbWater, &loop_parm);

         // submerging schema
         cTagSet SoundEvent("Event MediaTrans");
         SoundEvent.Add(cTag("MedTransDir", "Enter"));
         SoundEvent.Add(cTag("MediaLevel", "Head"));

         ESndPlayLoc(&SoundEvent, objID, flow_arch, &ObjPosGet(objID)->loc.vec);
      }

      // Body entering water
      if (((new_medium == kMS_Liquid_Submerged) || (new_medium == kMS_Liquid_Wading)) &&
          ((old_medium == kMS_Liquid_Standing) || (old_medium == kMS_Air)))
      {
         // play body splash if velocity high enough
         if (velocity.z < -5.0)
         {
            #ifndef SHIP
            if (config_is_defined("MediaSpew"))
               mprintf("  body entering water\n");
            #endif

            cTagSet SoundEvent("Event MediaTrans");
            SoundEvent.Add(cTag("MedTransDir", "Enter"));
            SoundEvent.Add(cTag("MediaLevel", "Body"));

            ESndPlayLoc(&SoundEvent, objID, flow_arch, &ObjPosGet(objID)->loc.vec);
         }

         g_pPlayerMode->SetMode(kPM_Swim);
      }

      // Body exiting water
      if (((new_medium == kMS_Liquid_Standing) || (new_medium == kMS_Air)) &&
          ((old_medium == kMS_Liquid_Submerged) || (old_medium == kMS_Liquid_Wading)))
      {
         if (velocity.z > 5.0)
         {
            #ifndef SHIP
            if (config_is_defined("MediaSpew"))
               mprintf("  body exiting water\n");
            #endif

            // play out of water schema
            cTagSet SoundEvent("Event MediaTrans");
            SoundEvent.Add(cTag("MedTransDir", "Exit"));
            SoundEvent.Add(cTag("MediaLevel", "Body"));

            ESndPlayLoc(&SoundEvent, objID, flow_arch, &ObjPosGet(objID)->loc.vec);
         }

         g_pPlayerMode->SetMode(kPM_Stand);
      }

      // Foot entering water
      if (((new_medium == kMS_Liquid_Standing) || (new_medium == kMS_Liquid_Wading) ||
           (new_medium == kMS_Liquid_Submerged)) &&
          (old_medium == kMS_Air))
      {
         #ifndef SHIP
         if (config_is_defined("MediaSpew"))
            mprintf("  foot entering water\n");
         #endif

         // play foot hitting water schema if velocity high enough
         if (velocity.z < -5.0)
         {
            cTagSet SoundEvent("Event MediaTrans");
            SoundEvent.Add(cTag("MedTransDir", "Enter"));
            SoundEvent.Add(cTag("MediaLevel", "Foot"));

            ESndPlayLoc(&SoundEvent, objID, flow_arch, &ObjPosGet(objID)->loc.vec);
         }
      }
   }
   else
   {
      if ((old_medium == kMS_Liquid_Submerged) && (new_medium == kMS_Air))
      {
         #ifndef SHIP
         if (config_is_defined("MediaSpew"))
            mprintf("  obj exits water\n");
         #endif

         // @TBD: do we want an emerge sound for objects?
      }

      if ((old_medium == kMS_Air) && (new_medium == kMS_Liquid_Submerged))
      {
         #ifndef SHIP
         if (config_is_defined("MediaSpew"))
            mprintf("  obj enters water\n");
         #endif

         if (velocity.z < - 5.0)
         {
            cTagSet SoundEvent("Event MediaTrans");
            SoundEvent.Add(cTag("MedTransDir", "Enter"));

            ESndPlayLoc(&SoundEvent, objID, flow_arch, &ObjPosGet(objID)->loc.vec);
         }
      }
   }

   sPhysMediaEvent event = { objID, cellID, old_medium, new_medium }; 
   PhysSendMediaEvent(&event); 
}

////////////////////////////////////////

int ObjMediumToPortalMedium(eMediaState obj_medium)
{
   switch (obj_medium)
   {
      case kMS_Air: 
         return MEDIA_AIR;

      case kMS_Liquid_Submerged:
         return MEDIA_WATER;

      case kMS_Invalid:
         return MEDIA_SOLID;

      case kMS_Liquid_Standing:
      case kMS_Liquid_Wading:

      default:
         return MEDIA_AIR;
   }
}

////////////////////////////////////////

eMediaState PortalMediumToObjMedium(int portal_medium)
{
   switch (portal_medium)
   {
      case MEDIA_AIR:
         return kMS_Air;

      case MEDIA_WATER:
         return kMS_Liquid_Submerged;

      case MEDIA_SOLID:
      case NO_MEDIUM:

      default:
         return kMS_Invalid;
   }
}

////////////////////////////////////////


