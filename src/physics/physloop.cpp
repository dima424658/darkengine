/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/physics/physloop.cpp,v 1.50 2000/02/19 12:32:27 toml Exp $

#include <lg.h>
#include <loopapi.h>

#include <loopmsg.h>
#include <dispbase.h>
#include <dbasemsg.h>

#include <idispatc.h>
#include <simman.h>
#include <dspchdef.h>
#include <simdef.h>

#include <physloop.h>
#include <ailoop.h>
#include <simloop.h>
#include <plyrloop.h>
#include <objloop.h>
#include <brloop.h>
#include <linkloop.h>
#include <wrloop.h>
#include <roomloop.h>

#include <simstate.h>
#include <simflags.h>
#include <simtime.h>

#include <particle.h>
#include <texprop.h> // for terrainprop_load which we database message here...
#include <schloop.h> // to constrain db messages after schemas

#include <objtype.h>
#include <objdef.h>
#include <objnotif.h>
#include <iobjsys.h>
#include <appagg.h>

#include <phcontct.h>
#include <physapi.h>
#include <phmoapi.h>
#include <phmterr.h>
#include <phref.h>
#include <phnet.h>

#include <tagfile.h>
#include <vernum.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


/////////////////////////////////////////////////////////////
// PHYSICS LOOP CLIENT
////////////////////////////////////////////////////////////

// I weep openly.
EXTERN void ParticleGroupUpdate(void); 

//////////////////
// SIMULATION
//

static void sim_msg(const sDispatchMsg* msg, const sDispatchListenerDesc* )
{
   switch (msg->kind)
   {
      case kSimSuspend:
         break;
   }
}

static sDispatchListenerDesc sim_listen =
{
   &LOOPID_Physics,       // my guid
   kSimInit|kSimResume,   // interests
   sim_msg,
};

static void init_sim_message()
{
   AutoAppIPtr_(SimManager,pSimMan);
   pSimMan->Listen(&sim_listen);
}

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_GUID   LOOPID_Physics

//
// My context data
typedef void Context;
//
// My state
typedef struct _StateRecord
{
   Context *context;                             // a pointer to the context data I got.
   // State fields go here
} StateRecord;


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
         PhysDeregisterAllModels();
         ParticlesInit(); 
         PhysRefSystemClear();
         ResetPlayerMotion();
         g_PhysContactLinks.Reset();
         PhysResetListeners();
         break;

      case kDatabasePostLoad:
         if (msg->subtype & kObjPartConcrete)
         {
            if (msg->subtype & kDBMap)
               terrainprop_load();
            PhysRefSystemRebuild();
            PhysUpdateProperties();
            PhysInitializeContacts();
            PhysPostLoad();
#ifdef NEW_NETWORK_ENABLED
            PhysNetPostLoad();
#endif
         }
         break; 

   }
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
         ParticlesInformOfDeletion(obj);
         PhysDeregisterModel(obj);
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
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//

#pragma off(unreferenced)
static eLoopMessageResult LGAPI _LoopFunc(void *data, eLoopMessage msg, tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue;
   StateRecord *state = (StateRecord *) data;
   LoopMsg info;
   info.raw = hdata;

   switch (msg)
   {
      case kMsgAppInit:
         ParticlesInit();
         PhysInit();
         PhysRefSystemInit();
         InitPlayerMotion();
         init_obj_message(); 
         init_sim_message();
         break;

      case kMsgAppTerm:
         TermPlayerMotion();
         PhysRefSystemTerm();
         PhysTerm();
         break;

      case kMsgEnterMode:
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;

      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimPhysics))
         {
            PlayerMotionUpdate(GetSimFrameTime());
            PhysUpdate(GetSimFrameTime());
            ParticlesUpdate(GetSimFrameTime());  
            UpdateMovingTerrain(GetSimFrameTime());
            ParticleGroupUpdate();
            PhysNetFrame();
         }
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
static ILoopClient *LGAPI _CreateClient(sLoopClientDesc * pDesc, tLoopClientData data)
{
   StateRecord *state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord *) Malloc(sizeof(StateRecord));
   state->context = (Context *) data;

   return CreateSimpleLoopClient(_LoopFunc, state, pDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//
sLoopClientDesc PhysicsLoopClientDesc =
{
   &MY_GUID,                                     // GUID
   "Physics Client",                             // NAME
   kPriorityNormal,                              // PRIORITY
   kMsgEnd | kMsgsFrameMid | kMsgsAppOuter | kMsgDatabase | kMsgsMode,  

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      {kConstrainAfter,  &LOOPID_AI, kMsgsFrame},
      {kConstrainAfter,  &LOOPID_LinkMan, kMsgsAppOuter},
      {kConstrainAfter,  &LOOPID_ObjSys, kMsgsAppOuter},
      {kConstrainAfter,  &LOOPID_Wr, kMsgsAppOuter},
      {kConstrainAfter,  &LOOPID_Room, kMsgsAppOuter},
      {kConstrainBefore, &LOOPID_ObjSys, kMsgDatabase},
      {kConstrainBefore, &LOOPID_Player, kMsgsFrame},
      {kConstrainBefore, &LOOPID_SimFinish, kMsgsFrame},
      {kConstrainBefore, &LOOPID_ObjSys, kMsgsFrame},
      {kConstrainBefore, &LOOPID_BrushList, kMsgDatabase},
      {kNullConstraint}                             // terminator
   }
};

//
// Base mode client, just database messages
//

sLoopClientDesc PhysicsBaseClientDesc =
{
   &LOOPID_PhysicsBase,                                     // GUID
   "Physics Base Client",                             // NAME
   kPriorityNormal,                              // PRIORITY
   kMsgEnd | kMsgDatabase ,  

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      {kConstrainBefore, &LOOPID_ObjSys, kMsgDatabase},
      {kConstrainBefore, &LOOPID_BrushList, kMsgDatabase},
      {kNullConstraint}                             // terminator
   }
};

