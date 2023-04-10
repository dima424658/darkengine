/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/roomloop.cpp,v 1.31 1999/12/01 20:10:36 BFarquha Exp $

#include <lg.h>
#include <loopapi.h>

#include <loopmsg.h>
#include <dispatch.h>
#include <dispbase.h>
#include <dbasemsg.h>
#include <iobjsys.h>
#include <objnotif.h>
#include <objdef.h>

#include <idispatc.h>
#include <simman.h>
#include <dspchdef.h>
#include <simdef.h>

#include <simloop.h>
#include <objloop.h>
#include <drkloop.h>
#include <schloop.h>
#include <ailoop.h>

#include <roomsys.h>
#include <roomeax.h>
#include <roomprop.h>
#include <psndapi.h>

#include <rooms.h>
#include <rpaths.h>
#include <roomloop.h>
#include <ged_room.h>

#include <gamemode.h>

// Must be last header
#include <dbmem.h>
#include <tagfile.h>
#include <vernum.h>

//////////////////
// CONSTANTS
//
//
//
#define MY_GUID    LOOPID_Room

//
// Context data
typedef void Context;

//
// State record
typedef struct _StateRecord
{
   Context* context;
} StateRecord;

static BOOL sgInGameMode = FALSE;

//////////////////////////////////////////////////
// Room system tag file structures

TagFileTag RoomSystemTag = { "ROOM_DB" };
TagVersion RoomSystemVersion = { 0, 4 };

TagFileTag AIRoomDBTag = { "AI_ROOM_DB" };
TagVersion AIRoomDBVersion = { 0, 5 };

TagFileTag RoomEAXTag = { "ROOM_EAX" };
TagVersion RoomEAXVersion = { 0, 1 };

static ITagFile* tagfile = NULL;
static void movefunc(void *buf, size_t elsize, size_t nelem)
{
   ITagFile_Move(tagfile,(char*)buf,elsize*nelem);
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

//////////////////
// Simulation
//

static void sim_msg(const sDispatchMsg* msg, const sDispatchListenerDesc* )
{
   switch (msg->kind)
   {
      case kSimInit:
         InitObjRooms();
         break;
   }
}

static sDispatchListenerDesc sim_listen =
{
   &LOOPID_Room,    // my guid
   kSimInit,        // interests
   sim_msg,
};

static void init_sim_message()
{
   AutoAppIPtr_(SimManager,pSimMan);
   pSimMan->Listen(&sim_listen);
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
         ResetRoomEAX();
         SoundPropagationReset();
         ClearRooms();
         ResetAIRoomDatabase();
         break;

      case kDatabaseSave:
         // only save if we're saving the terrain partition
         if ((msg->subtype & (kObjPartConcrete|kObjConcreteSubparts)) == kObjPartConcrete
             || (msg->subtype & kObjPartTerrain))
         {
            if (setup_tagfile(data.save, &RoomSystemTag, &RoomSystemVersion))
            {
               SaveRooms(movefunc);
               cleanup_tagfile(data.save);
            }
            if (setup_tagfile(data.save, &AIRoomDBTag, &AIRoomDBVersion))
            {
               SaveAIRoomDatabase(movefunc);
               cleanup_tagfile(data.save);
            }
            if (setup_tagfile(data.save, &RoomEAXTag, &RoomEAXVersion))
            {
               g_pRoomEAX->Write(data.save);
               cleanup_tagfile(data.save);
            }
         }

         break;

      case kDatabaseDefault:
         PostLoadRooms();

      case kDatabasePostLoad:
         if ((msg->subtype & kFiletypeAll) == 0)
            PostLoadRooms();
         break;

      case kDatabaseLoad:
         if (msg->subtype & kObjPartConcrete)
         {
            if (setup_tagfile(data.load, &RoomSystemTag, &RoomSystemVersion))
            {
               LoadRooms(movefunc);
               cleanup_tagfile(data.load);
            }
            if (setup_tagfile(data.load, &AIRoomDBTag, &AIRoomDBVersion))
            {
               LoadAIRoomDatabase(movefunc);
               cleanup_tagfile(data.save);
            }
            if (setup_tagfile(data.load, &RoomEAXTag, &RoomEAXVersion))
            {
               g_pRoomEAX->Read(data.load);
               cleanup_tagfile(data.load);
            }
         }
         break;
   }
}


////////////////////////////////////////////////////////////
// Database message handler
//
static void obj_message(ObjID obj, eObjNotifyMsg msg, void* )
{
   switch (msg)
   {
      case kObjNotifyDelete:
         g_pRooms->SetObjRoom(obj, NULL);
         break;
   }
}

static void install_obj_message(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys);
   sObjListenerDesc desc = { obj_message, NULL };
   pObjSys->Listen(&desc);
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
      case kMsgResumeMode:
      case kMsgEnterMode:
         sgInGameMode = ( *(info.mode->to.pID) == LOOPID_GameMode );
         break;

      case kMsgExitMode:
      case kMsgSuspendMode:
         sgInGameMode = FALSE;
         break;

      case kMsgAppInit:
         InitRooms();
         InitRoomEAX();
         SoundPropagationInit();
         InitAIRoomDatabase();

         AcousticsPropInit();
         AmbientPropInit();
         RoomGravPropInit();
         LoudRoomInit();
         install_obj_message();
         init_sim_message();
         break;

      case kMsgNormalFrame:
         if( sgInGameMode )
            ApplyPropSounds();
         break;

      case kMsgAppTerm:
         TermAIRoomDatabase();
         SoundPropagationTerm();
         TermRoomEAX();
         TermRooms();

         AcousticsPropTerm();
         AmbientPropTerm();
         RoomGravPropTerm();
         LoudRoomTerm();
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

sLoopClientDesc RoomLoopClientDesc =
{
   &MY_GUID,
   "Room System",
   kPriorityNormal,
   kMsgDatabase | kMsgsMode | kMsgsAppOuter | kMsgEnd | kMsgsFrame, // interests

   kLCF_Callback,
   _CreateClient,

   NO_LC_DATA,

   {
      {kConstrainBefore, &LOOPID_SimFinish, kMsgsFrame},
      {kConstrainBefore, &LOOPID_DarkSim, kMsgDatabase},
      {kConstrainBefore, &LOOPID_Schema, kMsgsAppOuter},
      {kConstrainAfter,  &LOOPID_AI, kMsgDatabase|kMsgsAppOuter},
      {kConstrainAfter,  &LOOPID_ObjSys, kMsgsAppOuter},
      {kNullConstraint}
   }
};
