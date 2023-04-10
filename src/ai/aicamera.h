/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aicamera.h,v 1.4 2000/02/22 16:04:06 adurant Exp $
//
// AI Camera Ability
//
// A camera is a device that scans an arc and broadcasts link messages down all
// attached AICamera links when it is alerted. The data on the link is used to
// derive a message & multi-parm data.

#ifndef __AICAMERA_H
#define __AICAMERA_H

#include <aidev.h>
#include <aiactjsc.h>

// I have to include all this just to get to the definition of the linkkind class
#include <comtools.h>
#include <scrptsrv.h>
#include <linkscpt.h>

#pragma once
#pragma pack(4)

class cAICamera : public cAIDevice
{
public:
   // Create, specify which links to alert & what message to broadcast
   cAICamera(void);
   ~cAICamera();

   // Standard component methods
   STDMETHOD_(const char *, GetName)();

   // Goals
   STDMETHOD (SuggestGoal)(cAIGoal * pPrevious, cAIGoal ** ppNew);

   // Ability flow
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew);

   // Notification
   STDMETHOD_(void, OnAlertness)(ObjID source,
                                 eAIAwareLevel previous, 
                                 eAIAwareLevel current,
                                 const sAIAlertness * pRaw);
   
   STDMETHOD_(void, OnModeChange)(eAIMode previous, eAIMode mode);

private:
   void BroadcastSwitches(void);
};

// link data
struct sAICameraLinkData
{
   char m_msg[32];      // script message
   char m_data[3][32]; // multi-parm args
};

// Init/term
BOOL AIInitCameraAbility(IAIManager *);
BOOL AITermCameraAbility(void);

#pragma pack()

#endif /* !__AICAMERA_H */
