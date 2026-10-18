/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/filevar.cpp,v 1.6 1998/09/22 14:13:24 mahk Exp $

#include <appagg.h>
#include <filevar.h>
#include <dlistsim.h>
#include <dlisttem.h>
#include <string.h>

#include <loopapi.h>
#include <dispbase.h>
#include <dbasemsg.h>
#include <dbfile.h>
#include <loopmsg.h>
#include <dispbase.h>
#include <comtools.h>

#include <campaign.h>
#include <cmpgnmsg.h>
#include <dspchdef.h>

#include <config.h>
#include <cfgdbg.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <fvarloop.h>




static cFileVarList* gVarList = NULL; 

static inline cFileVarList& VarList()
{
   if (!gVarList)
      gVarList = new cFileVarList; 
   return *gVarList; 
}


//
// cFileVarBase
//


cFileVarBase::cFileVarBase()
{
   mLoadedVersion.major = 0;
   mLoadedVersion.minor = 0;
   VarList().Append(this); 
}

cFileVarBase::~cFileVarBase()
{
   for (cFileVarList::cIter iter = VarList().Iter(); !iter.Done(); iter.Next())
   {
      if (iter.Value() == this)
      {
         VarList().Delete(iter.Node()); 
         break; 
      }
   }

   if (VarList().GetFirst() == NULL) // empty list, delete
   {
      delete gVarList;
      gVarList = NULL; 
   }
}

void cFileVarBase::Reset()
{
   memset(mBlock,0,mSize); 
}


BOOL cFileVarBase::SetupFile(ITagFile* file)
{
   if (mDesc->game)
   {
      char buf[64];  
      config_get_raw("game",buf,sizeof(buf)); 
      if (strnicmp(buf,mDesc->game,sizeof(buf)) != 0)
         return FALSE;
   }

   if (file->OpenMode() == kTagOpenWrite &&
       !ShouldWrite())
      return FALSE;

   if (file->OpenMode() == kTagOpenWrite &&
       dbCompatibilityUseRawTag(&mDesc->tag))
      return FALSE;

   const TagVersion maxRead = ReadVersion();
   TagVersion v = file->OpenMode() == kTagOpenRead ? maxRead : WriteVersion();
   if (!SUCCEEDED(file->OpenBlock(&mDesc->tag,&v)))
   {
      if (file->OpenMode() == kTagOpenRead &&
          file->BlockSize(&mDesc->tag) != kTagNoSuchBlock)
      {
         char diagnostic[256];
         _snprintf(diagnostic,sizeof(diagnostic)-1,
                   "Required file-variable %.12s has a malformed or unsupported version",
                   mDesc->tag.label);
         diagnostic[sizeof(diagnostic)-1] = '\0';
         dbLoadReportFailure("file-variable",diagnostic);
      }
      return FALSE; 
   }

   if (file->OpenMode() == kTagOpenRead &&
       VersionNumsCompare(&v,&maxRead) > 0)
   {
      Warning(("Unsupported file-variable version %u.%u for %s (max %u.%u)\n",
               v.major,v.minor,mDesc->tag.label,maxRead.major,maxRead.minor));
      file->CloseBlock();
      char diagnostic[256];
      _snprintf(diagnostic,sizeof(diagnostic)-1,
                "Required file-variable %.12s version %u.%u is newer than %u.%u",
                mDesc->tag.label,v.major,v.minor,maxRead.major,maxRead.minor);
      diagnostic[sizeof(diagnostic)-1] = '\0';
      dbLoadReportFailure("file-variable",diagnostic);
      return FALSE;
   }
   mLoadedVersion = v;

   if (file->OpenMode() == kTagOpenRead &&
       VersionNumsCompare(&v,&mDesc->version) > 0)
   {
      dbCompatibilityMarkTag(&mDesc->tag,kDBCompatPrefixDecoded,
         "newer block decoded; original retained for legacy writer");
      dbCompatibilityRetainRawTag(&mDesc->tag,NULL);
   }

   if (VersionNumsCompare(&v,&mDesc->last_good_version) < 0)
   {
      file->CloseBlock(); 
      char diagnostic[256];
      _snprintf(diagnostic,sizeof(diagnostic)-1,
                "Required file-variable %.12s version %u.%u predates the oldest safe codec %u.%u",
                mDesc->tag.label,v.major,v.minor,
                mDesc->last_good_version.major,mDesc->last_good_version.minor);
      diagnostic[sizeof(diagnostic)-1] = '\0';
      dbLoadReportFailure("file-variable",diagnostic);
      return FALSE; 
   }
   return TRUE; 
}

