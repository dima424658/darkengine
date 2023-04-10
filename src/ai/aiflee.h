/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiflee.h,v 1.9 2000/03/06 22:36:30 bfarquha Exp $
//
//
//

#ifndef __AIFLEE_H
#define __AIFLEE_H

#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

struct sAIFleeDest;
struct sAIFleeConditions;

///////////////////////////////////////////////////////////////////////////////
//
// Publish ability and all associated link and property types
//

BOOL AIInitFleeAbility(IAIManager *);
BOOL AITermFleeAbility();

// Return TRUE if this Id is already someone's flee destination.
BOOL AIFleeIsCurrentDest(ObjID dest);

struct IIntProperty;
EXTERN IIntProperty *              g_pAIFleePointProperty;

#define MAX_FLEEPOINTS 256

struct IAIPath;


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFlee
//

class cAIFlee : public cAIAbility
{
public:
   cAIFlee();
   ~cAIFlee();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   // Notifications
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);

   STDMETHOD_(void, OnAwareness)(ObjID source,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous,
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);

   // pass in object that is frustrating us.
   STDMETHOD_(void, OnFrustration)(ObjID source, ObjID dest, IAIPath *pPath);

   STDMETHOD_(void, OnDamage)(const sDamageMsg * pMsg, ObjID realCulpritID);
   STDMETHOD_(void, OnWeapon)(eWeaponEvent ev, ObjID victim, ObjID culprit);

   // Ability flow
   void DoSuggestGoal(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD_(BOOL, FinalizeGoal)(cAIGoal *pGoal);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

protected:
   BOOL CheckHitpointsFlee(const sAIFleeConditions * pConditions);
   BOOL CheckFriendsNotFlee(const sAIFleeConditions * pConditions);
   BOOL IsFleeing();
   void ActSurprised(cAIActions * pNew);
   BOOL RunAway(cAIActions *pNew, ObjID object, BOOL freshen);

   // Quick checks to see if we can path to patrol point, without pathfinding
   BOOL IsInUnpathableRegion(ObjID Obj);
   void MarkUnpathable(ObjID Obj);

   ObjID GetFleeSource();
   void SetFleeSource(ObjID);

   ObjID PickFleePoint(ObjID);
   const sAIFleeDest * GetFleeDest();
   void SetFleeDest(ObjID);
   void FlagFleeDestReached();
   BOOL GetFleeDestReached();
   unsigned GetFleeDestExpiration();
   void SetFleeDestExpiration(unsigned);
   void StopFlee();

   void ResetFleePointList();
   void InsertFleePoint(ObjID FleePointId, float fDist2, int nStartIx, int nEndIx);

   cAITimer       m_CheckTimer;
   cAIRandomTimer m_BroadcastTimer;

   ObjID m_ExplicitDestId;
   BOOL bFiguringDest;
   int nLastFailedGoalTime;
   cDynArray<int> m_UnpathableRegions;
   IAIPath *m_pPath;
   IAIPath *m_pExplicitPath;
   int m_nNumFleePoints;
   ObjID m_SortedFleePoints[MAX_FLEEPOINTS];
   BOOL bWasDamage;
   ObjID nFleeSource;
   BOOL bDoingInvestigate;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIFLEE_H */
