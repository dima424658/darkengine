/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactnet.cpp,v 1.4 2000/02/11 18:27:18 bfarquha Exp $
//
// Repository for all AI action networking code
//

// #define PROFILE_ON 1

#include <lg.h>
#include <appagg.h>
#include <cfgdbg.h>

#include <netman.h>
#include <iobjnet.h>
#include <traitman.h>
#include <netprops.h>
#include <objdef.h>

#include <aiapinet.h>
#include <aiactjr.h>
#include <aiactjs.h>
#include <aiactjsc.h>
#include <aiactlch.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointRotateAction
//

struct sAINetMsg_JointRotate_NG
{
   sAINetMsg_Header aihdr;
   ulong deltaTime;
   int jointID;
   floatang rotateSpeed;
   floatang targetAng;
   sGlobalObjID targetGID;
   floatang epsilonAng;
};

///////////////////////////////////////
//
// Constructor used when receiving a network message for this action.
//

cAIJointRotateAction::cAIJointRotateAction(IAI *pAI, void *pMsg):
   cAIAction(kAIAT_JointRotate, NULL, 0),
   m_schemaHandle(SCH_HANDLE_NULL)
{
   AutoAppIPtr_(NetManager, pNetMan);
   sAINetMsg_JointRotate_NG *netmsg = (sAINetMsg_JointRotate_NG *)pMsg;

   ConfigSpew("net_ai_spew", ("NET AI: Received joint rotate for %d\n", netmsg->aihdr.aiObj));

   m_targetID = pNetMan->FromGlobalObjID(&netmsg->targetGID);
   m_jointID = netmsg->jointID;
   m_targetAng = netmsg->targetAng;
   m_rotateSpeed = netmsg->rotateSpeed;
   m_epsilonAng = netmsg->epsilonAng;
   AutoAppIPtr(ObjectSystem);
   if ((m_targetID != OBJ_NULL) &&
       (pObjectSystem->Exists(m_targetID)))
   {
      // Listen for target destruction
      sObjListenerDesc desc = {AIJointRotateObjListener, (void*)this};
      m_listener = pObjectSystem->Listen(&desc);
   } else {
      m_listener = NULL;
   }

   // Since it has no pOwner ability, the cAIAction constructor didn't init m_pAI.
   m_pAI = (IInternalAI *)pAI;
   m_pAIState = m_pAI->GetState();

   // Verify that we are dealing with a network proxy, then start the action.
   Assert_(m_pAI->IsNetworkProxy());
   m_pAI->StartProxyAction(this, netmsg->deltaTime);

}

///////////////////////////////////////

