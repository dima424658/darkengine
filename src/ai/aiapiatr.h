/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiapiatr.h,v 1.1 1999/03/02 17:42:13 TOML Exp $
//
// AI Actor interface -- specifies objects that can create actions
//

#ifndef __AIAPIATR_H
#define __AIAPIATR_H

#ifndef __cplusplus
#error "Private AI APIs require C++"
#endif

#include <aiapicmp.h>

#pragma once
#pragma pack(4)

F_DECLARE_INTERFACE(IAIActor);

class cAIActions;

///////////////////////////////////////////////////////////////////////////////
//
// INTERFACE: IAIActor
//

DECLARE_INTERFACE_(IAIActor, IAIComponent)
{
   //
   // Current action list save/load (pTagFile cursor should be prepositioned).
   //
   STDMETHOD_(BOOL, SaveActions)(ITagFile * pTagFile, cAIActions * pActions) PURE;
   STDMETHOD_(BOOL, LoadActions)(ITagFile * pTagFile, cAIActions * pActions) PURE;
   
   //
   // Actor method
   //
   STDMETHOD (SuggestActions)(cAIGoal * pGoal, const cAIActions & previous, cAIActions * pNew) PURE;

};

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIAPIATR_H */
