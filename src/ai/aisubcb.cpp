/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aisubcb.cpp,v 1.2 1998/07/28 21:18:44 TOML Exp $
//
//

#include <lg.h>

#include <aisubcb.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISubCombat
//

STDMETHODIMP cAISubCombat::QueryInterface(REFIID id, void ** ppI)
{
   if (IsEqualIID(id, CLSID_cAISubCombat))
   {
      *ppI = this;
      return S_OK;
   }
   return cAIAbility::QueryInterface(id, ppI);
}

///////////////////////////////////////

void cAISubCombat::InitSubCombat(cAICombat * pCombat)
{
   m_pCombat = pCombat;
}

///////////////////////////////////////////////////////////////////////////////
