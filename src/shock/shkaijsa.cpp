/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaijsa.cpp,v 1.1 1998/07/14 11:18:13 JON Exp $
//
// AI Joint Slide Action 
//

#include <shkaijsa.h>

#include <aibascmp.h>
#include <rendprop.h>
#include <objpos.h>

#include <cfgdbg.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointSlideAction
//

void cAIJointSlideAction::Set(int jointID, float target, float speed)
{
   m_jointID = jointID;
   m_target = target;
   m_speed = speed;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointSlideAction::Update()
{
   if (ObjJointPos(m_pAIState->GetID())[m_jointID]==m_target)
   {
      result = kAIR_Success;
      return result;
   }
   result = kAIR_NoResultSwitch;
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointSlideAction::Enact(ulong deltaTime)
{
   float *pJointPos;
   float delta;

   pJointPos = ObjJointPos(m_pAIState->GetID());
   if (pJointPos[m_jointID]<m_target)
   {
      delta = m_target-pJointPos[m_jointID];
      if (delta<m_speed)
         pJointPos[m_jointID] = m_target;
      else
         pJointPos[m_jointID] += m_speed;
   }
   else
   {
      delta = pJointPos[m_jointID]-m_target;
      if (delta<m_speed)
         pJointPos[m_jointID] = m_target;
      else
         pJointPos[m_jointID] -= m_speed;
   }
   ObjSetJointPos(m_pAIState->GetID(), pJointPos);
   result = kAIR_NoResultSwitch;
   return result;
}










