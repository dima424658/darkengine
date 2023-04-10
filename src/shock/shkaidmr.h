/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaidmr.h,v 1.3 1999/04/01 13:54:11 JON Exp $
//
//
//

#ifndef __SHKAIDMR_H
#define __SHKAIDMR_H

#include <aibasmrg.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cShockAIDoorMovReg : public cAIMoveRegulator
{
public: 
   cShockAIDoorMovReg();
   
   STDMETHOD_(void, Init)();
   STDMETHOD_(void, End)();

   STDMETHOD_(const char *, GetName)();
   STDMETHOD_(BOOL, AssistGoal)(const sAIMoveGoal & goal, sAIMoveGoal * pResultGoal);

   STDMETHOD_(void, OnCollision)(const sPhysListenMsg *);
   STDMETHOD_(void, OnTripwire)(ObjID objID, BOOL enter);

   STDMETHOD_(BOOL, WantsInterrupt)();

private:
   void OpenDoor(ObjID doorID);

   cAITimer m_doorWait;
   BOOL m_waiting;
   BOOL m_wantsInterrupt;
   ObjID m_doorID;
};

///////////////////////////////////////////////////////////////////////////////

extern void ShockAIInitDoorMovReg(void);
extern void ShockAITermDoorMovReg(void);

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIDMR_H */
