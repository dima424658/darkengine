/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisnsdev.cpp,v 1.1 1998/08/05 16:15:35 JON Exp $
//
// Device senses
//

#include <aisnsdev.h>
#include <aiprdev.h>

#include <objpos.h>
#include <rendprop.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

floatang cAIDeviceSenses::GetVisionFacing() const
{
   float *jointPos = ObjJointPos(m_pAIState->GetID());
   sAIDeviceParams *pParams;

   if (!g_pAIDeviceProperty->Get(m_pAIState->GetID(), &pParams) || 
      (pParams->m_jointRotate<0) || (pParams->m_jointRotate>MAX_REND_JOINTS))
      return m_pAIState->GetFacingAng();
   else
      return m_pAIState->GetFacingAng()+DEGREES(jointPos[pParams->m_jointRotate]);
}