void cFileVarBase::CleanupFile(ITagFile* file)
{
   file->CloseBlock(); 
}

void cFileVarBase::DatabaseMsg(ulong msg, ITagFile* file)
{
   switch (DB_MSG(msg))
   {
      case kDatabaseReset:
         ConfigSpew("filevar_spew",("Resetting %s\n",mDesc->friendly_name)); 
         Reset(); 
         break; 

      case kDatabaseSave:
         if (SetupFile(file))
         {
            ConfigSpew("filevar_spew",("Saving %s\n",mDesc->friendly_name)); 
            WriteBlock(file);
            CleanupFile(file); 
         }
         break; 

      case kDatabaseLoad:
         if (SetupFile(file))
         {
            ConfigSpew("filevar_spew",("Loading %s\n",mDesc->friendly_name)); 
            if (!ReadBlock(file,mLoadedVersion))
            {
               char diagnostic[192];
               _snprintf(diagnostic,sizeof(diagnostic)-1,
                         "Required file-variable %.12s is truncated or malformed",
                         mDesc->tag.label);
               diagnostic[sizeof(diagnostic)-1] = '\0';
               dbLoadReportFailure("file-variable",diagnostic);
            }
            CleanupFile(file); 
         }
         break; 
      case kDatabasePostLoad:
      case kDatabaseDefault:
         Update(); 
         break; 
   }
}

cFileVarList* cFileVarBase::AllFileVars()
{
   return gVarList; 
}

////////////////////////////////////////////////////////////
// LOOP CLIENT
//

class cFileVarClient : public cCTUnaggregated<ILoopClient,&IID_ILoopClient,kCTU_NoSelfDelete> 
{
public:
   STDMETHOD_(short, GetVersion)(THIS) { return kVerLoopClient; }; 
   STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return &FileVarClientDesc; }; 
   STDMETHOD_(eLoopMessageResult,ReceiveMessage)(eLoopMessage, tLoopMessageData); 

}; 

static cFileVarClient gFileVarClient;  // singleton client 

sLoopClientDesc FileVarClientDesc =
{
   &LOOPID_FileVar,
   "File Variable Client",
   kPriorityLibrary,                    // PRIORITY
   kMsgEnd | kMsgDatabase | kMsgAppInit, 

   kLCF_Singleton,
   &gFileVarClient,

   NO_LC_DATA,

   {
      {kNullConstraint} // terminator
   }
};





static void SendDatabaseMessageToType(ulong msg, ITagFile* file, eWhichVarFile which)
{
   cFileVarList::cIter iter; 
   for (iter = VarList().Iter(); !iter.Done(); iter.Next())
   {
      cFileVarBase* fv = iter.Value(); 
      if (fv->mDesc->file == which)
         fv->DatabaseMsg(msg,file);
   }
   
}

static void setup_campaign();

STDMETHODIMP_(eLoopMessageResult) cFileVarClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData hData)
{
   LoopMsg info; 
   info.raw = hData;
   
   if (msg == kMsgDatabase)
   {
      ulong dbmessage = info.dispatch->subtype; 
      ITagFile* file = (ITagFile*)info.dispatch->data; 

      ulong mask = dbmessage; 
      if (dbmessage == kDatabaseReset)
         mask |= kDBMission|kDBGameSys; 

      if (mask & kDBMission)
         SendDatabaseMessageToType(dbmessage,file,kMissionVar); 

      if (mask & kDBGameSys)
         SendDatabaseMessageToType(dbmessage,file,kGameSysVar); 
         
   }
   if (msg == kMsgAppInit)
      setup_campaign(); 

   return kLoopDispatchContinue; 
}

////////////////////////////////////////////////////////////
// CAMPAIGN MESSAGE HANDLER
//

static void campaign_cb(const sDispatchMsg* dmsg, const sDispatchListenerDesc* desc)
{
   sCampaignMsg* msg = (sCampaignMsg*)dmsg; 
   SendDatabaseMessageToType(msg->db_kind,msg->file,kCampaignVar); 
}

static sDispatchListenerDesc camp_desc = 
{
   &LOOPID_FileVar, // guid
   kCampaignAll,
   campaign_cb,
   NULL,
}; 

static void setup_campaign()
{
   AutoAppIPtr(Campaign); 
   pCampaign->Listen(&camp_desc); 
}


