/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactjsc.h,v 1.8 1999/05/10 10:45:21 TOML Exp $
//
// AI joint scan
//

#ifndef __AIACTJSC_H
#define __AIACTJSC_H

#include <aiapiiai.h>
#include <aibasact.h>
#include <schtype.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointScanAction
//
// Scan a joint (oscillate from one angle to another)
//

typedef int eAIJointScanState;
enum eAIJointScanState_ {kAIJointScanStateOne, kAIJointScanStateTwo,};

class cAIJointScanAction : public cAIAction
{
public:
   cAIJointScanAction(IAIActor * pOwner, DWORD data = 0);
   virtual ~cAIJointScanAction();

   static void HandleNetMessage(IAI *pAI, void *netmsg);

   // Set up action parameters
   void Set(int jointID, floatang targetAng, floatang targetAng2, floatang ScanSpeed, floatang ScanSpeed2);

   // Update the action
   STDMETHOD_(eAIResult, Update)();

   // Start the action
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);

   // End action
   STDMETHOD_(eAIResult, End)();

protected:
   // Broadcast the action in a multi-player game.
   void BroadcastAction(ulong deltaTime);

   // Broadcast the halt in a multi-player game.
   void BroadcastHalt();

   // Switch the direction of rotation for a network proxy.
   void SwitchProxyDirection(int old_state);

   // Broadcast a switch in the directions, to keep all network copies in synch.
   void BroadcastSwitchDirections();

   // Kill any rotation noise
   void KillRotateSchema(void);
   
   // Networking update & enact, return TRUE if fully handled
   BOOL NetUpdate();
   BOOL NetEnact(ulong);
   
   BOOL IsHostedHere();

private:
   eAIJointScanState m_state;
   int m_jointID;
   BOOL m_bProxyWait;  // Is this a network proxy, waiting at a targetAng for a net message.
   floatang m_targetAng;
   floatang m_targetAng2;
   floatang m_ScanSpeed;
   floatang m_ScanSpeed2;
   int m_schemaHandle;

   friend void SchemaEndCallback(int hSchema, ObjID schemaID, void *pData);
};

////////////////////////////////////////

inline cAIJointScanAction::cAIJointScanAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_JointScan, pOwner, data),
   m_schemaHandle(SCH_HANDLE_NULL)
{
}

static void SchemaEndCallback(int hSchema, ObjID schemaID, void *pData);

#pragma pack()

#endif /* !__AIACTJSC_H */









