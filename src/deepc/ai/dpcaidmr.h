/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//
//

#ifndef __DPCAIDMR_H
#define __DPCAIDMR_H

#include <aibasmrg.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

class cDPCAIDoorMovReg : public cAIMoveRegulator
{
public: 
   cDPCAIDoorMovReg();
   
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

extern void DPCAIInitDoorMovReg(void);
extern void DPCAITermDoorMovReg(void);

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__DPCAIDMR_H */
