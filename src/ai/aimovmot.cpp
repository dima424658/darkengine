/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aimovmot.cpp,v 1.60 2000/02/11 18:27:52 bfarquha Exp $
//
// Motion components of cAIMoveEnactor
//

// #define PROFILE_ON 1

#include <lg.h>
#include <appagg.h>

#include <cfgdbg.h>
#include <creatapi.h>
#include <creatext.h>
#include <mclntapi.h>
#include <motdesc.h>
#include <motorapi.h>
#include <motprop.h>
#include <mtagvals.h>
#include <phoprop.h>
#include <physapi.h>
#ifdef INCORRECT_AI_NETWORKING
//include <netman.h>
//include <iobjnet.h>
#endif
#include <rendobj.h>
#include <ctagnet.h>

#include <aiapinet.h>
#include <aiactmot.h>
#include <aiactmov.h>
#include <aimove.h>
#include <aimovsug.h>
#include <aiprcore.h>
#include <aitagtyp.h>

#include <creatext.h>
#include <creature.h>
#include <creatur_.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveEnactor
//

void cAIMoveEnactor::MotCleanup()
{
   if (m_pMotionCoord)
   {
      delete m_pMotionCoord;
      m_pMotionCoord = NULL;
   }
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIMoveEnactor::OnSimStart()
{
#ifdef INCORRECT_AI_NETWORKING
   if (!AccessOuterAI()->IsNetworkProxy())
      g_pPhysAICollideProp->Set(GetID(), FALSE);
#endif
}

///////////////////////////////////////

void cAIMoveEnactor::MotOnModeChange(eAIMode previous, eAIMode mode)
{
   IMotor * pMotor = CreatureGetMotorInterface(GetID());

   // If we're entering combat, turn on collidability on,
   // otherwise if it was on, turn it off
   // (For the sake of network message bandwidth, we don't always turn it off).
   if (mode == kAIM_Combat)
      g_pPhysAICollideProp->Set(GetID(), TRUE);
   else if (previous == kAIM_Combat)
      g_pPhysAICollideProp->Set(GetID(), FALSE);

   if (previous == kAIM_Efficient)
   {
      cCreature* pCreature = CreatureFromObj(GetID());
      Assert_(pCreature!=NULL);
      CreatureMakeNonBallistic(GetID());
      // @HACK: yep, this is a total hack. We don't want our ballistic creatures falling
      // to the floor when we go out of efficiency mode
      if ((!pCreature->GetCreatureDesc()->alwaysBallistic) && pCreature->IsPhysical())
      {
         // @TBD (toml 08-27-98): get gravity from room
         PhysSetGravity(GetID(), 1.0);
         PhysSetBaseFriction(GetID(), 0.0);
      }
      //mprint("Non ballistic!\n");
   }

   if (pMotor)
   {
#ifndef NO_NON_PHYSICAL_SLEEP
      if (mode <= kAIM_SuperEfficient)
      {
         pMotor->MakeNonPhysical();
         //mprint("Non phys!\n");
      }
      else if (mode != kAIM_Dead)
      {
         pMotor->MakePhysical();
         //mprint("Phys!\n");
      }
#else
      //      if (mode == kAIM_Dead)
      //         pMotor->MakeNonPhysical();
#endif
   }

   if (mode == kAIM_Efficient)
   {
      CreatureMakeBallistic(GetID(), kCMB_Efficient);
      cCreature* pCreature = CreatureFromObj(GetID());
      if (pCreature->IsPhysical())
      {
         SetObjImpulse(GetID(), 0, 0, 0, 0);
         if (!pCreature->GetCreatureDesc()->alwaysBallistic)
         {
            PhysSetGravity(GetID(), 0);
            PhysSetBaseFriction(GetID(), 1.0);
         }
      }
      //mprint("Ballistic!\n");
   }
}

///////////////////////////////////////

void cAIMoveEnactor::MotorStateChangeCallback(void * p)
{
}

///////////////////////////////////////

BOOL cAIMoveEnactor::MotSave(ITagFile *pTagFile)
{
   if (m_pMotionCoord)
      m_pMotionCoord->Save(pTagFile);
   return TRUE;
}

///////////////////////////////////////

BOOL cAIMoveEnactor::MotLoad(ITagFile *pTagFile)
{
   if (m_pMotionCoord)
      m_pMotionCoord->Load(pTagFile);
   return TRUE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIMoveEnactor::SupportsMotionActions()
{
   return !!m_pMotionCoord;
}

///////////////////////////////////////

HRESULT cAIMoveEnactor::SetMotor(IMotor * pMotor)
{
   // this assertion is here just to sanity-check that
   // motion system enum and ai enums are in synch, since
   // one gets cast to the other.  Would have this in
   // constructor, but constructor is inline, and this
   // function doesn't get called very often.  KJ 5/20/98
   AssertMsg(kAIME_Error==kMCoord_Error,"mcoord and ai status enums out of synch");

   if (!pMotor)
   {
      if (m_pMotionCoord)
      {
         m_pMotionCoord->SetMotor(NULL);
         delete m_pMotionCoord;
         m_pMotionCoord = NULL;
      }
   }
   else
   {
      if (!m_pMotionCoord)
      {
         m_pMotionCoord = MotSysCreateMotionCoordinator();
         m_pMotionCoord->SetMotorStateChangeCallback(MotorStateChangeCallback,(void *)this);
         ResetMotionTags();
      }
      m_pMotionCoord->SetMotor(pMotor);
   }
   return S_OK;
}

///////////////////////////////////////

HRESULT cAIMoveEnactor::ResetMotionTags()
{
   if (!m_pMotionCoord)
      return S_FALSE;

   sMotActorTagList *actorTagList;
   cTagSet tags;

   if(!ObjGetActorTagList(m_pAIState->GetID(), &actorTagList))
   {
      m_pMotionCoord->SetPersistentTags(NULL);
      return S_OK;
   }

   tags.FromString(actorTagList->m_TagStrings);
   m_pMotionCoord->SetPersistentTags(&tags);

   return S_OK;
}

///////////////////////////////////////
//
//
//

eAIMEStatus cAIMoveEnactor::MotStatus()
{
   eAIMEStatus status = eAIMEStatus(m_pMotionCoord->GetStatus());
   if (!CreatureSelfPropelled(GetID()))
   {
      if (status == kAIME_ActiveBusy && m_ImpulsePeriod.Expired())
         status = kAIME_ActiveInterrupt;
   }
   return status;
}

///////////////////////////////////////

HRESULT cAIMoveEnactor::MotNoAction(ulong deltaTime)
{
   eAIMEStatus status;
   sAIMoveGoal nullGoal;
   BOOL newStandGoal;
   BOOL refreshBallistic = FALSE;

   if (m_pAIState->GetMode() <= kAIM_Efficient)
      return S_OK;

   status = MotStatus();
   // We only play "stand" motions if not efficient, and the coordinator
   // is idle, and we either didn't play one as our last act or the
   // AI is actually being rendered (toml 11-03-98)
   newStandGoal = (status == kMCoord_Idle) && (!(m_flags&kLastWasNoAction) || (m_StandTimer.Expired() && rendobj_object_visible(GetID())));
   // If we're not ready to "re-stand" then check if we need to update our ballistic creature
   if (!newStandGoal)
      refreshBallistic = !CreatureSelfPropelled(GetID()) && (status == kAIME_ActiveInterrupt);
   if (newStandGoal || refreshBallistic)
   {
      nullGoal.dir = m_pAIState->GetFacingAng();
      nullGoal.dest = *m_pAIState->GetLocation();
   }
   if (newStandGoal)
   {
      m_flags |= kLastWasNoAction;
      m_StandTimer.Reset();
      return MotEnactMoveGoal(nullGoal, cTagSet(), deltaTime);
   }
   else if (refreshBallistic)
      return NonMotEnactMoveGoal(nullGoal, deltaTime);
   return S_OK;
}

///////////////////////////////////////
//
// Enact a locomotion
//

DECLARE_TIMER(cAIMoveEnactor_MotEnact, Average);

HRESULT cAIMoveEnactor::MotEnact(cAIMoveAction * pAction, ulong deltaTime)
{
   AUTO_TIMER(cAIMoveEnactor_MotEnact);

   if (m_pAIState->GetMode() <= kAIM_Efficient)
      return S_FALSE;

#if 0
   if (m_pMotionCoord->GetStatus() == kMCoord_ActiveBusy)
      return S_OK;
#endif
   m_flags &= ~kLastWasNoAction;
   m_StandTimer.Force();
   return MotEnactMoveGoal(pAction->GetMoveGoal(), pAction->GetTags(), deltaTime);
}

///////////////////////////////////////
//
// Locomotive goal primitive
//

#define STATIONARY_ERROR         (MX_ANG_PI / 4)
#define STATIONARY_POS_ERROR_SQ  4.0

DECLARE_TIMER(AI_AIME_MEMG, Average);
DECLARE_TIMER(AI_AIME_BP_LOC, Average);

HRESULT cAIMoveEnactor::MotEnactMoveGoal(const sAIMoveGoal & inputGoal, const cTagSet & tags, ulong deltaTime)
{
  eAIMEStatus motionStatus = (eAIMEStatus)m_pMotionCoord->GetStatus();
  sAIMoveGoal goal = inputGoal;

  if (motionStatus == kAIME_Idle || motionStatus == kAIME_ActiveInterrupt)
  {
      AUTO_TIMER(AI_AIME_MEMG);

      for (int i = 0; i < m_MoveRegulators.Size(); i++)
         m_MoveRegulators[i]->AssistGoal(inputGoal, &goal);

#ifdef INCORRECT_AI_NETWORKING
   #ifdef NEW_NETWORK_ENABLED
      AutoAppIPtr(NetManager);
      AutoAppIPtr_(ObjectNetworking, pObjNet);
      if (pNetManager->Networking() && pObjNet->ObjHostedHere(m_pAIState->GetID())) // @TODO && !m_pAIState->NetworkingComplexAction()
         BroadcastMove(goal, tags, deltaTime);
   #endif
#endif

      const cMxsVector &  loc = *m_pAIState->GetLocation();
      sMcMoveParams       params;

      ConfigSpew("AIEnactTrace",("MotEnactMoveGoal"));

      params.mask         = 0;
      params.direction.tz = (mxs_ang)((goal.dir.value/TWO_PI)*0xffff);
      params.direction.tx = params.direction.ty=0;
      params.duration     = deltaTime;

      if (goal.speed > kAIS_Stopped)
      {
         params.tags.Add(cTag("Locomote", kMTV_set));

         if (goal.speed >= kAIS_Fast)
            params.tags.Add(cTag("LocoUrgent", kMTV_set));

         mxs_vector tmp;
         mx_copy_vec(&tmp,&goal.dest);
         ConfigSpew("CerebllmStride",("%d dest: %g, %g\n",m_pAIState->GetID(),tmp.x,tmp.y));
         mx_subeq_vec(&tmp,&loc);
         tmp.z=0;
         params.distance=mx_mag_vec(&tmp);
         params.mask|=kMotParmFlag_Distance;
         ConfigSpew("CerebllmStride",("%d curp: %g, %g\n%d delt: %g, %g\n",\
            m_pAIState->GetID(),loc.x,loc.y,m_pAIState->GetID(),tmp.x,tmp.y));
      }
      else
      {
         cTagSet standTags(AIStandTags(GetID()));
         if (standTags.Size())
            params.tags.Append(standTags);
      }
      params.tags.Append(tags);

      // only set distance if velocity non-zero
      params.mask|=(kMotParmFlag_Direction|kMotParmFlag_Duration);

      if (goal.facing.type != kAIF_Any)
      {
         params.mask|=kMotParmFlag_Facing;
         params.facing.tx=params.facing.ty=0;
         params.facing.tz=(mxs_ang)((ComputeFacing(goal).value/TWO_PI)*0xffff);
      }

      const char * pszTagsProp = AIGetMotionTags(GetID());

      if (pszTagsProp)
         params.tags.Append(cTagSet(pszTagsProp));

      TIMER_Start(AI_AIME_BP_LOC);

      IMotionPlan * pPlan = m_pMotionCoord->BuildPlan(m_pMotionCoord->GetNextEndState(), &params);

      if (pPlan)
      {
         IManeuver *pManeuver = pPlan->PopFirstManeuver();
         m_pMotionCoord->SetCurrentManeuver(pManeuver);
         ConfigSpew("CerebellumTrace",("%d: setting maneuver\n",m_pAIState->GetID()));
         delete pPlan;

         TIMER_MarkStop(AI_AIME_BP_LOC);

      }
      else
      {
         cStr tagString;

   #ifndef SHIP
         if (params.tags.Size())
         {
            params.tags.ToString(&tagString);
            Warning(("Unable to build motion plan for obj %d with tags {%s}\n",GetID(),tagString));
         }
   #endif
         TIMER_MarkStop(AI_AIME_BP_LOC);

         return E_FAIL;
      }
  }

   // If the motion system wont move us, use wedge movement
   if (m_ImpulsePeriod.Expired() && !CreatureSelfPropelled(GetID()))
// @TBD (toml 05-21-99): this appears to cause a double assist goal?
      return NonMotEnactMoveGoal(inputGoal, deltaTime);
   else
   {
      AutoAppIPtr(AINetServices);
      if (pAINetServices->Networking())
      {
         sAIImpulse impulse;
         CalculateImpulse(goal, deltaTime, &impulse);
         CalculateTargetVel(goal, deltaTime, &impulse);
         memcpy(m_pTargetVel, &impulse, sizeof(impulse));
      }
   }

   return S_OK;
}

//////////////////////////////////////////

#ifdef INCORRECT_AI_NETWORKING
// Non-guaranteed (NG) move message structure.  Only used by the following two routines.
struct sAINetMsg_Move_NG {
   sAINetMsg_Header aihdr;
   ulong deltaTime;
   sAIMoveGoal goal;
   char tagsetBlock[1]; // Raw block for tagset
};


// Broadcast a move for the given goal and tags (not based on an action object).
void cAIMoveEnactor::BroadcastMove(const sAIMoveGoal &goal, const cTagSet &tags, ulong deltaTime)
{
   ConfigSpew("net_ai_spew", ("NET AI: Send move for %d\n", m_pAIState->GetID()));

   // Figure out the size of the tagset block, allocate an appropriate
   // sized message, and fill in the tagset block.
   void *pTagBlock = NULL;
   int size = TagSetToNetBlock(&tags, &pTagBlock);
   AssertMsg(pTagBlock, "Tag block is empty!");
   // msgSize is the size of the message. We subtract 1 for the byte that
   // we have to declare for tagsetBlock:
   ulong msgSize = sizeof(sAINetMsg_Move_NG) + size - 1;
   sAINetMsg_Move_NG *netmsg = (sAINetMsg_Move_NG *)malloc(msgSize);
   memcpy(netmsg->tagsetBlock, pTagBlock, size);
   free(pTagBlock);

   AutoAppIPtr(AINetManager);
   sAINetMsg_Header hdr = {pAINetManager->NetMsgHandlerID(),
                           m_pAIState->GetID(),
                           FALSE,
                           kAIAT_Move};
   netmsg->aihdr = hdr;
   netmsg->deltaTime = deltaTime;
   netmsg->goal = goal;

   AutoAppIPtr(NetManager);
   pNetManager->Broadcast(netmsg, msgSize, FALSE);
   free(netmsg);
}

// Enact a move received off the network.  No action is created for it.
STDMETHODIMP_(void) cAIMoveEnactor::HandleMoveMessage(void *pMsg,
                                                      ObjID fromPlayer)
{
   ConfigSpew("net_ai_spew",
              ("NET AI: Receive move for %d\n", m_pAIState->GetID()));

   sAINetMsg_Move_NG *netmsg = (sAINetMsg_Move_NG *)pMsg;
   // Initialize with a string, because the stupid compiler chokes if
   // we use the empty constructor:
   cTagSet tags("");
   NetBlockToTagSet(netmsg->tagsetBlock, fromPlayer, &tags);

   // Get the goal, tags & delta Time out of netmsg.
   MotEnactMoveGoal(netmsg->goal, tags, netmsg->deltaTime);
}
#endif

///////////////////////////////////////
//
// Play a motion
//
// @Note (toml 05-19-98): Right now, all motions interrupt

DECLARE_TIMER(AI_AIME_ME_M, Average);
DECLARE_TIMER(AI_AIME_BP_MOT, Average);

HRESULT cAIMoveEnactor::MotEnact(cAIMotionAction * pAction, ulong deltaTime)
{
   if ((m_pAIState->GetMode() > kAIM_Efficient) && !pAction->Started())
   {
      if (!pAction->GetName() && !pAction->GetTags().Size())
         return MotNoAction(deltaTime);

      AUTO_TIMER(AI_AIME_ME_M);

      m_flags &= ~kLastWasNoAction;
      m_StandTimer.Force();

      sMcMoveParams        params;

      params.tags = pAction->GetTags();
      params.mask = 0;

      if (pAction->IsFacingSet())
      {
         params.facing.tz = (mxs_ang)((pAction->GetFacing().value/TWO_PI)*0xffff);
         params.facing.tx = params.facing.ty = 0;
         params.mask |= kMotParmFlag_Facing;
      }

      if(pAction->GetFocus(&params.focus, &params.turnspeed))
      {
         params.mask |= kMotParmFlag_FocusObj;
      }

      if (pAction->GetName())
      {
         params.motionNum = MotDescNameGetNum((char *)pAction->GetName());
         params.mask |= kMotParmFlag_MotNum;
         if (params.motionNum < 0)
            return E_FAIL;
         params.tags.Append(cTag("PlaySpecMotion",kMTV_true));
      }

      const char * pszTagsProp = AIGetMotionTags(GetID());

      if (pszTagsProp)
         params.tags.Append(cTagSet(pszTagsProp));

#ifdef INCORRECT_AI_NETWORKING
#ifdef NEW_NETWORK_ENABLED
      AutoAppIPtr(NetManager);
      AutoAppIPtr_(ObjectNetworking, pObjNet);
      if (pNetManager->Networking() &&  pObjNet->ObjHostedHere(m_pAIState->GetID())) // @TODO && !m_pAIState->NetworkingComplexAction()
         BroadcastMotion(params);
#endif
#endif

      return MotEnactMotion(params);
   }
   if (m_ImpulsePeriod.Expired() && !CreatureSelfPropelled(GetID()))
   {
      sAIMoveGoal nullGoal;

      if (pAction->IsFacingSet())
         nullGoal.dir = pAction->GetFacing();
      else
         nullGoal.dir = m_pAIState->GetFacingAng();
      nullGoal.dest = *m_pAIState->GetLocation();
      return NonMotEnactMoveGoal(nullGoal, deltaTime);
   }
   return S_OK;
}


HRESULT cAIMoveEnactor::MotEnactMotion(const sMcMoveParams &params)
{
   TIMER_Start(AI_AIME_BP_MOT);

   const sMcMoveState * pState = m_pMotionCoord->GetNextEndState();
   IMotionPlan *pPlan = m_pMotionCoord->BuildPlan(pState, &params);

   if (!pPlan)
   {
      cStr tagString;

      params.tags.ToString(&tagString);
      Warning(("Unable to build motion plan for obj %d with tags {%s}\n",m_pAIState->GetID(),tagString));

      TIMER_MarkStop(AI_AIME_BP_MOT);

      return E_FAIL;
   }

   IManeuver *pManeuver = pPlan->PopFirstManeuver();
   m_pMotionCoord->SetCurrentManeuver(pManeuver);
   delete pPlan;

   TIMER_MarkStop(AI_AIME_BP_MOT);

   return S_OK;
}

///////////////////////////////////////

#ifdef INCORRECT_AI_NETWORKING
// Non-guaranteed (NG) motion message structure.  Only used by the following two routines.
struct sAINetMsg_Motion_NG {
   sAINetMsg_Header aihdr;
   ulong mask;
   mxs_ang facing_tz;
   sGlobalObjID focusGID;
   int motionNum;
   char tagsetBlock[1]; // Raw block for tagset
};

// Broadcast a motion for the given params.
void cAIMoveEnactor::BroadcastMotion(const sMcMoveParams params)
{
   ConfigSpew("net_ai_spew", ("NET AI: Send motion for %d\n", m_pAIState->GetID()));

   // Figure out the size of the tagset block, allocate an appropriate
   // sized message, and fill in the tagset block.
   void *pTagBlock = NULL;
   int size = TagSetToNetBlock(&(params.tags), &pTagBlock);
   AssertMsg(pTagBlock, "Tag block is empty!");
   // msgSize is the size of the message. We subtract 1 for the bytes that
   // we have to declare for tagsetBlock:
   ulong msgSize = sizeof(sAINetMsg_Motion_NG) + size - 1;
   sAINetMsg_Motion_NG *netmsg = (sAINetMsg_Motion_NG *)malloc(msgSize);
   memcpy(netmsg->tagsetBlock, pTagBlock, size);
   free(pTagBlock);

   AutoAppIPtr(AINetManager);
   AutoAppIPtr(NetManager);
   sAINetMsg_Header hdr = {pAINetManager->NetMsgHandlerID(), m_pAIState->GetID(), FALSE, kAIAT_Motion};
   netmsg->aihdr = hdr;
   netmsg->mask = params.mask;
   if (params.mask & kMotParmFlag_Facing)
      netmsg->facing_tz = params.facing.tz;
   if (params.mask & kMotParmFlag_FocusObj)
      netmsg->focusGID = pNetManager->ToGlobalObjID(params.focus);
   if (params.mask & kMotParmFlag_MotNum)
      netmsg->motionNum = params.motionNum;

   pNetManager->Broadcast(netmsg, msgSize, FALSE);
   free(netmsg);
}

///////////////////////////////////////

// Enact a motion based on the net message.  No action is created.
STDMETHODIMP_(void) cAIMoveEnactor::HandleMotionMessage(void *pMsg,
                                                        ObjID fromPlayer)
{
   ConfigSpew("net_ai_spew", ("NET AI: Receive motion for %d\n", m_pAIState->GetID()));

   sAINetMsg_Motion_NG *netmsg = (sAINetMsg_Motion_NG *)pMsg;

   // Get the motion parameters out of netmsg.
   // First the tags.
   sMcMoveParams params;
   cTagSet tags("");
   NetBlockToTagSet(netmsg->tagsetBlock, fromPlayer, &tags);
   params.tags = tags;
   // Copy the mask and the optional fields.
   params.mask = netmsg->mask;
   if (params.mask & kMotParmFlag_Facing)
   {
      params.facing.tz = netmsg->facing_tz;
      params.facing.tx = params.facing.ty = 0;
   }
   if (params.mask & kMotParmFlag_FocusObj)
   {
      AutoAppIPtr(NetManager);
      params.focus = pNetManager->FromGlobalObjID(&netmsg->focusGID);
   }
   if (params.mask & kMotParmFlag_MotNum)
      params.motionNum = netmsg->motionNum;

   // Now do the motion on the proxy ai.
   MotEnactMotion(params);
}
#endif

///////////////////////////////////////
//
//
//

HRESULT cAIMoveEnactor::MotEnact(cAIOrientAction * pAction, ulong deltaTime)
{
   m_flags &= ~kLastWasNoAction;
   m_StandTimer.Force();
// @TBD (toml 04-10-98):
   return 0;
}

