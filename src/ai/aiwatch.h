/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiwatch.h,v 1.4 1998/11/18 16:22:04 MROWLEY Exp $
//
//
//

#ifndef __AIWATCH_H
#define __AIWATCH_H

#include <linktype.h>

#include <aiapicmp.h>
#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

struct sAIWatchPoint;

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitWatchAbility(IAIManager *);
BOOL AITermWatchAbility();

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIWatch
//

class cAIWatch : public cAIAbility
{
public:
   cAIWatch();
   ~cAIWatch();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   // Get the "best" watch point
   BOOL SeekTarget();
   BOOL CheckTrigger(sAIWatchPoint *);
   BOOL CheckTriggerForObj(sAIWatchPoint * pWatch, ObjID obj, ObjID senseObj);
   void StopCurrent();
   
   static void LGAPI LinkListenFunc(sRelationListenMsg* msg, RelationListenerData data);
   friend BOOL AIInitWatchAbility(IAIManager *);
   
   // Intrusion check frequency
   cAITimer m_Timer;
   
   // Current watch
   sAIWatchPoint * m_pWatch;
   
   static BOOL gm_fSelfLinkEdit;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIWATCH_H */
