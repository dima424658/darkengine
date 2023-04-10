/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/airecovr.h,v 1.1 1998/10/03 11:24:05 TOML Exp $
//
// Ability that attempts to recover from an out-of-world situation
//

#ifndef __AIRECOVR_H
#define __AIRECOVR_H

#include <aiapicmp.h>
#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIRecover
//

class cAIRecover : public cAIAbility
{
public:
   cAIRecover();
   ~cAIRecover();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(void, Init)();

   STDMETHOD_(void, OnBeginFrame)(const cAIGoal * pPrevious, const cAIGoal * pGoal);
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   // Get the "best" watch point
   BOOL SeekTarget();
   BOOL CheckTrigger(sAIWatchPoint *);
   void StopCurrent();
   
   static void LGAPI LinkListenFunc(sRelationListenMsg* msg, RelationListenerData data);
   friend BOOL AIInitWatchAbility(IAIManager *);
   
   // Failure timeout timer
   cAITimer m_Timer;
   
   // Current watch
   sAIWatchPoint * m_pWatch;
   
   
   static BOOL gm_fSelfLinkEdit;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIRECOVR_H */
