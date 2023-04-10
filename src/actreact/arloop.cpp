/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/actreact/arloop.cpp,v 1.10 1998/10/05 17:19:57 mahk Exp $

#include <comtools.h>
#include <appagg.h>

#include <lg.h>
#include <loopapi.h>

#include <objtype.h>

#include <simtime.h>
#include <dispbase.h>  
#include <loopmsg.h>
#include <dbasemsg.h>
#include <iobjsys.h>
#include <objnotif.h>
#include <objdef.h>

#include <arloop.h>
#include <simloop.h>
#include <objloop.h>

#include <stimuli.h>
#include <stimsens.h>
#include <stimsrc.h>
#include <stimul8r.h>
#include <propag8n.h>
#include <simstate.h>
#include <simflags.h>

// must be last header
#include <dbmem.h>

/////////////////////////////////////////////////////////////
// ACT/REACT LOOP CLIENT
////////////////////////////////////////////////////////////

static IStimuli* pStimuli;
static IStimSensors* pSensors;
static IStimSources* pSources;
static IStimulator*  pStimulator;
static IPropagation* pPropagation; 

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_GUID   LOOPID_ActReact

//
//
// My context data
typedef void Context;

//
// 
// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
} StateRecord;

EXTERN void add_ar_commands(void);

////////////////////////////////////////
// 
// Database message handler
//

static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;
   IUnknown* file = (IUnknown*)data.load;
   
   pStimuli->DatabaseNotify(msg->subtype,file);
   pSensors->DatabaseNotify(msg->subtype,file);
   pSources->DatabaseNotify(msg->subtype,file);

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
         pPropagation->Reset(); 
         break; 
   };

}

static ulong msg_map[] = { kObjNotifyCreate, kObjNotifyDelete}; 

static void obj_message(ObjID obj, eObjNotifyMsg msg, void* )
{
   uObjNotifyData data;
   data.obj = obj; 
   pSensors->ObjectNotify(msg,data.raw); 
   pSources->ObjectNotify(msg,data.raw);
}

static void init_obj_message(void)
{
   AutoAppIPtr_(ObjectSystem,pObjSys); 
   sObjListenerDesc desc = { obj_message, NULL}; 
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
      case kMsgAppInit:
#ifdef EDITOR
         add_ar_commands();
#endif // EDITOR 
         pStimuli = AppGetObj(IStimuli);
         pSensors = AppGetObj(IStimSensors);
         pSources = AppGetObj(IStimSources);
         pStimulator = AppGetObj(IStimulator);
         pPropagation = AppGetObj(IPropagation); 
         init_obj_message(); 
         break;

      case kMsgAppTerm: 
         SafeRelease(pStimuli);
         SafeRelease(pSensors);
         SafeRelease(pSources);
         SafeRelease(pStimulator);
         SafeRelease(pPropagation); 
         break;
         
      case kMsgDatabase:
         db_message(info.dispatch);
         break;

      case kMsgNormalFrame:
      {
         tSimTime t = GetSimTime();
         tSimTime dt = GetSimFrameTime(); 

         if (SimStateCheckFlags(kSimGameSpec))
         {
            pPropagation->Propagate(t,dt); 
            pStimulator->UpdateFrame(t,dt);
         }
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

sLoopClientDesc ActReactLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "ActReact Client",                    // NAME        
   kPriorityNormal,                    // PRIORITY          
   kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter | kMsgDatabase,   // INTERESTS      

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      { kConstrainBefore, &LOOPID_SimFinish, kMsgsFrame },
      { kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase },

      {kNullConstraint} // terminator
   }
};





