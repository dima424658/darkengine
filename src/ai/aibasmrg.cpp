/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aibasmrg.cpp,v 1.5 1998/10/30 12:27:18 TOML Exp $
//
//
//

#include <lg.h>
#include <aibasmrg.h>
#include <aibasctm.h>
#include <aimovsug.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIMoveRegulator
//

cAIMoveRegulator::~cAIMoveRegulator()
{
   m_Suggestions.DestroyAll();
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIMoveRegulator::NewRegulations()
{
   return FALSE;
}
   
///////////////////////////////////////

STDMETHODIMP cAIMoveRegulator::SuggestRegulations(cAIMoveSuggestions &)
{
   return S_FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIMoveRegulator::AssistGoal(const sAIMoveGoal & goal, sAIMoveGoal * pResultGoal)
{
   return FALSE;
}

///////////////////////////////////////

STDMETHODIMP_(BOOL) cAIMoveRegulator::WantsInterrupt()
{
   return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
