/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/ai/aiprconv.cpp,v 1.6 1998/11/21 14:19:25 MROWLEY Exp $
//
// Conversation property
//

#include <lg.h>

#include <propbase.h>
#include <property.h>
#include <propert_.h>
#include <prophash.h>
#include <sdesbase.h>
#include <sdesc.h>

#include <linkbase.h>
#include <relation.h>

#include <aiprconv.h>
#include <aiprops.h>
#include <aipsdscr.h>
#include <aicnvrse.h>

#include <dbmem.h>

F_DECLARE_INTERFACE(IAIConversationProperty);

///////////////////////////////////////////////////////////////////////////////
//
// Conversation property
//

///////////////////////////////////////

IAIConversationProperty* g_pAIConversationProperty;

///////////////////

static sPropertyDesc _g_ConversationPropertyDesc = 
{
   PROP_AI_CONV,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CONVERSE_CAT, "Conversation" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////////////////////////

IBoolProperty* g_pAISaveConversationProperty;

//////////////////////////////

static sPropertyDesc _g_SaveConversationPropertyDesc = 
{
   PROP_AI_SAVE_CONV,
   0,
   NULL, 0, 0,  // constraints, version
   { AI_CONVERSE_CAT, "SaveConversation", 
     "Should this conversation be restarted from the beginning after load" },
   kPropertyChangeLocally,  // net_flags
};

///////////////////

extern void CreateAIConversationPropEditor(IAIConversationProperty* prop);

typedef cSpecificProperty<IAIConversationProperty, &IID_IAIConversationProperty, cAIConversationDesc *, cHashPropertyStore< cNoZeroDataOps<cAIConversationDesc> > > cAIConversationDescPropertyBase;

class cAIConversationDescProperty : public cAIConversationDescPropertyBase
{
public:
   cAIConversationDescProperty()
      : cAIConversationDescPropertyBase(&_g_ConversationPropertyDesc)
   {
   };

   STANDARD_DESCRIBE_TYPE(cAIConversationDesc);

protected:
   virtual void CreateEditor()
   {
#ifdef EDITOR
       CreateAIConversationPropEditor(this);
#endif 
   }
};
 
///////////////////
 
cAIConversationDesc::cAIConversationDesc()
{
   abortLevel = kAIAL_Num;
   abortPriority = kAIP_Normal;
   for (int i=0; i<kAIMaxConvSteps; i++)
      for (int j=0; j<kAIMaxConvActions; j++)
      {
         steps[i][j].actor = kAICA_None;
         steps[i][j].flags = 0;
         steps[i][j].act.type = kAIPS_Nothing;
         for (int k=0; k<kAIPSArgs; k++)
            steps[i][j].act.args[k][0] = '\0';
      }
}

///////////////////////////////////////

static sRelationDesc _g_AIConvActorRelationDesc = 
{
   AI_CONV_ACTOR_REL,
   kRelationNetworkLocalOnly,
};

static sRelationDataDesc _g_AIConvActorRelationDataDesc = {"Actor ID", sizeof(int),}; 

static sFieldDesc _g_AIConvActorFieldDesc = 
   { "Actor ID", kFieldTypeInt, sizeof(int), 0, 0 };

static sStructDesc _g_AIConvActorStructDesc = 
   { "Actor ID", sizeof(float), kStructFlagNone, 1, &_g_AIConvActorFieldDesc};

IRelation* g_pAIConvActorRelation = NULL;

///////////////////////////////////////

void AIInitConvActorRelation(void)
{
   Assert_(g_pAIConvActorRelation == NULL);
   StructDescRegister(&_g_AIConvActorStructDesc);
   g_pAIConvActorRelation = CreateStandardRelation(&_g_AIConvActorRelationDesc, &_g_AIConvActorRelationDataDesc, kQCaseSetSourceKnown);
}

///////////////////////////////////////

void AITermConvActorRelation(void)
{
   Assert_(g_pAIConvActorRelation != NULL);
   SafeRelease(g_pAIConvActorRelation);
   g_pAIConvActorRelation = NULL;
}

///////////////////////////////////////

void AIInitConversationProps(void)
{
   g_pAIConversationProperty = new cAIConversationDescProperty;
   g_pAISaveConversationProperty = CreateBoolProperty(&_g_SaveConversationPropertyDesc,
                                                      kPropertyImplSparseHash);
   AIInitConvActorRelation();
}

///////////////////////////////////////

void AITermConversationProps(void)
{
   SafeRelease(g_pAIConversationProperty);
   AITermConvActorRelation();
}

////////////////////////////////////////////////////////////////////////////////


