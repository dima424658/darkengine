/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprconv.h,v 1.2 1998/11/03 23:00:05 MROWLEY Exp $
//
// Conversation properties
//

#ifndef __AIPRCONV_H
#define __AIPRCONV_H

#include <aiprops.h>
#include <aiutils.h>

#pragma once
#pragma pack(4)

///////////////////////////////////////////////////////////////////////////////

F_DECLARE_INTERFACE(IAIConversationProperty);

///////////////////////////////////////

class cAIConversationDesc;

///////////////////////////////////////

#define PROP_AI_CONV "AI_Converation"

EXTERN IAIConversationProperty* g_pAIConversationProperty;

#undef  INTERFACE
#define INTERFACE IAIConversationProperty
DECLARE_PROPERTY_INTERFACE(IAIConversationProperty)
{
   DECLARE_UNKNOWN_PURE();
   DECLARE_PROPERTY_PURE();
   DECLARE_PROPERTY_ACCESSORS(cAIConversationDesc*);
};

//////////////////////////////////////////

#define PROP_AI_SAVE_CONV "AI_SaveConverse"

EXTERN IBoolProperty* g_pAISaveConversationProperty;

//////////////////////////////////////////

F_DECLARE_INTERFACE(IRelation);

#define AI_CONV_ACTOR_REL "AIConversationActor"

EXTERN IRelation* g_pAIConvActorRelation;

///////////////////////////////////////////////////////////////////////////////

void AIInitConversationProps(void);
void AITermConversationProps(void);

void AIConvDescPropSetStep(int step);  // set which conversation step we are editing

///////////////////////////////////////////////////////////////////////////////

#pragma pack()

#endif /* !__AIPRCONV_H */



