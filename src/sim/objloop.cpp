/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/objloop.cpp,v 1.32 2000/02/19 12:36:58 toml Exp $

#include <lg.h>
#include <loopapi.h>

#include <loopmsg.h>

#include <appagg.h>
#include <iobjsys.h>
#include <objloop.h>
#include <eosapp.h>
#include <objpos.h>
#include <editmode.h>
#include <gamemode.h>
#include <dbasemsg.h>
#include <dispbase.h>
#include <objnotif.h>
#include <pfacinit.h>
#include <phref.h> 
#include <linkint.h>

#include <simloop.h>
#include <physloop.h>
#include <wrloop.h>

#include <objsys.h>
#include <command.h>
#include <scalprop.h>
#include <mnamprop.h>
#include <mnumprop.h>
#include <collprop.h>
#include <partprop.h>
#include <litprop.h>
#include <elitprop.h>


#include <proptest.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

/////////////////////////////////////////////////////////////
// OBJECT SYSTEM LOOP CLIENT
////////////////////////////////////////////////////////////

static IObjectSystem* ObjSys;
static ILinkManagerInternal* LinkMan; 

//////////////////
// CONSTANTS
//
//

#define MY_GUID   LOOPID_ObjSys

typedef void Context;

// My state
typedef struct _StateRecord
{
   Context* context;
} StateRecord;


////////////////////////////////////////
// VERIFY LINK LOCK COUNT
//

static void verify_links()
{
#ifdef PLAYTEST

   long count = LinkMan->GetLockCount();
   AssertMsg1(count == 0,"Link Lock Count is %d\n",count);

#endif // PLAYTEST
}




////////////////////////////////////////
// DATABASE MESSAGE HANDLER
//

#define DB_TO_OBJNOTIF(x) ((x)|kObjNotifyDatabase) // DB and obj notify messages are coincidentally the same

static void db_message(DispatchData* msg)
{
#ifdef PLAYTEST
   static BOOL first_reset = TRUE; 
   if (!first_reset)
      verify_links();   
   else
      first_reset = FALSE; 
#endif 

   IObjectSystem_DatabaseNotify(ObjSys,DB_TO_OBJNOTIF(msg->subtype),msg->data);

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
      {
          ObjDeleteAllRefs();
      }
      break;

      case kDatabaseLoad:
      {
         if (msg->subtype & kDBMap) // only do this when wr_num_cells has changed
         {
            // I realize that this shouldn't really be here, but constraints
            // blew up when I tried to put it in physloop and put it after
            // WR for database messages.  Plus it's like 39 hours till E3 
            // right now. (cc)
            PhysRefSystemResize(); 
         }
      }
      break;

      case kDatabasePostLoad:
      {
          if (msg->subtype & kObjPartConcrete)
          {
             ObjBuildAllRefs();
          }
          ObjAnimLightReset();
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

   info.raw = hdata; 

   switch(msg)
   {
      case kMsgAppInit:
         ObjSys = AppGetObj(IObjectSystem);
         LinkMan = AppGetObj(ILinkManagerInternal); 
         PropertyInterfacesInit(); 
         ScalePropInit();
         ModelNamePropInit();
         ModelNumberPropInit();
         CollisionTypePropInit();
         ParticlePropInit();
         LightPropInit();
         AnimLightPropInit();
         ExtraLightPropInit();
         proptest_init();
         ObjPosInit(); 
         EOSInit();

         break;

      case kMsgAppTerm:
         SafeRelease(ObjSys);
         SafeRelease(LinkMan); 
         LightPropRelease();
         AnimLightPropRelease();
         ModelNamePropTerm();
         ModelNumberPropTerm();
         ObjPosTerm(); 
         EOSTerm();
         PropertyInterfacesTerm(); 
         break;

      case kMsgNormalFrame:
         ObjPosSynchronize(); 
         verify_links(); 
         break;

      case kMsgExitMode:
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
   
   return CreateSimpleLoopClient(_LoopFunc,state,&ObjSysLoopClientDesc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
// 


sLoopClientDesc ObjSysLoopClientDesc =
{
   &MY_GUID,
   "Object system client",
   kPriorityNormal,              
   kMsgEnd|kMsgDatabase|kMsgsFrame|kMsgsAppOuter|kMsgsMode,

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
      { kConstrainBefore, &LOOPID_SimFinish, kMsgsFrame}, 
      { kConstrainAfter,  &LOOPID_Physics,   kMsgsFrame}, 
      { kConstrainAfter,  &LOOPID_Wr,        kMsgDatabase}, 

      {kNullConstraint} // terminator
   }
};

