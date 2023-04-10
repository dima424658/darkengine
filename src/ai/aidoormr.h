/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidoormr.h,v 1.6 2000/02/28 11:30:30 toml Exp $
//
//
//

#ifndef __AIDOORMR_H
#define __AIDOORMR_H

#include <aibasmrg.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

enum eAIDoorLinkFlags
{
   kWasAIOpened = 1<<0,
   kWasAIUnlocked = 1<<1,
};

///////////////////////////////////////////////////////////////////////////////

BOOL AIInitDoorMovReg(IAIManager *);
BOOL AITermDoorMovReg();

///////////////////////////////////////////////////////////////////////////////

class cAIDoorMovReg : public cAIMoveRegulator
{
public: 
   cAIDoorMovReg();
   
   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(BOOL, AssistGoal)(const sAIMoveGoal & goal, sAIMoveGoal * pResultGoal);

   STDMETHOD_(BOOL, WantsInterrupt)();
private:
   cAITimer m_DoorLookPeriod;
   cAITimer m_DoorWait;
   BOOL     m_fWaiting;
};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIDOORMR_H */
