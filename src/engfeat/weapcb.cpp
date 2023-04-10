/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/engfeat/weapcb.cpp,v 1.9 1999/12/09 20:13:34 BODISAFA Exp $
//
// Weapon event callback system
//

#include <lg.h>
#include <comtools.h>
#include <appagg.h>
#include <dynarray.h>

#include <scrptapi.h>
#include <scrptbas.h>

#include <weapon.h>
#include <atkscrpt.h>

#include <weapcb.h>
#include <crwpnapi.h>
#include <crwpnlst.h>

// must be last header
#include <dbmem.h>

typedef struct 
{
   eWeaponEvent event;
   fWeaponEventCallback callback;
   void *data;
} sWeaponCallbackData;

cDynArray<sWeaponCallbackData> WeaponCallbackList;


void InitWeaponEventCallbacks()
{
   WeaponCallbackList.SetSize(0);
}

void TermWeaponEventCallbacks()
{
   WeaponCallbackList.SetSize(0);
}

BOOL RegisterWeaponEventCallback(eWeaponEvent events, fWeaponEventCallback callback, void *data)
{
   sWeaponCallbackData CallbackData;
   BOOL success = FALSE;

   for (int i=0; i<kNumWeaponEvents; i++)
   {
      if (events & (1 << i))
      {
         CallbackData.event = 1 << i;
         CallbackData.callback = callback;
         CallbackData.data = data;

         WeaponCallbackList.Append(CallbackData);

         success = TRUE;
      }
   }
   
   return success;
}

BOOL DeregisterWeaponEventCallback(eWeaponEvent events, fWeaponEventCallback callback)
{
   BOOL success = FALSE;

   for (int i=0; i<WeaponCallbackList.Size(); i++)
   {
      if ((WeaponCallbackList[i].event & events) && (WeaponCallbackList[i].callback == callback))
      {
         WeaponCallbackList.DeleteItem(i);
         i--;

         success = TRUE;
      }
   }

   return success;
}

void WeaponEvent(eWeaponEvent event, ObjID culprit, ObjID victim, eWeaponEventType event_type)
{
   // call any registered callbacks
   for (int i=0; i<WeaponCallbackList.Size(); i++)
   {
      if (WeaponCallbackList[i].event == event)
         WeaponCallbackList[i].callback(event, victim, culprit, WeaponCallbackList[i].data);
   }

   // do our weapon link thang, and send script messages
   switch (event)
   {
      case kStartAttack:
      {
         ObjID weapon; 
         EquipAIWeapon(culprit, &weapon, kWeaponModeMelee); 
         if (weapon != OBJ_NULL)
         {
            StartWeaponSwing(culprit, GetWeaponObjID(culprit), event_type);
         }
         sAttackMsg attack_msg(culprit, GetWeaponObjID(culprit), kAMT_StartAttack);

         AutoAppIPtr(ScriptMan);
         pScriptMan->SendMessage(&attack_msg);

         break;
      }

      case kEndAttack:
      {
         FinishWeaponSwing(culprit, GetWeaponObjID(culprit));
         sAttackMsg attack_msg(culprit, GetWeaponObjID(culprit), kAMT_EndAttack);

         AutoAppIPtr(ScriptMan);
         pScriptMan->SendMessage(&attack_msg);
         break;
      }

      case kStartWindup:
      {
         sAttackMsg attack_msg(culprit, GetWeaponObjID(culprit), kAMT_StartWindup);

         AutoAppIPtr(ScriptMan);
         pScriptMan->SendMessage(&attack_msg);
         break;
      }

      case kStartBlock:
      {
         ObjID weapon; 
         EquipAIWeapon(culprit, &weapon, kWeaponModeBlock); 
         StartWeaponSwing(culprit, GetWeaponObjID(culprit), kWeaponEventBlock);
         break;
      }

      case kEndBlock:
      {
         FinishWeaponSwing(culprit, GetWeaponObjID(culprit));
         break;
      }

      case kStartRangedAttack:
      {
         sAttackMsg attack_msg(culprit, OBJ_NULL, kAMT_StartAttack);
         
         // Switch the model if necessary.
         if (CreatureWeaponObjSwizzle)
         {
            ((*CreatureWeaponObjSwizzle)(culprit, kWeaponModeRanged));
         }

         AutoAppIPtr(ScriptMan);
         pScriptMan->SendMessage(&attack_msg);

         break;
      }

      case kEndRangedAttack:
      {
         sAttackMsg attack_msg(culprit, OBJ_NULL, kAMT_EndAttack);

         AutoAppIPtr(ScriptMan);
         pScriptMan->SendMessage(&attack_msg);
         break;
      }

      case kStartRangedWindup:
      {
         sAttackMsg attack_msg(culprit, OBJ_NULL, kAMT_StartWindup);

         AutoAppIPtr(ScriptMan);
         pScriptMan->SendMessage(&attack_msg);
         break;
      }

   }
}






