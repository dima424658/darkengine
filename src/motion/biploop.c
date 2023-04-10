/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/motion/biploop.c,v 1.28 1999/08/05 17:04:40 Justin Exp $

#include <lg.h>
#include <loopapi.h>
#include <dispatch.h>
#include <dispbase.h>
#include <loopmsg.h>
#include <simman.h>
#include <simdef.h>
#include <simloop.h>
#include <dspchdef.h>
#include <appagg.h>
#include <iobjsys.h>
#include <objdef.h>

#include <dbasemsg.h>
#include <tagfile.h>
#include <vernum.h>

#include <objloop.h>
#include <biploop.h>
#include <creatext.h>
#include <objmodel.h>
#include <timer.h>

#include <simstate.h>
#include <simflags.h>
#include <simtime.h>
#include <simloop.h>

#include <puppet.h>
#include <motmngr.h>
#include <mvrflags.h>
#include <mvrutils.h>

// This must be last
#include <dbmem.h>


/////////////////////////////////////////////////////////////
// SAMPLE LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
//
#define MY_GUID   LOOPID_Biped

//
// @INSTRUCTIONS: If your client requires context data, you should publish the 
// type of the context data, and then change the "void" in the typedef below
// to your context data type.  See Resloop.c and .h for an example.
//
typedef void Context;

//
// @INSTRUCITONS: If your client requires any state, add the state variables as 
// fields to this structure. 
// 
typedef struct _StateRecord
{
   Context* context;
} StateRecord;



//////////////////////////////////////////////////
// Tag File Stuff

static ITagFile* tagfile = NULL;
static void movefunc(void *buf, size_t elsize, size_t nelem)
{
   ITagFile_Move(tagfile,(char*)buf,elsize*nelem);
}

TagFileTag CreatureSystemTag = { "CRET_SYSTEM" };
TagVersion CreatureSystemVersion = { 0, 1 };

static BOOL setup_tagfile(ITagFile* file, TagFileTag *tag, 
                          TagVersion *version)
{
   HRESULT result;
   TagVersion found_version = *version;
   tagfile = file;

   result = ITagFile_OpenBlock(file, tag, &found_version);
   return (result == S_OK
        && found_version.major == version->major
        && found_version.minor == version->minor);
}

static void cleanup_tagfile(ITagFile* file)
{
   ITagFile_CloseBlock(file);
}

EXTERN BOOL g_SimRunning;

static void sim_msg(const sDispatchMsg* msg, const sDispatchListenerDesc* desc)
{
   switch (msg->kind)
   {
      case kSimInit:
      case kSimResume:
      {
         InitGroundHeightObjects();
         g_SimRunning = TRUE;
         break;
      }

      case kSimSuspend:
      case kSimTerm: // @Q (toml 09-15-98): unused branch?
      {
         g_SimRunning = FALSE;
         break;
      }
   }
}

static sDispatchListenerDesc sim_listen =
{
   &LOOPID_Biped,       // my guid
   kSimInit|kSimResume|kSimSuspend|kSimTerm,   // interests
   sim_msg,
};

static void init_sim_message()
{
   ISimManager *pSimMan = AppGetObj(ISimManager);
   IMessageDispatch_Listen(pSimMan, &sim_listen);
   SafeRelease(pSimMan);
}

////////////////////////////////////////
//
// OBJECT MESSAGE HANDLER
//
#pragma off(unreferenced)
static void obj_message(ObjID obj, eObjNotifyMsg msg, void* data)
{
   switch (msg) 
   {
      case kObjNotifyDelete:
         ClearMotionFlagListeners(obj);
         break;
   }
}
#pragma on(unreferenced)

static void init_obj_message(void)
{
   IObjectSystem* pObjSys = AppGetObj(IObjectSystem); 
   sObjListenerDesc desc = { obj_message, NULL }; 
   IObjectSystem_Listen(pObjSys,&desc);
   SafeRelease(pObjSys);    
}

////////////////////////////////////////
// DATABASE MESSAGE HANDLER
//
static void db_message(DispatchData * msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         MotionManagerReset();
         TermGroundHeightObjects();
         break;

      // Hey, look, we really should be loading motion stuff out of the briefcase too
      // but the motion system doesn't seem to handle merge loading, so for the moment
      // we are just going to load out of the mission. Someone can fix this for real
      // sometime in the future (after Shock ships). (Jon 5/27/99)
      case kDatabaseSave:
         if (msg->subtype & kObjPartConcrete)
         {
            if (setup_tagfile(data.save, &CreatureSystemTag, 
                              &CreatureSystemVersion)) {
               CreaturesWrite((fCreatureReadWrite)movefunc,NOTIFY_PARTITION(msg->subtype));
               cleanup_tagfile(data.save);
            }
         }
         break;

      case kDatabaseLoad:
         if (msg->subtype & kDBMission)
         {
            MotionManagerLoad();
         }
         if (msg->subtype & kObjPartConcrete)
         {
            if (setup_tagfile(data.load, &CreatureSystemTag, 
                              &CreatureSystemVersion)) {
               CreaturesRead((fCreatureReadWrite)movefunc,NOTIFY_PARTITION(msg->subtype));
               cleanup_tagfile(data.load);
            }
         }
         break;
         
      case kDatabasePostLoad:
         if (msg->subtype & kObjPartConcrete)
         {
            InitGroundHeightObjects();
         }
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
   long cur_time;

   info.raw = hdata; 

   switch(msg)
   {

      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimAI))
         {
            cur_time = GetSimFrameTime();
            PuppetsUpdate(cur_time);
            CreaturesUpdate(cur_time);
            // update other multipeds here too
         }
         break;

      case kMsgAppInit:
         MotionManagerInit();
         MotionFlagsInit();
         PuppetsInit();
         init_sim_message();
         init_obj_message(); 
         break;

      case kMsgAppTerm:
         MotionFlagsTerm();
         MotionManagerClose();
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
// Frome here on in is boiler plate code.
// Nothing need be changed.
//


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
   
   return CreateSimpleLoopClient(_LoopFunc,state,&BipedLoopClientDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
// 

sLoopClientDesc BipedLoopClientDesc =
{
   &MY_GUID,
   "Biped Client",              
   kPriorityNormal,              
   kMsgEnd | kMsgsFrameMid | kMsgsAppOuter | kMsgDatabase,

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

// XXX should constrain this to happen BEFORE renderer.  But where is renderer?
   {
      {kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase},
      {kConstrainAfter, &LOOPID_ObjSys, kMsgsAppOuter}, // to install listener
#if 0
      {kConstrainAfter, &LOOPID_ObjSys, kMsgAppInit}, // to install listener
      {kConstrainBefore, &LOOPID_ObjSys, kMsgAppTerm}, // to detach weapons
#endif
      {kConstrainBefore, &LOOPID_SimFinish, kMsgsFrameMid},

      {kNullConstraint} // terminator
   }
};


