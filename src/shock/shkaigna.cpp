/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaigna.cpp,v 1.27 1999/11/19 14:54:31 adurant Exp $
//
// Shock AI Action - gun attack
//

This file has been moved to aigunact.cpp AMSD


#include <shkaigna.h>

#include <fix.h>
#include <cfgdbg.h>

#include <appagg.h>
#include <esnd.h>
#include <relation.h>
#include <linkbase.h>
#include <lnkquery.h>
#include <traitman.h>
#include <matrixc.h>
#include <objpos.h>
#include <objsys.h>
#include <objdef.h>
#include <netman.h>
#include <iobjnet.h>
#include <aiprdev.h>

#include <aiapinet.h>
#include <aiapinet.h>
#include <airngwpn.h>
#include <rendprop.h>

#include <physapi.h>

#include <shkai.h>
#include <shkflash.h>
#include <shkgunpr.h>
#include <shkgnapi.h>
#include <shkproj.h>
#include <shkvhot.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIGunAction
//
  
cAIGunAction::cAIGunAction(IAIAbility * pOwner, DWORD data)
 : cAIAction(kAIAT_FireGun, pOwner, data),
   m_targetObj(OBJ_NULL),
   m_shotsTaken(0),
   m_gunJointID(-1),
   m_gunID(OBJ_NULL),
   m_pAIGunDesc(NULL),
   m_pBaseGunDesc(NULL)
{
   Initialize();
}

/////////////////////////////////////////

cAIGunAction::cAIGunAction(IAI * pAI)
 : cAIAction(kAIAT_FireGun, NULL, 0),
   m_targetObj(OBJ_NULL),
   m_gunJointID(-1),
   m_shotsTaken(0),
   m_gunID(OBJ_NULL),
   m_pAIGunDesc(NULL),
   m_pBaseGunDesc(NULL)
{
   // Since we have no pOwner, we have to fill in m_pAI and m_pAIState ourselves.
   m_pAI = (IInternalAI *)pAI;
   m_pAIState = m_pAI->GetState();
   Initialize();
   m_gunJointID = AIGetDeviceParams(m_pAIState->GetID())->m_jointRotate;
}

/////////////////////////////////////////

// Initialization used by both constructors
void cAIGunAction::Initialize()
{
   // Get gun from link from our archetype
   AutoAppIPtr_(TraitManager, pTraitMan);
   ILinkQuery *query = g_pAIRangedWeaponLinks->Query(pTraitMan->GetArchetype(m_pAIState->GetID()), LINKOBJ_WILDCARD);
   if (!query->Done())
   {
      sLink link;
      query->Link(&link);
      m_gunID = link.dest;
   }
   else
      Warning(("cAIGunAction: no Ranged Weapon link for AI %d\n", m_pAIState->GetID()));
   SafeRelease(query);

   if (!AIGunDescGet(m_gunID, &m_pAIGunDesc))
      Warning(("cAIGunAction: gun %d has no AI gun description\n", m_gunID));
   if (!BaseGunDescGet(m_gunID, &m_pBaseGunDesc))
      Warning(("cAIGunAction: gun %d has no base gun description\n", m_gunID));
   SetState(kAIGunActionStart);
}

////////////////////////////////////////

