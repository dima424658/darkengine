/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactsnd.h,v 1.6 1999/03/02 17:42:11 TOML Exp $
//
//
//

#ifndef __AIACTSND_H
#define __AIACTSND_H

#include <ctagset.h>
#include <aibasact.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAISoundAction
//

class cAISoundAction : public cAIAction
{
public:
   cAISoundAction(IAIActor * pOwner, DWORD data = 0);

   // Update the action.
   STDMETHOD_(eAIResult, Update)();

   // Start the action. Instantaneous actions will return result
   STDMETHOD_(eAIResult, Enact)(ulong deltaTime);
   
protected:
   // Cached pointer to soundenactor
   IAISoundEnactor * m_pEnactor;
};

///////////////////////////////////////

inline cAISoundAction::cAISoundAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Sound, pOwner, data)
{
   m_pEnactor = m_pAI->AccessSoundEnactor();
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTSND_H */

