/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/quesloop.cpp,v 1.9 1998/10/27 12:20:38 mahk Exp $

#include <lg.h>
#include <loopapi.h>

#include <loopmsg.h>
#include <dispatch.h>
#include <dispbase.h>
#include <dbasemsg.h>
#include <iobjsys.h>
#include <objnotif.h>
#include <objdef.h>

#include <quesloop.h>
#include <quesedit.h>

#include <appagg.h>
#include <questapi.h>
#include <str.h>
#include <mprintf.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <command.h>
#include <simpwrap.h>
#include <quesfile.h>

#include <campaign.h>
#include <cmpgnmsg.h>
#include <dspchdef.h>

#include <sdesc.h>
#include <sdesbase.h>
#include <isdesced.h>
#include <isdescst.h>

// Must be last header
#include <dbmem.h>
#include <tagfile.h>
#include <vernum.h>

//////////////////
// CONSTANTS
//
//
//
#define MY_GUID    LOOPID_Quest

//
// Context data
typedef void Context;

//
// State record
typedef struct _StateRecord
{
   Context* context;
} StateRecord;

//////////////////////////////////////////////////
// Quest system tag file structures

TagFileTag QuestSystemTags[2] = { { "QUEST_DB" }, {"QUEST_CMP" } };
TagVersion QuestSystemVersion = { 0, 1 };

static ITagFile* tagfile = NULL;
static long movefunc(void *buf, size_t elsize, size_t nelem)
{
   return ITagFile_Move(tagfile,(char*)buf,elsize*nelem);
}

static BOOL setup_tagfile(ITagFile* file, TagFileTag *tag,
                          TagVersion *version)
{
   HRESULT result;
   TagVersion found_version = *version;
   tagfile = file;

   result = file->OpenBlock(tag, &found_version);
   if (!VersionNumsEqual(&found_version,version))
   {
      file->CloseBlock(); 
      return FALSE; 
   }

   return result == S_OK;
}


static void cleanup_tagfile(ITagFile* file)
{
   file->CloseBlock();
}


EXTERN void QuestDataLoadTagFile(eQuestDataType type, ITagFile* file) 
{
   if (setup_tagfile(file, &QuestSystemTags[type],
                     &QuestSystemVersion))
   {
      AutoAppIPtr(QuestData); 
      pQuestData->Load(movefunc, type);
      cleanup_tagfile(file);
   }
}

EXTERN void QuestDataSaveTagFile(eQuestDataType type, ITagFile* File)
{

   if (setup_tagfile(File, &QuestSystemTags[type],
                     &QuestSystemVersion))
   {
      AutoAppIPtr(QuestData); 
      pQuestData->Save(movefunc, type);
      cleanup_tagfile(File);
   }
}


////////////////////////////////////////////////////////////
// Database message handler
//

static void db_message(DispatchData* msg)
{
   AutoAppIPtr(QuestData);
   msgDatabaseData data; 
   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         // Hey, the campaign isn't a part of this, OK? 
         pQuestData->DeleteAllType(kQuestDataMission);
         pQuestData->UnsubscribeAll(); 
         break; 

      case kDatabaseDefault:
         break;

      case kDatabaseSave:
         if (msg->subtype & kDBMission)
            QuestDataSaveTagFile(kQuestDataMission,data.save); 

         break;

      case kDatabaseLoad:
         if (msg->subtype & kDBMission)
            QuestDataLoadTagFile(kQuestDataMission,data.load); 

         break;
   }
}

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

void setup_campaign(); 
 
#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void* data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;

   info.raw = hdata; 

   switch(msg)
   {
      case kMsgAppInit:
         setup_campaign(); 
#ifdef EDITOR
         QuestEditInit();
#endif
         break;

      case kMsgAppTerm:
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;

      case kMsgEnd:
         Free(state);
         break;   
   }
   return result;
}

////////////////////////////////////////////////////////////
// 
// Loop client factory function. 
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)Malloc(sizeof(StateRecord));
   state->context = (Context*)data;
   
   return CreateSimpleLoopClient(_LoopFunc,state,pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
// 

sLoopClientDesc QuestLoopClientDesc =
{
   &MY_GUID,
   "Quest Data System",              
   kPriorityNormal,              
   kMsgDatabase | kMsgsAppOuter | kMsgEnd, // interests 

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      {kNullConstraint}
   }
};

////////////////////////////////////////////////////////////
// CAMPAIGN LISTENER
//

static void campaign_cb(const sDispatchMsg* dmsg, const sDispatchListenerDesc* desc)
{
   AutoAppIPtr(QuestData); 
   sCampaignMsg* msg = (sCampaignMsg*)dmsg; 
   switch(msg->kind)
   {
      case kCampaignReset:
         pQuestData->DeleteAllType(kQuestDataCampaign); 
         break; 

      case kCampaignSave:
         QuestDataSaveTagFile(kQuestDataCampaign,msg->file);
         break; 

      case kCampaignLoad:
         QuestDataLoadTagFile(kQuestDataCampaign,msg->file);
         break; 
   }

}

static sDispatchListenerDesc camp_desc = 
{
   &MY_GUID, // guid
   kCampaignAll,
   campaign_cb,
   NULL,
}; 

static void setup_campaign()
{
   AutoAppIPtr(Campaign); 
   pCampaign->Listen(&camp_desc); 
}

