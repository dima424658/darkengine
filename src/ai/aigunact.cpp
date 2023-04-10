/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// Shock AI Action - gun attack
//

// #define PROFILE_ON 1

#include <aigunact.h>

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

//these are for the calcarc code that I copied from launch.
#include <phprop.h> 
#include <phoprop.h>

//hope we don't actually need this
//well, we don't seem to, but if it causes
//problems, blame me. AMSD
//#include <shkai.h>
#include <gunflash.h>
#include <gunprop.h>
#include <gunapi.h>
#include <gunproj.h>
#include <gunvhot.h>

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


   cPhysAttrProp *pAttrProp;
   mxs_vector *pInitVel;

   // Network the individual shoot events, rather than the whole action.
   // @NOTE (Justin 4/19/99): Networking at this level is now disabled,
   // since we're already networking down in GunLaunchProjectile. Once
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

   // Do actual arc calculation.
   // First, get the info we need on the projectile.
   bool oktocalc = TRUE;
   if (!g_pPhysAttrProp->Get(projID, &pAttrProp))
   {
      Warning(("cAIGunAction::Shoot(): no physics attributes on projectile %d, for AI %d\n",
                projID, m_pAIState->GetID()));
      oktocalc = FALSE;
      //but we don't really want to return here, we just want to not do arc calculation.
   }
   //now check to make sure we have a velocity.
   if (!g_pPhysInitVelProp->Get(projID, &pInitVel))
   {
      Warning(("cAIGunAction::Shoot(): no initial velocity on projectile %d, for AI %d\n",
                projID, m_pAIState->GetID()));
      oktocalc = FALSE;
   }
   if (oktocalc)
   {
      //ok, so now we have the new "target" position at which we have to fire.
      //now we do the pitch, so the pitch should look ok.
      //Note that we have the problem that the launch is occurring not from offset but from center
      //It's possible that the AI will miss since we haven't yet calculated the offset and matched
      //that.  I'm going to do what pitch calc does for the moment and work with that.
      //This shouldn't break any existing gun stuff, because nullgrav projectiles (Shock, and
      //DeepC too, presumably, will ignore the CalcArc, so we're ok).  Also the above things about
      //attributes are only warnings, since we didn't do those checks before and nothing broke, I
      //don't want to suddenly add them.
      //Verbose
      //Maximum Verbosity
      //AMSD
      mxs_vector targetDeltaVec;
      //get our real delta vec
      mx_sub_vec(&targetDeltaVec,&targetModelVec,&launchParams.loc); 
      //figure out our new delta vec (z change)
      CalcArc(&targetDeltaVec, pInitVel->x, pAttrProp->gravity);
      //Fix our targetmodelvec.  targetmodelvec.z used to be targetdeltavec.z + launchparams.loc.z
      //from the mx_sub_vec.  Now we reset it back since deltavec has changed.
      targetModelVec.z = targetDeltaVec.z + launchParams.loc.z;
   }

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
      GunLaunchProjectile(m_pAIState->GetID(), projID, &launchParams);
      CreateGunFlashes(pTraitManager->GetArchetype(m_pAIState->GetID()), m_pAIState->GetID(), jointHeading);
   }
   else
   {
      mx_addeq_vec(&launchParams.loc, &m_pAIGunDesc->m_fireOffset);
      GunLaunchProjectile(m_pAIState->GetID(), projID, &launchParams);
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

DECLARE_TIMER(cAIGunAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIGunAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIGunAction_Enact);

   Assert_((m_pAIGunDesc != NULL) && (m_pBaseGunDesc != NULL));

   // Note that it is not necessary to broadcast this action within network games,
   // since the lower level: GunLaunchProjectile() will be broadcast.

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

