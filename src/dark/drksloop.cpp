/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drksloop.cpp,v 1.38 1999/12/22 18:45:25 PATMAC Exp $

#include <lg.h>
#include <loopapi.h>
#include <config.h>

#include <dispbase.h>
#include <loopmsg.h>
#include <dbasemsg.h>

#include <simloop.h>
#include <drkloop.h>
#include <drkwbow.h>
#include <drkwswd.h>
#include <physloop.h>
#include <biploop.h>
#include <objloop.h>
#include <uiloop.h>
#include <gamemode.h>
#include <plyrloop.h>

#include <headmove.h>
#include <drkgame.h>
#include <playrobj.h>
#include <simstate.h>
#include <simflags.h>
#include <simtime.h>
#include <prjctile.h>
#include <particle.h>
#include <picklock.h>

#include <drkplinv.h>
#include <drkinvui.h>

#include <drkreact.h>
#include <drksound.h>

#include <appagg.h>
#include <scrptapi.h>

#include <drkamap.h>

// Must be last header
#include <dbmem.h>

/////////////////////////////////////////////////////////////
// DARK SIMULATION LOOP CLIENT
////////////////////////////////////////////////////////////

//////////////////
// CONSTANTS
//
// These are just here to separate out boiler-plate code and leave it untouched
//

#define MY_FACTORY DarkSimLoopFactory
#define MY_GUID   LOOPID_DarkSim

// My context data
typedef void Context;

// My state
typedef struct _StateRecord
{
   Context* context; // a pointer to the context data I got.
   // State fields go here
   BOOL in_game_mode;
   BOOL is_first_game_frame;
} StateRecord;

////////////////////////////////////////
//
// LOOP/DISPATCH callback
// Here's where we do the dirty work.
//


static void db_message(DispatchData* msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   AutoAppIPtr(Inventory);
   pInventory->DatabaseNotify(msg->subtype,data.load); 

   DarkAutomapDatabaseNotify( msg->subtype, data.load ); 

   switch (DB_MSG(msg->subtype))
   {
      case kDatabasePostLoad:
      case kDatabaseDefault:
         InitProjectileArchetype();
         InitParticleArchetype();
         DarkReactionsPostLoad();
         break;
   }

}

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
         dark_init_game();
         break;

      case kMsgAppTerm:
         dark_term_game();
         break;

      case kMsgEnterMode:
      case kMsgResumeMode:
         state->in_game_mode = IsEqualGUID(*info.mode->to.pID, LOOPID_GameMode);
         if (state->in_game_mode)
         {  // center so mouse look around will work
            headmoveInit();
            dark_start_gamemode(msg == kMsgResumeMode);
            if ( msg == kMsgEnterMode ) {
               state->is_first_game_frame = TRUE;
            }
         }
         break;

      case kMsgExitMode:
      case kMsgSuspendMode:
         if (state->in_game_mode)
         {
            dark_end_gamemode(msg == kMsgSuspendMode);
            state->in_game_mode = FALSE; 
         }
         break;

      case kMsgNormalFrame:
         if (SimStateCheckFlags(kSimGameSpec))
         {
            dark_sim_update_frame();
            DrkSoundFrame();
            UpdateBowAttack(GetSimFrameTime());
            UpdateWeaponAttack();
            PickLockPerFrameCallback(GetSimFrameTime());
         }
         if ( state->in_game_mode && state->is_first_game_frame ) {
            state->is_first_game_frame = FALSE;
            DarkAutomapFirstFrameInit();
         }
         break;

      case kMsgDatabase:
         db_message(info.dispatch);
         break;

      case kMsgEnd:
         free(state);
         break;
   }
   return result;
}

////////////////////////////////////////////////////////////
//
// Loop client factory function.
//

#pragma off(unreferenced)
static ILoopClient* LGAPI _CreateClient(const sLoopClientDesc* desc, tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = (StateRecord*)malloc(sizeof(StateRecord));
   state->context = (Context*)data;
   state->is_first_game_frame = FALSE;

   return CreateSimpleLoopClient(_LoopFunc,state,desc);
}
#pragma on(unreferenced)

///////////////
// DESCRIPTOR
//

sLoopClientDesc DarkSimLoopClientDesc =
{
   &MY_GUID,                           // GUID
   "Dark Simulation",                    // NAME
   kPriorityNormal,                    // PRIORITY
   kMsgEnd | kMsgsMode | kMsgsFrameMid | kMsgsAppOuter | kMsgDatabase,   // INTERESTS

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,
   
   {
      {kConstrainAfter, &LOOPID_Physics, kMsgsFrame},
      {kConstrainBefore, &LOOPID_SimFinish, kMsgsFrame},
      {kConstrainAfter, &LOOPID_UI, kMsgsMode},
      {kConstrainAfter, &LOOPID_Game, kMsgsMode},
      {kConstrainAfter, &LOOPID_ObjSys, kMsgDatabase},
      {kConstrainAfter, &LOOPID_Biped, kMsgsAppOuter},
      {kConstrainBefore, &LOOPID_Biped, kMsgsFrame},
      {kConstrainAfter, &LOOPID_Player, kMsgsFrame},
      {kNullConstraint} // terminator
   }
};



