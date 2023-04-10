/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/shock/shkaimc.cpp,v 1.1 1999/05/23 15:31:07 JON Exp $
//
//
//

#include <shkaimc.h>

#include <aiprops.h>

#include <shkaipr.h>

#include <propbase.h>
#include <propface.h>

#include <dbmem.h>
 
///////////////////////////////////////

cAIShockMeleeMultiCombat::cAIShockMeleeMultiCombat()
{
   m_pActiveSubcombat = new cAIHtoHSubcombat;
   m_pActiveSubcombat->AddRef();
   m_pActiveSubcombat->InitSubability(this);
}

///////////////////////////////////////

cAIShockMeleeMultiCombat::~cAIShockMeleeMultiCombat()
{
   SafeRelease(m_pActiveSubcombat);
}

///////////////////////////////////////

STDMETHODIMP_(void) cAIShockMeleeMultiCombat::GetSubComponents(cAIComponentPtrs * pResult)
{
   m_pActiveSubcombat->AddRef();
   pResult->Append(m_pActiveSubcombat);
}

///////////////////////////////////////

cAISubcombat * cAIShockMeleeMultiCombat::SelectSubcombat(cAIGoal * pGoal, const cAIActions & previous)
{
   return m_pActiveSubcombat;
}

///////////////////////////////////////////////////////////////////////////////

BOOL cAIShockMeleeMultiCombat::IsValidTarget(ObjID object) 
{
   if (AIGetNotMeleeTarget(object))
      return FALSE;
   else
      return cAICombat::IsValidTarget(object);
}
