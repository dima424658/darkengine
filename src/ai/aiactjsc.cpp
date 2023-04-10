/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactjsc.cpp,v 1.11 2000/02/22 14:39:28 adurant Exp $
//
// AI Joint Scan Action
//

// #define PROFILE_ON 1

#include <aiactjsc.h>

#include <appagg.h>
#include <aiapinet.h>
#include <aibascmp.h>
#include <rendprop.h>
#include <objpos.h>

#include <esnd.h>
#include <schema.h>

#include <cfgdbg.h>

// Must be last header
#include <dbmem.h>

const float kJointScanEpsilon = 0.1;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointScanAction
//

cAIJointScanAction::~cAIJointScanAction()
{
   // @TODO: should be in End if End was actually called
   // Well I call End, so I put it there.  AMSD 
   KillRotateSchema();
   BroadcastHalt();
}

/////////////////////////////////

void SchemaEndCallback(int hSchema, ObjID schemaID, void *pData)
{
   Assert_(((cAIJointScanAction*)pData)->m_schemaHandle == hSchema);
   ((cAIJointScanAction*)pData)->m_schemaHandle = SCH_HANDLE_NULL;
}

/////////////////////////////////

void cAIJointScanAction::KillRotateSchema(void)
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

/////////////////////////////////


void cAIJointScanAction::SwitchProxyDirection(int old_state)
{
   floatang targetAng;
   switch (old_state)
   {
      case kAIJointScanStateOne:
         targetAng = m_targetAng;
         m_state = kAIJointScanStateTwo;
         break;
      case kAIJointScanStateTwo:
         targetAng = m_targetAng2;
         m_state = kAIJointScanStateOne;
         break;
      default:
         CriticalMsg("Bad JointScan state");
   }
   float *jointPos = ObjJointPos(m_pAIState->GetID());
   jointPos[m_jointID] = RADIANS(targetAng.value);
   ObjSetJointPos(m_pAIState->GetID(), jointPos);
   m_bProxyWait = FALSE;
}

void cAIJointScanAction::Set(int jointID, floatang targetAng, floatang targetAng2, floatang ScanSpeed, floatang ScanSpeed2)
{
   m_state = kAIJointScanStateOne;
   m_bProxyWait = FALSE;
   m_jointID = jointID;
   m_targetAng = targetAng;
   m_ScanSpeed = ScanSpeed;
   m_targetAng2 = targetAng2;
   m_ScanSpeed2 = ScanSpeed2;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointScanAction::Update()
{
   float *jointPos;
   floatang jointAng;
   floatang targetAng;

   if (m_state == kAIJointScanStateTwo)
      targetAng = m_targetAng2;
   else
      targetAng = m_targetAng;
   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[m_jointID]);
   if (Delta(jointAng, targetAng).value<kJointScanEpsilon)
   {
      if (NetUpdate())
         return result;

      switch (m_state)
      {
         case kAIJointScanStateOne:
         {
            m_state = kAIJointScanStateTwo;
            result = kAIR_NoResultSwitch;
         }
            break;
         case kAIJointScanStateTwo:
         {
            m_state = kAIJointScanStateOne;
            result = kAIR_NoResultSwitch;
         }
            break;
      }
   }
   else
      result = kAIR_NoResult;
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointScanAction::End()
{
   KillRotateSchema();
   BroadcastHalt();
   return cAIAction::End();
}

////////////////////////////////////////

DECLARE_TIMER(cAIJointScanAction_Enact, Average);

STDMETHODIMP_(eAIResult) cAIJointScanAction::Enact(ulong deltaTime)
{
   AUTO_TIMER(cAIJointScanAction_Enact);

   float *jointPos;
   floatang jointAng;
   float deltaAng;
   floatang targetAng;
   floatang ScanSpeed;

   if (NetEnact(deltaTime))
      return result;

   if (IsHostedHere() && (m_schemaHandle == SCH_HANDLE_NULL))
   {
      // play rotate sound
      cTagSet tagSet("Event Rotate");
      tagSet.Add(cTag("LoopState", "loop"));
      sSchemaCallParams callParams = g_sDefaultSchemaCallParams;
      callParams.flags |= SCH_SET_CALLBACK;
      callParams.callback = &SchemaEndCallback;
      callParams.pData = this;
      m_schemaHandle = ESndPlayObj(&tagSet, m_pAIState->GetID(), OBJ_NULL, &callParams);
   }

   SetStarted(TRUE);
   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[m_jointID]);
   if (m_state == kAIJointScanStateTwo)
   {
      targetAng = m_targetAng2;
      ScanSpeed = m_ScanSpeed2;
   }
   else
   {
      targetAng = m_targetAng;
      ScanSpeed = m_ScanSpeed;
   }
   deltaAng = (jointAng-targetAng).value-TWO_PI;
   ConfigSpew("JointScan", ("delta %g ", deltaAng));
   deltaAng = max(deltaAng, -ScanSpeed.value*deltaTime);
   ConfigSpew("JointScan", ("bound %g ", deltaAng));

   jointPos[m_jointID] = RADIANS(floatang(jointAng+deltaAng).value);
   ObjSetJointPos(m_pAIState->GetID(), jointPos);

   result = kAIR_NoResultSwitch;
   return result;
}










