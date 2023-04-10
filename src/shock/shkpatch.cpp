/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkpatch.cpp,v 1.10 2000/02/19 13:25:51 toml Exp $
// code for doing patch like things to the levels

#include <string.h>

#include <mprintf.h>
#include <config.h>

#include <iobjsys.h>
#include <traitman.h>
#include <netprops.h>
#include <shkprop.h>
#include <scrptpr_.h>
#include <frobprop.h>
#include <phmtprop.h>
#include <aiprcore.h>
#include <lockprop.h>
#include <shkspawn.h>
#include <objpos.h>

#include <shkpatch.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Objnum conversion. Deals with objnum remapping, as well as making sure
// that this really is the right object. May return OBJ_NULL if we can't
// safely resolve the objnum.
static ObjID RealObj(ObjID old_obj, const char *arch_name)
{
   AutoAppIPtr(ObjectSystem);
   AutoAppIPtr(TraitManager);
   ObjID obj = pObjectSystem->RemapOnLoad(old_obj);
   ObjID archetype = pObjectSystem->GetObjectNamed(arch_name);
   if (pTraitManager->GetArchetype(obj) != archetype)
   {
      return OBJ_NULL;
   } else {
      return obj;
   }
}

// Make an object local-only
static void MakeLocal(ObjID old_obj, const char *archetype)
{
   AutoAppIPtr(ObjectSystem);
   ObjID obj = RealObj(old_obj, archetype);
   if (obj != OBJ_NULL)
      SetNetworkCategory(obj, kLocalOnly);
}

// Mark an object as moving terrain
static void MakeMovingTerrain(ObjID startObj, const char *pArchetype)
{
   ObjID obj = RealObj(startObj, pArchetype);
   if (obj != OBJ_NULL)
   {
      cMovingTerrainProp pMTProp;
      pMTProp.active = FALSE;
      pMTProp.prev_state = FALSE;
      g_pMovingTerrainProperty->Set(obj, &pMTProp);
   }
}

// Macro to test whether an update is needed. Intended to be used inside
// PatchLevelData().
#define NEED_UPDATE(target, patchlevel) \
  ((stricmp(level, target) == 0) &&\
   (old_ver < patchlevel) &&\
   (new_ver >= patchlevel))

