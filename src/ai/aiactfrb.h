/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactfrb.h,v 1.2 1999/03/02 17:41:59 TOML Exp $
//
//
//

#ifndef __AIACTFRB_H
#define __AIACTFRB_H

#include <aibasact.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAIFrobAction
//

class cAIFrobAction : public cAIAction
{
public:
   cAIFrobAction(IAIActor * pOwner, DWORD data = 0);

};

///////////////////////////////////////

inline cAIFrobAction::cAIFrobAction(IAIActor * pOwner, DWORD data)
 : cAIAction(kAIAT_Frob, pOwner, data)
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTFRB_H */
