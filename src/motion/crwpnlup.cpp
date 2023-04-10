/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// Creature Weapon loop client, including save/load
//

#include <mprintf.h>
#include <config.h>
#include <appagg.h>
#include <loopmsg.h>
#include <loopapi.h>  // for CreateSimpleLoopClient
#include <netloop.h>
#include <dbasemsg.h>
#include <dispatch.h>
#include <dispbase.h>
#include <objremap.h>
#include <vernum.h>
#include <objedit.h>  // for ObjEditName()
#include <iobjsys.h>
#include <physloop.h> // for LOOPID_Physics
#include <simtime.h>  // for GetSimTimeFrame()
#include <netman.h>   // INetMan

#include <crwpnapi.h>
#include <crwpnlst.h>

#include <initguid.h>
#include <crwpnlup.h>

#include <dbmem.h>

//////////
//
// LOOP CLIENT
//

// State record for use by the ghost loop client
// Currently unused, but since it was in the boilerplate, I decided to
// keep it in case we want it later:
typedef struct _StateRecord
{
   int dummy;  // placeholder
} StateRecord;

static void CreatureWeapon_db_message(DispatchData *msg)
{
   msgDatabaseData data;
   data.raw = msg->data;

   switch (DB_MSG(msg->subtype))
   {
      case kDatabaseReset:
      {
         CreatureWeaponListTerm();
         break;
      }
      
      case kDatabaseLoad:
      case kDatabasePostLoad:
      case kDatabaseSave:
      {
         break;
      }
   }
}

static eLoopMessageResult LGAPI _LoopFunc(void* data, 
                                          eLoopMessage msg, 
                                          tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;
   info.raw = hdata;

   switch(msg)
   {
      case kMsgAppInit:
      {
         CreatureWeaponInit();
         break;
      }
      case kMsgAppTerm:
      {
         CreatureWeaponTerm();
         break;
      }
      case kMsgNormalFrame:
      {
         break;
      }
      case kMsgDatabase:
      {
         CreatureWeapon_db_message(info.dispatch);
         break;
      }
      case kMsgEnd:
      {
         delete state;
         break;   
      }
   }
   return result;
}

// Factory function for our loop client.
static ILoopClient* LGAPI _CreateClient(sLoopClientDesc * pDesc,
                                        tLoopClientData data)
{
   // allocate space for our state, and fill out the fields
   StateRecord* state = new StateRecord;

   return CreateSimpleLoopClient(_LoopFunc,state,&CreatureWeaponLoopClientDesc);
}

// The public loop client descriptor
// used by loopapp.c
sLoopClientDesc CreatureWeaponLoopClientDesc =
{
   &LOOPID_CreatureWeapon,
   "CreatureWeapon Client",
   kPriorityNormal,              
   kMsgDatabase | kMsgsMode | kMsgsAppOuter | kMsgsFrameMid | kMsgEnd, // interests 

   kLCF_Callback,
   _CreateClient,
   
   NO_LC_DATA,

   {
//      {kConstrainAfter,  &LOOPID_Physics, kMsgsAppOuter },
//      {kConstrainBefore, &LOOPID_Physics, kMsgsFrame },
//      {kConstrainAfter,  &LOOPID_Network, kMsgsFrame },
      {kNullConstraint}
   }
};
