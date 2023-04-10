/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// AI Pathfinding.

#include <dpcaipth.h>

#include <appagg.h>

#include <aiprops.h>

#include <autolink.h>
#include <doorphys.h>
#include <linkman.h>
#include <lockprop.h>
#include <propman.h>
#include <questapi.h>
#include <relation.h>

#include <dpcfrob.h>
#include <dpcprop.h>

#include <dbmem.h>

#define AIUsesDoors(obj)   AIGetProperty(g_pAIUsesDoorsProperty, (obj), TRUE)

static IBoolProperty* g_pAIUsesDoorsProperty = NULL;
static IRelation* g_pSwitchLinkRelation = NULL;

///////////////////////////////////////

void DPCAITermPathfinder(void)
{
   SafeRelease(g_pAIUsesDoorsProperty);
}

///////////////////////////////////////

STDMETHODIMP_(const char *) cDPCAIPathfinder::GetName()
{
   return "DPC pathfinding component";
}

////////////////////////////////////////
//
// A hack, for sure
//

inline BOOL DPCAIIsTripwire(ObjID objID)
{
   return gPropTripFlags->IsRelevant(objID);
}

////////////////////////////////////////
//
// Is this object a quest filter that should currently be "locking" this door?
// This is really a hack, but we assume that anything with the QB Name property on 
// it is a QuestFilter.
//

BOOL cDPCAIPathfinder::QuestBlock(ObjID objID) const
{
   const char* pQBName;
   int QBVal;
   int val;

   // Start by checking for quest props on obj
   if (!gPropQBName->Get(objID, &pQBName))
      return FALSE;
   gPropQBVal->Get(objID, &QBVal);
   AutoAppIPtr(QuestData);
   val = pQuestData->Get(pQBName);
   if (val<=QBVal)
      return TRUE;
   return FALSE;
}


///////////////////////////////////////
// 
// Recursive search back along switch links to specified depth.
// This function returns the result in the openable parameter.
// It is defensive in the sense that it will return FALSE if any lock
// or unset quest bit is encountered in the search. It will only return
// true if it encounters a tripwire and no locks or unset quest bits.
// The actual function return value is used to specify whether to continue
// searching or not.
// The initial value of the openable parameter is assumed to be FALSE.
//

BOOL cDPCAIPathfinder::DoorOpenable(ObjID objID, int depth, BOOL& openable)
{
   cAutoLinkQuery query(g_pSwitchLinkRelation, LINKOBJ_WILDCARD, objID);

   if (depth<=0)
      return TRUE;

   while (!query->Done())
   {
      ObjID sourceID = query.GetSource();
      int keypadCode;

      // Check for impassable. 
      // Assume that anything with a keypad code property on it is a keypad
      if (ObjSelfLocked(sourceID) || QuestBlock(sourceID) || g_pPropKeypadCode->Get(sourceID, &keypadCode))
      {
         openable = FALSE;
         return FALSE;
      }
      if (DPCAIIsTripwire(sourceID))
         openable = TRUE;
      if ((depth>1) && !DoorOpenable(sourceID, depth-1, openable))
         return FALSE;
      query->Next();
   }
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cDPCAIPathfinder::CanPassDoor(ObjID door)
{
   BOOL openable;

   // This is kind of stupid, but avoids ordering issues with init functions (& the AI)
   if (g_pAIUsesDoorsProperty == NULL)
   {
      AutoAppIPtr(PropertyManager);
      g_pAIUsesDoorsProperty = (IBoolProperty*)pPropertyManager->GetPropertyNamed("AI_UsesDoors");
      Assert_(g_pAIUsesDoorsProperty);
   }
   if (g_pSwitchLinkRelation == NULL)
   {
      AutoAppIPtr(LinkManager);
      g_pSwitchLinkRelation = pLinkManager->GetRelationNamed("SwitchLink");
      Assert_(g_pSwitchLinkRelation);
   }

   if (!AIUsesDoors(GetID()))
      return FALSE;

   if (!IsDoor(door) || GetDoorStatus(door) == kDoorOpen || GetDoorStatus(door) == kDoorOpening)
      return TRUE;

   // just do 2 deep search for now
   openable = FALSE;
   DoorOpenable(door, 2, openable);
   
   // The idea is that any unlocked, non-questy, closed door should be openable.
   // In Shock, everything had to be hooked to a tripwire (tripped by either the AI or the player).
   // This is no longer the case in DeepC.  I'm leaving in support for Shock-style while allowing the Thief-style.
   if (!openable                           &&
      (GetDoorStatus(door) == kDoorClosed) &&
      (!ObjSelfLocked(door))               &&
      (!QuestBlock(door)))
   {
      return TRUE;
   }
   return openable;
}

