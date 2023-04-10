/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aitrig.h,v 1.2 1998/10/06 15:03:25 TOML Exp $
//
//
//

#ifndef __AITRIG_H
#define __AITRIG_H

#include <aiapicmp.h>
#include <aibasabl.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

struct sAIPsdScrAct;
struct sAIPsdScrActs;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAITriggeredPScripted
//

class cAITriggeredPScripted : public cAIAbility
{
public:
   cAITriggeredPScripted(eAITimerPeriod testRate, unsigned SaveLoadSubtag = 0);
   ~cAITriggeredPScripted();

   // Standard component methods
   STDMETHOD_(void, Init)();

   //
   // Save/load. Note that this base class uses "additional id" 1 for its tag
   //
   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);
   STDMETHOD_(BOOL, SaveGoal)(ITagFile * pTagFile, cAIGoal * pGoal);
   STDMETHOD_(BOOL, LoadGoal)(ITagFile * pTagFile, cAIGoal ** ppGoal);

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestGoal)(cAIGoal *, cAIGoal **);
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

protected:
   void SetTriggered(eAIPriority, const sAIPsdScrAct *, unsigned nActs);
   BOOL IsTriggered() const;
   void ClearTrigger();

private:
   cAITimer            m_Timer;
   sAIPsdScrActs *     m_pActs;
   unsigned            m_SaveLoadSubtag;
};

///////////////////////////////////////

inline BOOL cAITriggeredPScripted::IsTriggered() const
{
   return !!m_pActs;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AITRIG_H */
