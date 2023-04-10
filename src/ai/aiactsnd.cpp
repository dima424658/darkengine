/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactsnd.cpp,v 1.5 2000/02/11 18:27:23 bfarquha Exp $
//
//
//

// #define PROFILE_ON 1

#include <aiactsnd.h>

// Must be last header
#include <dbmem.h>


///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISoundAction
//

///////////////////////////////////////
//
// Update the action
//

STDMETHODIMP_(eAIResult) cAISoundAction::Update()
{
   return (result = kAIR_NoResultSwitch);
}

///////////////////////////////////////
//
//
//

STDMETHODIMP_(eAIResult) cAISoundAction::Enact(ulong deltaTime)
{
   return (result = kAIR_NoResultSwitch);
}

///////////////////////////////////////////////////////////////////////////////
