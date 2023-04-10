/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpcaicha.h>

// Must be last header
#include <dbmem.h>

///////////////////////////////////////
//
// Update the action
//

STDMETHODIMP_(eAIResult) cAIChargeAction::Update()
{
   result = cAILocoAction::Update();
   // switch if timer expired
   if (m_rePathTimer.Expired())
   {
      result = kAIR_Success;
   }
   return result;
}

///////////////////////////////////////
//
// Enact the action.
//

STDMETHODIMP_(eAIResult) cAIChargeAction::Enact(ulong deltaTime)
{
   if (m_rePathTimer.Expired())
   {
      if (m_toObj)
      {
         ObjID objID = GetObj();
         Clear();
         cAILocoAction::Set(objID, kAIS_Fast, 0);
      }
      else
      {
         Clear();
         cAILocoAction::Set(GetDest(), kAIS_Fast, 0);
      }
      m_rePathTimer.Reset();
   }
   return cAILocoAction::Enact(deltaTime);
}

///////////////////////////////////////////////////////////////////////////////

