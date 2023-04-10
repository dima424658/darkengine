/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaitsn.cpp,v 1.2 1998/07/26 11:17:23 JON Exp $
//
// Turret senses
//

#include <shkaitsn.h>
#include <shkaitpr.h>

#include <objpos.h>
#include <rendprop.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////

floatang cAITurretSenses::GetVisionFacing() const
{
   float *jointPos = ObjJointPos(m_pAIState->GetID());
   sAITurretParams *pParams;

   g_pAITurretProperty->Get(m_pAIState->GetID(), &pParams);
   return m_pAIState->GetFacingAng()+DEGREES(jointPos[pParams->m_jointRotate]);
}

