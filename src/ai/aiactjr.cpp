/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactjr.cpp,v 1.20 2000/02/28 11:29:35 toml Exp $
//
// AI Joint Rotate Action
//

// #define PROFILE_ON 1

#include <aiactjr.h>

#include <appagg.h>
#include <aiapinet.h>
#include <aibascmp.h>
#include <aiprcore.h>
#include <rendprop.h>
#include <iobjsys.h>
#include <objdef.h>
#include <objpos.h>

#include <esnd.h>
#include <schema.h>

#include <cfgdbg.h>

//to supposrt Awareness checks on rotate
#include <aiaware.h>

// Must be last header
#include <dbmem.h>

const float kJointRotateEpsilon = 0.1;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointRotateAction
//

cAIJointRotateAction::~cAIJointRotateAction()
{
   // @TODO: this schema kill should be in End()?
   KillRotateSchema();
   // Don't broadcast the stop in network games, let other machines get to the target.
   if (m_listener != NULL)
   {
      AutoAppIPtr(ObjectSystem);
      pObjectSystem->Unlisten(m_listener);
   }
}

void cAIJointRotateAction::Set(int jointID, floatang targetAng, floatang epsilonAng)
{
   m_jointID = jointID;
   m_targetID = OBJ_NULL;
   m_targetAng = targetAng;
   m_epsilonAng = epsilonAng;
}

////////////////////////////////////////

void AIJointRotateObjListener(ObjID objID, eObjNotifyMsg msg, void *data)
{
   cAIJointRotateAction* pAction = (cAIJointRotateAction*)data;

   if ((objID == pAction->GetTarget()) && (msg == kObjNotifyDelete))
      pAction->SetTarget(OBJ_NULL);
}

////////////////////////////////////////

void cAIJointRotateAction::Set(int jointID, ObjID targetID, floatang epsilonAng)
{
   m_jointID = jointID;
   m_targetID = targetID;
   m_targetAng = m_pAIState->AngleTo(ObjPosGet(m_targetID)->loc.vec);
   m_epsilonAng = epsilonAng;
   // Listen for target destruction
   AutoAppIPtr_(ObjectSystem, pObjSys);
   sObjListenerDesc desc = {AIJointRotateObjListener, (void*)this};
   m_listener = pObjSys->Listen(&desc);
}

////////////////////////////////////////

void cAIJointRotateAction::KillRotateSchema(void)
{
   // kill our schema & play stop sound
   if (m_schemaHandle != SCH_HANDLE_NULL)
   {
      // @TODO: we should really have a callback to make sure that the schema didn't die
      // underneath us
      SchemaPlayHalt(m_schemaHandle);
      m_schemaHandle = SCH_HANDLE_NULL;
   }
}

////////////////////////////////////////

void cAIJointRotateAction::StartRotateSchema(void)
{
   Assert_(m_schemaHandle == SCH_HANDLE_NULL);
   cTagSet tagSet("Event Rotate");
   tagSet.Add(cTag("LoopState", "loop"));
   sSchemaCallParams callParams = g_sDefaultSchemaCallParams;
   callParams.flags |= SCH_SET_CALLBACK;
   callParams.callback = &SchemaEndCallback;
   callParams.pData = this;
   m_schemaHandle = ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL, &callParams);
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointRotateAction::Update()
{
   float *jointPos;
   floatang jointAng;

   // update target angle if necessary
   if (m_targetID != OBJ_NULL)
      //if our awareness is too low, don't keep knowing where it is.
      if (m_pAI->GetAwareness(m_targetID)->level <= kAIAL_Lowest)
        m_targetID = OBJ_NULL;
      else
        m_targetAng = m_pAIState->AngleTo(ObjPosGet(m_targetID)->loc.vec);
   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = m_pAIState->GetFacingAng()+DEGREES(jointPos[m_jointID]);
   if (Delta(jointAng, m_targetAng).value<m_epsilonAng)
   {
      // kill our schema & play stop sound
      KillRotateSchema();
      cTagSet tagSet("Event Rotate");
      tagSet.Add(cTag("LoopState", "stop"));
      ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL);
      result = kAIR_Success;
      return result;
   }
   result = kAIR_NoResult;
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointRotateAction::End()
{
   KillRotateSchema();
   return cAIAction::End();
}

////////////////////////////////////////

DECLARE_TIMER(cAIJointRotateAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIJointRotateAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIJointRotateAction_Enact);

   float *jointPos;
   floatang jointAng;
   floatang facingAng;
   float deltaAng;
   float maxTurn;
   BOOL finish;

   if (m_schemaHandle == SCH_HANDLE_NULL)
      StartRotateSchema();

   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[m_jointID]);
   facingAng = m_pAIState->GetFacingAng()+jointAng;

   ConfigSpew("JointRotate", ("target: %g, joint: %g\n", m_targetAng.value, jointAng.value));

   BroadcastAction(deltaTime);

   SetStarted(TRUE);
   if (facingAng.ccwFrom(m_targetAng))
   {
      //deltaAng in this case was grossly miscalculated as near negative TWO_PI
      //Caused massive overshoot in this direction.  Now fixed.  AMSD 2/25/00
      deltaAng = -(facingAng-m_targetAng).value;
      ConfigSpew("JointRotate", ("delta %g ", deltaAng));
      Assert_(deltaAng<=0);
      // @TODO: figure out why AITurnRate is all screwed up in the release build
      // old code here return bad values for some reason
      // old code: maxTurn = -AIGetTurnRate(m_pAIState->GetID())*float(deltaTime);
      g_pAITurnRateProperty->Get(m_pAIState->GetID(), &maxTurn);
      maxTurn = -1*DEGREES(maxTurn/1000.0)*float(deltaTime);
      deltaAng = max(maxTurn, deltaAng);
      finish = (deltaAng>maxTurn);
   }
   else
   {
      deltaAng = (m_targetAng-facingAng).value;
      Assert_(deltaAng>=0);
      ConfigSpew("JointRotate", ("delta %g ", deltaAng));
      g_pAITurnRateProperty->Get(m_pAIState->GetID(), &maxTurn);
      maxTurn = DEGREES(maxTurn/1000.0)*float(deltaTime);
      deltaAng = min(maxTurn, deltaAng);
      finish = (deltaAng<maxTurn);
   }
   if (finish)
      jointPos[m_jointID] = RADIANS((m_targetAng-m_pAIState->GetFacingAng()).value);
   else
      jointPos[m_jointID] = RADIANS(floatang(jointAng+deltaAng).value);
   ObjSetJointPos(m_pAIState->GetID(), jointPos);

   result = kAIR_NoResultSwitch;
   return result;
}

/////////////////////////////////

void SchemaEndCallback(int hSchema, ObjID schemaID, void *pData)
{
   Assert_(((cAIJointRotateAction*)pData)->m_schemaHandle == hSchema);
   ((cAIJointRotateAction*)pData)->m_schemaHandle = SCH_HANDLE_NULL;
}