void cAIGunAction::Set(ObjID target, int subModel, int gunJointID)
{
   m_targetObj = target;
   m_subModel = subModel;
   m_gunJointID = gunJointID;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIGunAction::Update()
{
   // check target
   if (!CheckTarget())
   {
      result = kAIR_Fail;
      return result;
   }
   if ((m_pAIGunDesc == NULL) || (m_pBaseGunDesc == NULL))
   {
      result = kAIR_Success;
      return result;
   }
   // check for finished
   if ((m_state == kAIGunActionEnd) && (m_startTime+m_pAIGunDesc->m_endTime<=GetSimTime()))
   {
      result = kAIR_Success;
      ConfigSpew("AIGunSpew", ("%d: Returning success\n", m_pAIState->GetID()));
      return result;
   }
   result = kAIR_NoResult;
   return result;
}

////////////////////////////////////////

struct sAINetMsg_ShockShoot {
   sAINetMsg_Header aihdr;
   sGlobalObjID target;
   int gunJointID;
   int subModel;
};

void cAIGunAction::BroadcastShoot()
{
   // Broadcast the action.
   ConfigSpew("net_ai_spew", ("NET AI: Send turret shoot for %d\n", m_pAIState->GetID()));
   AutoAppIPtr_(AINetManager, pAINetMan);
   AutoAppIPtr_(NetManager, pNetMan);
   sAINetMsg_Header hdr = {pAINetMan->NetMsgHandlerID(),
                           m_pAIState->GetID(),
                           FALSE,
                           kAIAT_FireGun};
   sAINetMsg_ShockShoot netmsg;
   netmsg.aihdr = hdr;
   netmsg.target = pNetMan->ToGlobalObjID(m_targetObj);
   netmsg.gunJointID = m_gunJointID;
   netmsg.subModel = m_subModel;
   pNetMan->Broadcast((void *)&netmsg, sizeof(netmsg), FALSE);
}

//////////////////

void cAIGunAction::EnactProxyShootGun(IAI *pAI, void *pMsg)
{
   AutoAppIPtr_(NetManager, pNetMan);
   sAINetMsg_ShockShoot *netmsg = (sAINetMsg_ShockShoot *)pMsg;

   ConfigSpew("net_ai_spew", 
              ("NET AI: Received turret shoot for %d\n", 
               netmsg->aihdr.aiObj));

   cAIGunAction tmpgun(pAI);
   tmpgun.Set(pNetMan->FromGlobalObjID(&(netmsg->target)),
               netmsg->subModel,
               netmsg->gunJointID);
   tmpgun.Shoot();
}   

//////////////////

void cAIGunAction::Shoot(void)
{
   ObjID projID;
   BOOL bNetSuspended = FALSE;
   AutoAppIPtr(TraitManager);
   mxs_vector targetModelVec;   

   ConfigSpew("AIGunSpew", ("%d: Taking shot\n", m_pAIState->GetID()));
   if ((projID = GetProjectile(m_gunID)) == OBJ_NULL)
      return;

   // Network the individual shoot events, rather than the whole action.
   // @NOTE (Justin 4/19/99): Networking at this level is now disabled,
   // since we're already networking down in ShockLaunchProjectile. Once
   // we're confident that this doesn't break anything, we should remove
   // all of the networking code from here, as well as the call into it
   // in the custom proxy behaviour set.
#if 0
   AutoAppIPtr_(NetManager, pNetMan);
   AutoAppIPtr_(ObjectNetworking, pObjNet);
   if (!Started() && pNetMan->Networking() && pObjNet->ObjHostedHere(m_pAIState->GetID()))
   {
      BroadcastShoot();
      // Suspend messaging so that the lower level events are not also broadcasted.
      bNetSuspended = TRUE;
      pNetMan->SuspendMessaging();
   }
#endif

   // Launch the projectile
   sLaunchParams launchParams = g_defaultLaunchParams;
   VHotGetLoc(&(launchParams.loc), m_pAIState->GetID(), 0);
   launchParams.error = m_pAIGunDesc->m_aimError;
 
   // Cheat our pitch & heading
   launchParams.flags = kLaunchPitchOverride|kLaunchHeadingOverride|kLaunchLocOverride;
   // Get position of the submodel we're shooting at
   PhysGetSubModLocation(m_targetObj, m_subModel, &targetModelVec);
   // We really should check pitch from the launch offset, but I'll try this and see if it is adequate
   launchParams.pitch = degrees_to_fixang(RADIANS(m_pAIState->PitchTo(targetModelVec)));
   sAITurretParams *pTurretParams = AIGetTurretParams(m_pAIState->GetID());
   fixang pitchEpsilon = degrees_to_fixang(int(pTurretParams->m_pitchEpsilon));
   if ((launchParams.pitch>pitchEpsilon) && (fixang(-launchParams.pitch)>pitchEpsilon))
      if (launchParams.pitch>FIXANG_PI)
         launchParams.pitch = -pitchEpsilon;
      else
         launchParams.pitch = pitchEpsilon;
   // We cheat our heading too, so that we are always shooting at the selected submodel even though
   // the turret is actually aiming at the object center
   launchParams.heading = degrees_to_fixang(RADIANS(m_pAIState->AngleTo(targetModelVec).value));

   if (m_gunJointID != -1)
   {
      float *pJointPos = ObjJointPos(m_pAIState->GetID());
      mxs_ang jointHeading = degrees_to_fixang(pJointPos[m_gunJointID]);
      mxs_vector fireOffset;

      mx_rot_z_vec(&fireOffset, &m_pAIGunDesc->m_fireOffset, launchParams.heading);
      mx_addeq_vec(&launchParams.loc, &fireOffset);
      ShockLaunchProjectile(m_pAIState->GetID(), projID, &launchParams);
      CreateGunFlashes(pTraitManager->GetArchetype(m_pAIState->GetID()), m_pAIState->GetID(), jointHeading);
   } 
   else
   {
      mx_addeq_vec(&launchParams.loc, &m_pAIGunDesc->m_fireOffset);
      ShockLaunchProjectile(m_pAIState->GetID(), projID, &launchParams);
      CreateGunFlashes(pTraitManager->GetArchetype(m_pAIState->GetID()), m_pAIState->GetID());
   }
   // play firing sound
   cTagSet tagSet("Event Shoot");
   ESndPlayObj(&tagSet, m_pAIState->GetID(), m_gunID); 
#if 0
   if (bNetSuspended)
      pNetMan->ResumeMessaging();
#endif      
}

////////////////////////////////////////

void cAIGunAction::SetState(eAIGunActionState state)
{
   ConfigSpew("AIGunSpew", ("%d: State from %d to %d\n", m_pAIState->GetID(), m_state, state));
   m_state = state;
   switch (m_state)
   {
   case kAIGunActionStart:
      m_startTime = GetSimTime();
      break;
   case kAIGunActionBurst:
      // shoot here
      Shoot();
      ++m_shotsTaken;
      if (m_shotsTaken>=m_pBaseGunDesc->m_burst)
         SetState(kAIGunActionEnd);
      else
         m_startTime = GetSimTime();
      break;
   case kAIGunActionEnd:
      m_startTime = GetSimTime();
      break;
   }
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIGunAction::Enact(ulong deltaTime)
{
   Assert_((m_pAIGunDesc != NULL) && (m_pBaseGunDesc != NULL));

   // Note that it is not necessary to broadcast this action within network games,
   // since the lower level: ShockLaunchProjectile() will be broadcast.

   // Check for state changes
   switch (m_state)
   {
   case kAIGunActionStart:
      if (m_startTime+m_pAIGunDesc->m_startTime<=GetSimTime())
         SetState(kAIGunActionBurst);
      break;
   case kAIGunActionBurst:
      if (m_startTime+m_pAIGunDesc->m_burstTime<=GetSimTime())
         SetState(kAIGunActionBurst);
      break;
   }

   return kAIR_NoResult;
}

