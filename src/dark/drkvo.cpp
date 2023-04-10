/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkvo.cpp,v 1.2 2000/02/19 13:08:58 toml Exp $
#include <appagg.h>
#include <drkvo.h>
#include <vocore.h>
#include <relation.h>
#include <linkbase.h>
#include <bintrait.h>
#include <lnkquery.h>
#include <schtype.h>

#include <sdesbase.h>
#include <sdestool.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// AUTO-VOICEOVER links
//

//
// Relation for voice overs 
//

static IRelation*    gpVoiceRel = NULL; 
static ITrait*       gpVoiceTrait = NULL;
static IVoiceOverSys* gpVoiceSys = NULL;  

//
// voice over data flags
// 


struct sVoiceData
{
   ulong events;  // actually an evoice events
}; 

//
// Relation desc and data desc 
// 

static sRelationDesc voice_rdesc = 
{
   "VoiceOver",
   0, // flags
   0, 0, // version
};

static sRelationDataDesc voice_ddesc = LINK_DATA_DESC_FLAGS(sVoiceData,kRelationDataAutoCreate); 


//
// Voice Data Sdesc 
//

static const char* event_names[] = 
{
   "World Hilight",
   "Pick Up", 
}; 

static sFieldDesc voice_fields[] = 
{ 
   { "Events", kFieldTypeBits, FieldLocation(sVoiceData,events), kFieldFlagNone, 0, FieldNames(event_names) },
};

static sStructDesc voice_sdesc = StructDescBuild(sVoiceData,kStructFlagNone,voice_fields); 

//
// Init/Term
//

void DarkVoiceOverInit(void)
{
   AutoAppIPtr_(StructDescTools,pTools); 
   pTools->Register(&voice_sdesc); 
   gpVoiceRel = CreateStandardRelation(&voice_rdesc,&voice_ddesc,kQCaseSetSourceKnown); 
   gpVoiceTrait = MakeTraitFromRelation(gpVoiceRel); 
   gpVoiceSys = AppGetObj(IVoiceOverSys); 
}

void DarkVoiceOverTerm(void)
{
   SafeRelease(gpVoiceRel); 
   SafeRelease(gpVoiceTrait); 
   SafeRelease(gpVoiceSys); 
}

//
// Events
//

EXTERN BOOL DarkVoiceOverEvent(ObjID obj, ulong event)
{
   cAutoIPtr<ILinkQuery> query = QueryInheritedLinksSingle(gpVoiceTrait,gpVoiceRel, obj, LINKOBJ_WILDCARD); 

   for (; !query->Done(); query->Next())
   {
      sVoiceData* data = (sVoiceData*)query->Data(); 
      if (data->events & event)
      {
         sLink link ;
         query->Link(&link); 
         return gpVoiceSys->Play(link.dest) != SCH_HANDLE_NULL; 
      }
   }
   return FALSE; 
}

