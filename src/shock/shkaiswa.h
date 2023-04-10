/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiswa.h,v 1.2 1998/11/23 09:14:08 JON Exp $
//

#ifndef __SHKAISWA_H
#define __SHKAISWA_H

#include <aicombat.h>
#include <ctagset.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISwarm
//

enum eAISwarmMode {kAISwM_Close = 0, kAISwM_BackOff = 1,};

class cAISwarm : public cAICombat
{
public:   
   cAISwarm();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   STDMETHOD_(void, Init)();

   STDMETHOD_(BOOL, Save)(ITagFile *);
   STDMETHOD_(BOOL, Load)(ITagFile *);

   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   
   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

private:
   eAISwarmMode m_mode;
   cMxsVector m_gotoLoc;   // for backing off
   cAIRandomTimer m_giveupTimer;

   void SetMode(eAISwarmMode mode);
   cAILocoAction* CreateBackoffAction(float distance);
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAISWA_H */
