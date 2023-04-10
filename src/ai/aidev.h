/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aidev.h,v 1.7 1999/11/23 19:07:11 adurant Exp $
//
//
//

#ifndef __AIDEV_H
#define __AIDEV_H

#include <aicombat.h>
#include <aiactjr.h>
#include <aiprdev.h>
#include <rendprop.h>

#pragma once
#pragma pack(4)

typedef int eAIDeviceState;
enum eAIDeviceState_ {kAIDeviceInactive, kAIDeviceActive, kAIDeviceActivating, kAIDeviceDeactivating};

class cAIDevice : public cAICombat
{
public:
   cAIDevice();
   ~cAIDevice();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   // Goals
   STDMETHOD (SuggestGoal)(cAIGoal * pPrevious, cAIGoal ** ppNew);
   STDMETHOD_(void, OnGoalChange)(const cAIGoal * pPrevious, const cAIGoal * pGoal);

   // Notifications
   STDMETHOD_(void, OnActionProgress)(IAIAction * pAction);
   STDMETHOD_(void, OnAlertness)(ObjID source, eAIAwareLevel previous,  eAIAwareLevel current, const sAIAlertness * pRaw);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   STDMETHOD_(eAIDeviceState, GetDeviceState)(void) const {return m_state;};
   STDMETHOD_(int, GetRotateJoint)(void) const {return (AIGetDeviceParams(m_pAIState->GetID())->m_jointRotate);};
   
protected:
   virtual void SetState(eAIDeviceState state);

   floatang FacingDelta(ObjID target);

private:
   BOOL IsDoing(const cAIActions & previous, tAIActionType type);
   cAIJointRotateAction *m_pRotateAction;
   eAIDeviceState m_state;

   HRESULT ActiveSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
   HRESULT DeactivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);
   HRESULT ActivateSuggestActions(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   BOOL IsValidJoint(int jointNum) const {return ((jointNum>=0) && (jointNum<MAX_REND_JOINTS));}

   BOOL ShouldChangeFacing(void);

   BOOL ReadyToDeactivate(void);
   BOOL ReadyToActivate(void);
};



#pragma pack()

#endif /* !__AICBTRNG_H */
