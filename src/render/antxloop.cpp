/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/antxloop.cpp,v 1.3 1998/09/08 13:48:24 mahk Exp $

#include <lg.h>
#include <loopapi.h>

#include <loopmsg.h>
#include <dispatch.h>
#include <dispbase.h>
#include <dbasemsg.h>
#include <iobjsys.h>
#include <objnotif.h>
#include <objdef.h>

#include <antxloop.h>
#include <antxtype.h>

#include <port.h>
#include <wrfunc.h>
#include <wrdbrend.h>
#include <objpos.h>
#include <texmem.h>

#include <appagg.h>
#include <str.h>
#include <mprintf.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Must be last header
#include <dbmem.h>
#include <tagfile.h>
#include <vernum.h>

//////////////////
// CONSTANTS
//
//
//
#define MY_GUID    LOOPID_AnimTexture

cTexturePatchTable gTexturePatchTable;
cTexturePatchTable gTextureInverseTable;

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
// tag file structures

TagFileTag TexturePatchTag = { "TXTPAT_DB" };
TagVersion TexturePatchVersion = { 0, 1 };

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

////////////////////////////////////////////////////////////
// Do the save
//
static void AnimTextureSave(msgDatabaseData *data)
{
   int len,val;
   cTexturePatchTable::cIter iter;

   // dump the contents of the hashtable out to disk
   if (setup_tagfile(data->save, &TexturePatchTag,
                     &TexturePatchVersion))
   {
      len = gTexturePatchTable.nElems();
      movefunc((void *)&len, sizeof(int), 1);
      iter = gTexturePatchTable.Iter();
      while (!iter.Done())
      {
         val = iter.Key();
         movefunc((void *)&val, sizeof(int), 1);
         val = iter.Value();
         movefunc((void *)&val, sizeof(int), 1);

         iter.Next();
      }

      cleanup_tagfile(data->save);
   }
}

////////////////////////////////////////////////////////////
// Actually apply the patches, like craze, detox, etc.
//
void AnimTextureApplyPatches(cTexturePatchTable *pTable)
{
   cTexturePatchTable::cIter iter;
   int cellid,polyid,texval;
   iter = pTable->Iter();
   PortalPolygonRenderInfo *ppri;
   PortalCell *pcell;

   // for each entry in the patch table, break it down into the cell and renderpoly
   // ids, then go poke in the correct new values.
   while (!iter.Done())
   {
      cellid = iter.Key() >> 8;
      polyid = iter.Key() & 0xFF;
      texval = iter.Value();

      pcell = WR_CELL(cellid);
      ppri = &pcell->render_list[polyid];
      ppri->texture_id = texval;
      iter.Next();
   }
}

////////////////////////////////////////////////////////////
// Do the load
//
static void AnimTextureLoad(msgDatabaseData *data)
{
   int len, val, key;
   int i;

   // read back in the patch table
   if (setup_tagfile(data->load, &TexturePatchTag,
                     &TexturePatchVersion))
   {
      movefunc((void *)&len, sizeof(int), 1);
      for (i=0; i < len; i++)
      {
         movefunc((void *)&key, sizeof(int), 1);
         movefunc((void *)&val, sizeof(int), 1);
         gTexturePatchTable.Set(key,val);
      }
      cleanup_tagfile(data->load);
   }

   // now read through the table and apply the patches
   AnimTextureApplyPatches(&gTexturePatchTable);
}

////////////////////////////////////////////////////////////
// Database message handler
//

static void db_message(DispatchData* msg)
{
   msgDatabaseData data; 
   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
      case kDatabaseDefault:
         gTexturePatchTable.Clear();
         break;

      case kDatabaseSave:
         if (msg->subtype & kDBMission)
            AnimTextureSave(&data);
         break;

      case kDatabaseLoad:
         if (msg->subtype & kDBMission)
            AnimTextureLoad(&data);
         break;
   }
}

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//
 
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

sLoopClientDesc AnimTextureLoopClientDesc =
{
   &MY_GUID,
   "Animating Texture System",              
   kPriorityNormal,              
   kMsgDatabase | kMsgsAppOuter, // interests 

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      {kNullConstraint}
   }
};

