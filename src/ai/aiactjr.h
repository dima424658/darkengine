/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactjr.h,v 1.9 1999/11/19 20:51:27 adurant Exp $
//
// AI joint rotate
//

#ifndef __AIACTJR_H
#define __AIACTJR_H

#include <aiapiiai.h>
#include <aibasact.h>
#include <schtype.h>

#include <objnotif.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointRotateAction
//
// Rotate a joint (don't move the object)
//

class cAIJointRotateAction : public cAIAction
{
public:
   cAIJointRotateAction(IAIActor * pOwner, DWORD data = 0);
   cAIJointRotateAction(IAI *pAI, void *netmsg);
   virtual ~cAIJointRotateAction();

   // Set up action parameters
   void Set(int jointID, floatang targetAng, floatang epsilonAng = 0.01);
   void Set(int jointID, ObjID targetObj, floatang epsilonAng = 0.01);

   ObjID GetTarget(void) const {return m_targetID;}
   floatang GetTargetAng(void) const {return m_targetAng;}
   int GetJointID(void) const {return m_jointID;}
   void SetTarget(ObjID targetID) {m_targetID = targetID;}

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   // End action
   STDMETHOD_(eAIResult, End)();

private:
   int m_jointID;
   ObjID m_targetID;
   floatang m_targetAng;
   floatang m_rotateSpeed;
   floatang m_epsilonAng;
   int m_schemaHandle;
   tObjListenerHandle m_listener;

   // Broadcast the action in a multi-player game.
   void BroadcastAction(ulong deltaTime);
   // Kill rotation schema
   void KillRotateSchema(void);
   // Start rotate schema
   void StartRotateSchema(void);

   friend void SchemaEndCallback(int hSchema, ObjID schemaID, void *pData);
};

EXTERN void AIJointRotateObjListener(ObjID objID, eObjNotifyMsg msg, void *data);

////////////////////////////////////////

inline cAIJointRotateAction::cAIJointRotateAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_JointRotate, pOwner, data),
   m_targetID(OBJ_NULL),
   m_schemaHandle(SCH_HANDLE_NULL),
   m_listener(NULL)
{
}

static void SchemaEndCallback(int hSchema, ObjID schemaID, void *pData);

#pragma pack()

#endif /* !__AIACTJR_H */