void cAIJointRotateAction::BroadcastAction(ulong deltaTime)
{
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);

   if (!Started() && pNetMan->Networking() && pObjNet->ObjHostedHere(m_pAIState->GetID()))
   {
      // Broadcast the action.
      ConfigSpew("net_ai_spew", ("NET AI: Send joint rotate for %d\n", m_pAIState->GetID()));
      AutoAppIPtr_(AINetManager, pAINetMan);
      AutoAppIPtr_(NetManager, pNetMan);
      sAINetMsg_Header hdr = {pAINetMan->NetMsgHandlerID(),m_pAIState->GetID(),FALSE,kAIAT_JointRotate};
      sAINetMsg_JointRotate_NG netmsg;
      netmsg.aihdr = hdr;
      netmsg.deltaTime = deltaTime;
      netmsg.jointID = m_jointID;
      netmsg.rotateSpeed = m_rotateSpeed;
      netmsg.targetAng = m_targetAng;
      netmsg.targetGID = pNetMan->ToGlobalObjID(m_targetID);
      netmsg.epsilonAng = m_epsilonAng;
      pNetMan->Broadcast((void *)&netmsg, sizeof(netmsg), FALSE);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointSlideAction
//

struct sAINetMsg_JointSlide_NG
{
   sAINetMsg_Header aihdr;
   ulong deltaTime;
   int jointID;
   float target;
   float speed;
};

///////////////////////////////////////
//
// Constructor used when receiving a network message to Enact this action type.
//

cAIJointSlideAction::cAIJointSlideAction(IAI *pAI, void *pMsg):
   cAIAction(kAIAT_JointSlide, NULL, 0)
{
   AutoAppIPtr_(NetManager, pNetMan);
   sAINetMsg_JointSlide_NG *netmsg = (sAINetMsg_JointSlide_NG *)pMsg;

   ConfigSpew("net_ai_spew", ("NET AI: Received joint slide for %d\n", netmsg->aihdr.aiObj));

   m_jointID = netmsg->jointID;
   m_target = netmsg->target;
   m_speed = netmsg->speed;

   // Since it has no pOwner ability, the cAIAction constructor didn't init m_pAI.
   m_pAI = (IInternalAI *)pAI;
   m_pAIState = m_pAI->GetState();

   // Verify that we are dealing with a network proxy, then start the action.
   Assert_(m_pAI->IsNetworkProxy());
   m_pAI->StartProxyAction(this, netmsg->deltaTime);
}

///////////////////////////////////////

void cAIJointSlideAction::BroadcastAction(ulong deltaTime)
{
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);

   if (!Started() && pNetMan->Networking() && pObjNet->ObjHostedHere(m_pAIState->GetID()))
   {
      // Broadcast the action, non-guaranteed ("_NG" by convention).
      ConfigSpew("net_ai_spew", ("NET AI: Send joint slide for %d\n", m_pAIState->GetID()));
      AutoAppIPtr_(AINetManager, pAINetMan);
      AutoAppIPtr_(NetManager, pNetMan);
      sAINetMsg_Header hdr = {pAINetMan->NetMsgHandlerID(),m_pAIState->GetID(),FALSE,kAIAT_JointSlide};
      sAINetMsg_JointSlide_NG netmsg;
      netmsg.aihdr = hdr;
      netmsg.deltaTime = deltaTime;
      netmsg.jointID = m_jointID;
      netmsg.speed = m_speed;
      netmsg.target = m_target;
      pNetMan->Broadcast((void *)&netmsg, sizeof(netmsg), FALSE);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointScanAction
//

struct sAINetMsg_JointScan_NG
{
   sAINetMsg_Header aihdr;
   ubyte justSwitchDirections;  // Must be first field after aihdr
   ubyte state;
   ulong deltaTime;
   int jointID;
   floatang ScanSpeed;
   floatang ScanSpeed2;
   floatang targetAng;
   floatang targetAng2;
};

///////////////////////////////////////

void cAIJointScanAction::BroadcastHalt()
{
#ifdef NEW_NETWORK_ENABLED
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);
   AutoAppIPtr_(AINetManager, pAINetMan);
   if (pNetMan->Networking() && pObjNet->ObjHostedHere(m_pAIState->GetID()))
   {
      sAINetMsg_Header netmsg = {pAINetMan->NetMsgHandlerID(),m_pAIState->GetID(),TRUE,kAIAT_JointScan};
      pNetMan->Broadcast((void *)&netmsg, sizeof(netmsg), FALSE);
   }
#endif
}

///////////////////////////////////////

void cAIJointScanAction::BroadcastAction(ulong deltaTime)
{
   // Broadcast the action.
   ConfigSpew("net_ai_spew", ("NET AI: Send joint scan for %d\n", m_pAIState->GetID()));
   AutoAppIPtr_(AINetManager, pAINetMan);
   AutoAppIPtr_(NetManager, pNetMan);
   sAINetMsg_Header hdr = {pAINetMan->NetMsgHandlerID(),m_pAIState->GetID(),FALSE,kAIAT_JointScan};
   sAINetMsg_JointScan_NG netmsg;
   netmsg.aihdr = hdr;
   netmsg.justSwitchDirections = FALSE;
   netmsg.deltaTime = deltaTime;
   netmsg.jointID = m_jointID;
   netmsg.state = m_state;
   Assert_(netmsg.state == m_state);  // check that there was no truncation.
   netmsg.ScanSpeed = m_ScanSpeed;
   netmsg.ScanSpeed2 = m_ScanSpeed2;
   netmsg.targetAng = m_targetAng;
   netmsg.targetAng2 = m_targetAng2;
   pNetMan->Broadcast((void *)&netmsg, sizeof(netmsg), FALSE);
}

///////////////////////////////////////


void cAIJointScanAction::HandleNetMessage(IAI *pAI, void *pMsg)
{
   cAIJointScanAction *pAction;
   IInternalAI *pIAI = (IInternalAI *)pAI;

   AutoAppIPtr_(NetManager, pNetMan);
   sAINetMsg_JointScan_NG *netmsg = (sAINetMsg_JointScan_NG *)pMsg;

   if (netmsg->justSwitchDirections)
   {
      ConfigSpew("net_scan_spew", ("NET AI: Received scan synch for %d\n", netmsg->aihdr.aiObj));
      pAction = (cAIJointScanAction *)pIAI->GetTheActionOfType(kAIAT_JointScan);
      if (!pAction)
         Warning(("Received join scan update for unknown action\n"));
      else
      {
         // Get the old_state out of the justSwitchDirections field (see BroadcastSwitch...)
         int old_state = netmsg->justSwitchDirections - TRUE;
         pAction->SwitchProxyDirection(old_state);
      }
   }
   else
   {
      ConfigSpew("net_ai_spew", ("NET AI: Received joint scan for %d\n", netmsg->aihdr.aiObj));

      pAction = new cAIJointScanAction(NULL, 0);

      // Since it has no pOwner ability, the cAIAction constructor didn't init m_pAI.
      pAction->m_pAI = pIAI;
      pAction->m_pAIState = pIAI->GetState();

      // Verify that we are dealing with a network proxy, then start the action.
      Assert_(pIAI->IsNetworkProxy());

      pAction->Set(netmsg->jointID, netmsg->targetAng, netmsg->targetAng2,
                   netmsg->ScanSpeed, netmsg->ScanSpeed2);

      // Start the action.
      pIAI->StartProxyAction(pAction, netmsg->deltaTime);
   }
}

///////////////////////////////////////

void cAIJointScanAction::BroadcastSwitchDirections()
{
   ConfigSpew("net_scan_spew", ("NET AI: Sending joint scan synch for %d\n", m_pAIState->GetID()));
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(AINetManager, pAINetMan);
   sAINetMsg_Header hdr = {pAINetMan->NetMsgHandlerID(),m_pAIState->GetID(),FALSE,kAIAT_JointScan};
   sAINetMsg_JointScan_NG netmsg;
   netmsg.aihdr = hdr;

   // Combine TRUE with the current state of the joint (m_state is non-negative).
   netmsg.justSwitchDirections = TRUE + m_state;

   // Just send the header plus the first byte (the justSwitchDirections field).
   pNetMan->Broadcast((void *)&netmsg, sizeof(netmsg)+1, FALSE);
}

///////////////////////////////////////

BOOL cAIJointScanAction::NetUpdate()
{
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);
   // Only Switch directions when we get a message to do so.
   if (pNetMan->Networking())
   {
      if (pObjNet->ObjHostedHere(m_pAIState->GetID())
       && m_pAIState->GetMode() >= kAIM_Normal)
      {
         BroadcastSwitchDirections();
      }
      else if (pObjNet->ObjIsProxy(m_pAIState->GetID()))
      {
         // If its a proxy, just stop and wait for a message to switch directions.
         m_bProxyWait = TRUE;
         result = kAIR_NoResultSwitch;
         return TRUE;
      }
   }
   return FALSE;
}

