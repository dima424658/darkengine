/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#ifndef __DPCAIBIT_H
#define __DPCAIBIT_H

#include <aiactloc.h>
#include <stimtype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIBiteAction
//

enum eAIBiteActionState {kAIBiteActionStart, kAIBiteActionLeaping, kAIBiteActionBitten,};

class cAIBiteAction : public cAIAction
{ 
 public:
   cAIBiteAction(IAIAbility* pOwner, DWORD data);

   void Set(ObjID object, float biteDist, StimID stimID, tStimLevel intensity, float leapSpeedX, float leapSpeedZ);

   STDMETHOD_(eAIResult, Update)();

   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

 private:
   eAIBiteActionState m_state;
   ObjID              m_targetObjID;
   float              m_biteDist;      // Distance at which we can bite
   
   ObjID              m_stimID;
   tStimLevel         m_intensity;
   
   float              m_leapSpeedX;
   float              m_leapSpeedZ;

   void Bite(void);
   void Leap(void);

   inline void SetState(eAIBiteActionState state) {m_state = state;};
};

///////////////////////////////////////

inline cAIBiteAction::cAIBiteAction(IAIAbility * pOwner, DWORD data)
 : cAIAction(kAIAT_Bite, pOwner, data)
{
}

///////////////////////////////////////

inline void cAIBiteAction::Set(ObjID object, float biteDist, StimID stimID, tStimLevel intensity, float leapSpeedX, float leapSpeedZ)
{
   m_state       = kAIBiteActionStart;
   m_targetObjID = object;
   m_biteDist    = biteDist;
   m_stimID      = stimID;
   m_intensity   = intensity;
   m_leapSpeedX  = leapSpeedX;
   m_leapSpeedZ  = leapSpeedZ;
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__SHKAIBIT_H */
