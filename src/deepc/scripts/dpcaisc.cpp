/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcaisc.h>

#include <appagg.h>

#include <aiman.h>
#include <aistnapi.h>

#include <scrptapi.h>
#include <scrptsrv.h>
#include <scrptbas.h>

#include <lockprop.h>

#include <config.h>

extern BOOL ValidateCreaturePosition(ObjID objID);

// must be last header
#include <dbmem.h>

DECLARE_SCRIPT_SERVICE_IMPL(cDPCAISrv, DPCAI)
{
public:
   STDMETHOD_(BOOL, Stun)(object who, string startTags, string loopTags, float sec)
   {
      return DoAISetStun(who, (char*)(const char*)startTags, (char*)(const char*)loopTags, sec*1000);
   }
   STDMETHOD_(BOOL, IsStunned)(object who)
   {
      return AIGetStun(who);
   }
   STDMETHOD_(BOOL, UnStun)(object who)
   {
      return DoAIUnsetStun(who);
   }

   STDMETHOD_(BOOL, Freeze)(object who, float sec)
   {
      AutoAppIPtr(AIManager);
      IAI* pAI;
      
      if ((pAI = pAIManager->GetAI(who)) == NULL)
         return FALSE;
      pAI->Freeze(sec*1000);
      pAI->Release();
      return TRUE;
   }
   STDMETHOD_(BOOL, IsFrozen)(object who)
   {
      Warning(("IsFrozen: not implemented yet\n"));
      return FALSE;
   }
   STDMETHOD_(BOOL, UnFreeze)(object who)
   {
      AutoAppIPtr(AIManager);
      IAI* pAI;
      
      if ((pAI = pAIManager->GetAI(who)) == NULL)
         return FALSE;
      pAI->Unfreeze();
      pAI->Release();
      return TRUE;
   }

   STDMETHOD_(BOOL, Mace)(object who, string startTags, string loopTags, float sec)
   {
      return DoAISetStun(who, (char*)(const char*)startTags, (char*)(const char*)loopTags, sec*1000);
   }
   STDMETHOD_(BOOL, IsMaced)(object who)
   {
      return AIGetStun(who);
   }
   STDMETHOD_(BOOL, UnMace)(object who)
   {
      return DoAIUnsetStun(who);
   }

   STDMETHOD_(void, NotifyEnterTripwire)(object who, object what)
   {
      AutoAppIPtr(AIManager);
      IAI* pAI;
      
      if ((pAI = pAIManager->GetAI(who)) != NULL)
      {
         pAI->NotifyTripwire(what, TRUE);
         pAI->Release();
      }
   }
   STDMETHOD_(void, NotifyExitTripwire)(object who, object what)
   {
      AutoAppIPtr(AIManager);
      IAI* pAI;
      
      if ((pAI = pAIManager->GetAI(who)) != NULL)
      {
         pAI->NotifyTripwire(what, FALSE);
         pAI->Release();
      }
   }
   STDMETHOD_(BOOL, ObjectLocked)(object obj) 
   {
      return ObjSelfLocked(obj);
   }
   STDMETHOD_(void, ValidateSpawn)(object creature, object spawnMarker) 
   {
#ifndef SHIP
      if (!ValidateCreaturePosition(creature))
      {
         if (config_is_defined("spawn_assert"))
            CriticalMsg2("Creature spawn point %d caused the bad cret pos for %d", ObjID(spawnMarker), ObjID(creature));
         else
            Warning(("Creature spawn point %d caused the bad cret pos for %d\n", ObjID(spawnMarker), ObjID(creature)));
      }
#endif
   }
};

IMPLEMENT_SCRIPT_SERVICE_IMPL(cDPCAISrv, DPCAI);
