/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/editor/convpred.cpp,v 1.4 2000/02/19 12:27:52 toml Exp $
//

#include <proped.h>
#include <iobjed.h>

#include <sdesc.h>
#include <isdesced.h>
#include <isdescst.h>
#include <sdesbase.h>
#include <sdestool.h>

#include <aiprconv.h>
#include <aicnvrse.h>

#include <simpwrap.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// Conversation property editing

//
// The step struct desc
//
static const char* _g_AIConversationActorNames[] = 
{
   "Unused",
   "ActorOne",
   "ActorTwo",
   "ActorThree",
   "ActorFour",
   "ActorFive",
   "ActorSix",
};

static char *_g_AIConversationFlagBits[] = 
{
   "Don't Block",
};

#define CONVERSATION_FIELDS(i) \
   { "Actor", kFieldTypeEnum, FieldLocation(cAIConversationDesc, steps[0][i].actor),  kFieldFlagNone, -1, \
      sizeof(_g_AIConversationActorNames)/sizeof(_g_AIConversationActorNames[0])-1, \
      sizeof(_g_AIConversationActorNames)/sizeof(_g_AIConversationActorNames[0]), \
      _g_AIConversationActorNames}, \
   { "Flags", kFieldTypeBits, FieldLocation(cAIConversationDesc, steps[0][i].flags), kFieldFlagNone, 0, 1, 1, _g_AIConversationFlagBits,}, \
   AIPSDSCR_ARG_FIELD("Conversation: Action " #i, cAIConversationDesc, steps[0][i].act)

static sFieldDesc _g_AIConversationFieldDesc[] =
{
   CONVERSATION_FIELDS(0),
   CONVERSATION_FIELDS(1),
   CONVERSATION_FIELDS(2),
   CONVERSATION_FIELDS(3),
   CONVERSATION_FIELDS(4),
   CONVERSATION_FIELDS(5),
};

static sStructDesc _g_AIConversationStructDesc = StructDescBuild(cAIConversationDesc, kStructFlagNone, _g_AIConversationFieldDesc);

//
// The abort conditions struct desc
//

const char * _g_ppszAIAwareAlertLevels[kAIAL_Num+1] =
{
   "(0) Lowers",
   "(1) Low",
   "(2) Moderate",
   "(3) High",
   "None"
};

static sFieldDesc _g_AIConvAbortFieldDesc[] =
{
   { "Abort level", kFieldTypeEnum, FieldLocation(cAIConversationDesc, abortLevel), kFieldFlagNone, 0, FieldNames(_g_ppszAIAwareAlertLevels) },
   { "Abort priority", kFieldTypeEnum, FieldLocation(cAIConversationDesc, abortPriority), kFieldFlagNone, 0, FieldNames(g_ppszAIPriority) },
};

static sStructDesc _g_AIConvAbortStructDesc = StructDescBuild(cAIConversationDesc, kStructFlagNone, _g_AIConvAbortFieldDesc);

///////////////////////////////////////

void AIConvDescPropSetStep(int step)
{
   sFieldDesc* pField;
   
   // field 0 is abort level
   for (int i=0; i<kAIMaxConvActions; i++)
   {
      // actor
      pField = &_g_AIConversationStructDesc.fields[i*6];
      pField->offset = FieldOffset(cAIConversationDesc, steps[step][i].actor);
      // flags
      pField = &_g_AIConversationStructDesc.fields[i*6+1];
      pField->offset = FieldOffset(cAIConversationDesc, steps[step][i].flags);
      // psuedo-script
      // action type
      pField = &_g_AIConversationStructDesc.fields[i*6+2];
      pField->offset = FieldOffset(cAIConversationDesc, steps[step][i].act.type);
      // action args
      pField = &_g_AIConversationStructDesc.fields[i*6+3];
      pField->offset = FieldOffset(cAIConversationDesc, steps[step][i].act.args[0]);
      pField = &_g_AIConversationStructDesc.fields[i*6+4];
      pField->offset = FieldOffset(cAIConversationDesc, steps[step][i].act.args[1]);
      pField = &_g_AIConversationStructDesc.fields[i*6+5];
      pField->offset = FieldOffset(cAIConversationDesc, steps[step][i].act.args[2]);
   }
}

class cAIConversationEditTrait: public cBasePropEditTrait
{
public:

   cAIConversationEditTrait(IAIConversationProperty* prop)
      : cBasePropEditTrait((IProperty*)prop)
   {
      Caps.flags |= kTraitCanEdit; 
   }

   STDMETHOD(Edit)(ObjID obj)
   {
      cAIConversationDesc* pConvDesc; 
      IAIConversationProperty* prop = (IAIConversationProperty*)Prop; 
      HRESULT retval = S_FALSE; 

      int iNumMenuSlots = kAIMaxConvSteps+1;
      char **ppszMenu = (char **)Malloc(sizeof(char**)*iNumMenuSlots);

      for (int i=0; i<iNumMenuSlots-2; i++)
      {
         ppszMenu[i] = (char*) Malloc(sizeof(char)*3);
         // wacky string sorting...
         if (i<10)
            sprintf(ppszMenu[i], "0%d", i);
         else
            sprintf(ppszMenu[i], "%d", i);
      }
      ppszMenu[i] = (char*) Malloc(sizeof(char)*20);
      sprintf(ppszMenu[i], "Abort Steps");
      ppszMenu[++i] = (char*) Malloc(sizeof(char)*20);
      sprintf(ppszMenu[i], "Abort Conditions");
      int iChoice = PickFromStringList("Step", (const char* const*)ppszMenu, iNumMenuSlots);
      if ((iChoice>=0) && (iChoice<iNumMenuSlots-1))
      {
         AIConvDescPropSetStep(iChoice);

         if (!prop->Get(obj,&pConvDesc))
            return E_FAIL; 
         
         sStructEditorDesc eddesc = { "" , kStructEditAllButtons };
         strncpy(eddesc.title, Desc.strings.friendly_name, sizeof(eddesc.title)); 
         IStructEditor* sed = CreateStructEditor(&eddesc, &_g_AIConversationStructDesc, pConvDesc); 
         
         if (sed->Go(kStructEdModal))
         {
            prop->Set(obj, pConvDesc); 
            retval = S_OK; 
         }
         SafeRelease(sed);
      }
      else if (iChoice == iNumMenuSlots-1)
      {
         if (!prop->Get(obj,&pConvDesc))
            return E_FAIL; 
         
         sStructEditorDesc eddesc = { "" , kStructEditAllButtons };
         strncpy(eddesc.title, Desc.strings.friendly_name, sizeof(eddesc.title)); 
         IStructEditor* sed = CreateStructEditor(&eddesc, &_g_AIConvAbortStructDesc, pConvDesc); 
         
         if (sed->Go(kStructEdModal))
         {
            prop->Set(obj, pConvDesc); 
            retval = S_OK; 
         }
         SafeRelease(sed);
      }
      for (i = 0; i < iNumMenuSlots; ++i)
         Free(ppszMenu[i]);
      Free(ppszMenu);
      return retval; 
   }

   STDMETHOD(Parse)(ObjID obj, const char* val)
   {
      return S_FALSE;
   }

   STDMETHOD(Unparse)(ObjID obj, char* buf, int buflen)
   {
      return S_FALSE; 
   }
};
  
void CreateAIConversationPropEditor(IAIConversationProperty* prop)
{
   IObjEditors* pEditors = AppGetObj(IObjEditors);
   if (pEditors)
   {
      IEditTrait* trait = new cAIConversationEditTrait(prop); 
      pEditors->AddTrait(trait);
      SafeRelease(trait); 
      SafeRelease(pEditors); 
   }
}
