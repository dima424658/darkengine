/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiactcst.h,v 1.2 1999/03/02 17:41:58 TOML Exp $
//
//
//

#ifndef __AIACTCST_H
#define __AIACTCST_H

#include <aibasact.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////
//
// CLASS: cAICustomAction
//

class cAICustomAction : public cAIAction
{
public:
   cAICustomAction(IAIActor * pOwner);

   // Game-defined custom action type
   int customType;
};

///////////////////////////////////////

inline cAICustomAction::cAICustomAction(IAIActor * pOwner)
 : cAIAction(kAIAT_Custom, pOwner)
{
}

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIACTCST_H */