///////////////////////////////////////

BOOL cAIJointScanAction::IsHostedHere()
{
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);
   return (pNetMan->Networking()) ? pObjNet->ObjHostedHere(m_pAIState->GetID()) : TRUE;
}

///////////////////////////////////////

BOOL cAIJointScanAction::NetEnact(ulong deltaTime)
{
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);
   if (pNetMan->Networking())
   {
      if (!Started() && IsHostedHere())
         BroadcastAction(deltaTime);
      else if (m_bProxyWait)  // we are waiting to be told to switch directions.
      {
         result = kAIR_NoResultSwitch;
         return TRUE;
      }
   }
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAILaunchAction
//

struct sAINetMsg_Launch
{
   sAINetMsg_Header aihdr;
   mxs_vector startLoc, dir;
   NetObjID projectileArchID;
};

////////////////////////////////////////

void cAILaunchAction::BroadcastAction(mxs_vector start_loc, mxs_vector dir)
{
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking())
   {
      AutoAppIPtr_(AINetManager, pAINetMan);
      sAINetMsg_Header hdr = {pAINetMan->NetMsgHandlerID(),
                              m_pAIState->GetID(),
                              FALSE,
                              kAIAT_Launch};
      sAINetMsg_Launch netmsg;
      netmsg.aihdr = hdr;
      netmsg.startLoc = start_loc;
      netmsg.dir = dir;
      AutoAppIPtr_(TraitManager, pTraitMan);
      ObjID projectileArch = pTraitMan->GetArchetype(m_projectile);
      ConfigSpew("net_ai_spew",
                 ("NET AI: Launch %d for %d\n",
                  projectileArch,
                  m_pAIState->GetID()));
      AutoAppIPtr_(ObjectNetworking, pObjNet);
      AutoAppIPtr_(NetManager, pNetMan);
      // This used to use a GID for the archetype. But that seems entirely
      // broken, since these archetypes are statically built into the level.
      // This sometimes crashed, and was probably always unneeded. (I think.)
      // netmsg.projectileArchGID = pNetMan->ToGlobalObjID(projectileArch);
      netmsg.projectileArchID = projectileArch;
      pNetMan->Broadcast((void *)&netmsg, sizeof(netmsg), TRUE);
   }
}

///////////////////////////////////////
//
// used when receiving a network message to Enact this action type.
//

void cAILaunchAction::EnactFire(IAI *pAI, void *pMsg)
{
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);
   sAINetMsg_Launch *netmsg = (sAINetMsg_Launch *)pMsg;

   ConfigSpew("net_ai_spew",
              ("NET AI: Received launch %d for %d\n",
               netmsg->projectileArchID,
               netmsg->aihdr.aiObj));

   ObjID AIObj = ((IInternalAI*)pAI)->GetState()->GetID();

   // Why did this want to use a GID?
   // Convert projectile GID to an ObjID.
   // ObjID projectileArch = pNetMan->FromGlobalObjID(&netmsg->projectileArchGID);
   ObjID projectileArch = netmsg->projectileArchID;

   // If shooting multiples, we need to clone the firing projectile
   AutoAppIPtr_(ObjectSystem, pObjSys);
   ObjID projectile = pObjSys->Create(projectileArch, kObjectConcrete);
   if (projectile != OBJ_NULL) {
      // Note that this is just a copy of the "real" bullet; this will
      // affect damage later:
      gLocalCopyProp->Set(projectile, TRUE);
   }

   cAILaunchAction::Fire(AIObj, netmsg->startLoc, netmsg->dir, projectile);
}

///////////////////////////////////////////////////////////////////////////////
