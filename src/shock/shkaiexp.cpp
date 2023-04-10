/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaiexp.cpp,v 1.2 1999/11/19 14:56:08 adurant Exp $
//
// AI Action - explode
//

#include <shkaiexp.h>

#include <comtools.h>
#include <appagg.h>

#include <mvrflags.h>

#include <gunproj.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIExplodeAction
//

////////////////////////////////////////

void cAIExplodeAction::Set(ObjID projectileID)
{
   m_projectileID = projectileID;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIExplodeAction::Update()
{
   result = cAIMotionAction::Update();
   return result;
}

////////////////////////////////////////

STDMETHODIMP_(eAIResult) cAIExplodeAction::Enact(ulong deltaTime)
{
   result = cAIMotionAction::Enact(deltaTime);
   // Check if the motion has hit a launch flag
   if (PollMotionFlags(m_pAIState->GetID(), MF_FIRE_RELEASE))
      GunLaunchProjectile(m_pAIState->GetID(), m_projectileID);
   return result;
}