void PatchLevelData(char *level, int old_ver, int new_ver)
{
#ifndef SHIP
   if (config_is_defined("loud_level_patch"))
      mprintf("Want to patch %s from %d to %d\n",level,old_ver,new_ver);
#endif

   AutoAppIPtr(ObjectSystem);

   if (NEED_UPDATE("ops3.mis", 1))
   {
      // Make the glue elements of the massacre cutscene local-only
      MakeLocal(136, "Sound Trap");
      MakeLocal(1066, "Simple Button");
      MakeLocal(1067, "Sound Trap");
      MakeLocal(1068, "Trigger Delay");
      MakeLocal(1069, "Sound Trap");
      MakeLocal(1070, "Trigger Delay");
      MakeLocal(1071, "Sound Trap");
      MakeLocal(1072, "Simple Button");
      MakeLocal(1073, "Sound Trap");
      MakeLocal(1323, "Trigger Delay");
   }

   if (NEED_UPDATE("eng2.mis", 1))
   {
      // This exp cookie was somehow strangely set up
      ObjID obj = RealObj(934, "Small BP Pile");
      if (obj != OBJ_NULL)
         gPropAutoPickup->Set(obj, FALSE);

      // The Many ride needs to use the ChangeInterfaceMany script,
      // instead of ChangeInterface
      obj = RealObj(1563, "Marker");
      if (obj != OBJ_NULL)
      {
         AutoAppIPtr(PropertyManager);
         sScriptProp *pScripts;
         IScriptProperty *pScriptProperty =
            (IScriptProperty*)(pPropertyManager->GetPropertyNamed("Scripts"));
         if (pScriptProperty->Get(obj, &pScripts))
         {
            strcpy(pScripts->script[0], "ChangeInterfaceMany");
            pScriptProperty->Set(obj, pScripts);
         }
      }
   }

   if (NEED_UPDATE("ops4.mis", 1))
   {
      // Needs a multiplayer respawn marker; we just rename one of the
      // start markers
      ObjID obj = RealObj(343, "Marker");
      if (obj != OBJ_NULL)
         pObjectSystem->NameObject(obj, "Multiplayer Respawn");

      // These are cardslots for a pair of locked security doors. In
      // retrospect, the key for these is much too separated from the
      // doors themselves, and they're plot-blocking. So unlock them.
      obj = RealObj(620, "Card slot");
      if (obj != OBJ_NULL)
         ObjSetSelfLocked(obj, FALSE);
      obj = RealObj(619, "Card slot");
      if (obj != OBJ_NULL)
         ObjSetSelfLocked(obj, FALSE);
   }

   if (NEED_UPDATE("rec1.mis", 1))
   {
      // This is a Transmitter Tower -- actually a fancy keypad -- and
      // should be frobLocally
      ObjID obj = RealObj(89, "Transmitter Tower Off");
      if (obj != OBJ_NULL)
         SetFrobHandler(obj, kFrobHandlerLocal);
   }

   if (NEED_UPDATE("rick3.mis", 1))
   {
      // This is the "escape tunnel". When the shuttle launches to go to
      // the Many, we *actually* move the tunnel. But it wasn't set up
      // properly -- the thing needs the moving terrain property on it...
      ObjID obj = RealObj(120, "EscapeTunnel");
      if (obj != OBJ_NULL)
      {
         cMovingTerrainProp pMTProp;
         pMTProp.active = FALSE;
         pMTProp.prev_state = FALSE;
         g_pMovingTerrainProperty->Set(obj, &pMTProp);
      }
   }

   if (NEED_UPDATE("command2.mis", 1))
   {
      // This is the escape pod that leaves just as we try to get to it.
      // Again, it needs the moving terrain property set...
      ObjID obj = RealObj(301, "Escape Pod");
      if (obj != OBJ_NULL)
      {
         cMovingTerrainProp pMTProp;
         pMTProp.active = FALSE;
         pMTProp.prev_state = FALSE;
         g_pMovingTerrainProperty->Set(obj, &pMTProp);
      }
   }

   if (NEED_UPDATE("medsci1.mis", 1))
   {
      // The very first OG is controlled by a pseudoscript; handoff
      // interferes with that. The result is that, if the client sets
      // him off, the OG begins to run at him, then turns around and
      // forgets about him. Better to not hand off, and have poor aim,
      // than this dumbness.
      ObjID obj = RealObj(1383, "OG-Pipe");
      if (obj != OBJ_NULL)
      {
         g_pAINoHandoffProperty->Set(obj, TRUE);
      }
   }

   if (NEED_UPDATE("many.mis", 1))
   {
      // One of these spawn traps is way the heck too hard, and people
      // have been complaining about it. So switch it from generating
      // mostly midwives and a few rumblers, to just rumblers.
      ObjID obj = RealObj(1489, "RandMonsterGen");
      if (obj != OBJ_NULL)
      {
         sSpawnInfo *pSpawnInfo;
         if (gPropSpawn->Get(obj, &pSpawnInfo))
         {
            pSpawnInfo->m_odds[0] = 0;   // Chance of midwife
            pSpawnInfo->m_odds[1] = 100; // Chance of rumbler
            gPropSpawn->Set(obj, pSpawnInfo);
         }
      }
   }

   if (NEED_UPDATE("rec2.mis", 1))
   {
      // One of the LandingPoint markers, for player 3, is slightly
      // embedded in the wall. As a result, he can't move when he
      // teleports in. Shove it a foot in:
      ObjID obj = RealObj(268, "Marker");
      if (obj != OBJ_NULL)
      {
         ObjPos *pos = ObjPosGet(obj);
         if (pos)
         {
            // X is about 41.6; should be about 40.6:
            pos->loc.vec.x -= 1;
            ObjTranslate(obj, &(pos->loc.vec));
         }
      }
   }
}
