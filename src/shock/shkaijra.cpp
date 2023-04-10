/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaijra.cpp,v 1.2 1998/07/14 11:12:27 JON Exp $
//
// AI Joint Rotate Action 
//

#include <shkaijra.h>

#include <aibascmp.h>
#include <rendprop.h>
#include <objpos.h>

#include <cfgdbg.h>

// Must be last header
#include <dbmem.h>

const float kJointRotateEpsilon = 0.1;

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIJointRotateAction
//

void cAIJointRotateAction::Set(int jointID, floatang targetAng, floatang rotateSpeed)
{
   m_jointID = jointID;
   m_targetAng = targetAng;
   m_rotateSpeed = rotateSpeed;
}

////////////////////////////////////////

void cAIJointRotateAction::Set(int jointID, ObjID targetID, floatang rotateSpeed)
{
   m_jointID = jointID;
   m_targetID = targetID;
   m_rotateSpeed = rotateSpeed;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointRotateAction::Update()
{
   float *jointPos;
   floatang jointAng;

   // update target angle if necessary
   if (m_targetID != OBJ_NULL)
      m_targetAng = m_pAIState->AngleTo(ObjPosGet(m_targetID)->loc.vec);
   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[m_jointID]);
   if (Delta(jointAng, m_targetAng).value<kJointRotateEpsilon)
   {
      result = kAIR_Success;
      return result;
   }
   result = kAIR_NoResultSwitch;
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIJointRotateAction::Enact(ulong deltaTime)
{
   float *jointPos;
   floatang jointAng;
   float deltaAng;

   jointPos = ObjJointPos(m_pAIState->GetID());
   jointAng = DEGREES(jointPos[m_jointID]);

   ConfigSpew("JointRotate", ("target: %g, joint: %g\n", m_targetAng.value, jointAng.value));

   if (jointAng.ccwFrom(m_targetAng))
   {
      deltaAng = (jointAng-m_targetAng).value-TWO_PI;
      ConfigSpew("JointRotate", ("delta %g ", deltaAng));
      deltaAng = max(deltaAng, -m_rotateSpeed.value*deltaTime);
      ConfigSpew("JointRotate", ("bound %g ", deltaAng));
   }
   else
   {
      deltaAng = (m_targetAng-jointAng).value;
      ConfigSpew("JointRotate", ("delta %g ", deltaAng));
      deltaAng = min(deltaAng, m_rotateSpeed.value*deltaTime);
      ConfigSpew("JointRotate", ("bound %g ", deltaAng));
   }
   jointPos[m_jointID] = RADIANS(floatang(jointAng+deltaAng).value);
   ObjSetJointPos(m_pAIState->GetID(), jointPos);

   result = kAIR_NoResultSwitch;
   return result;
}










