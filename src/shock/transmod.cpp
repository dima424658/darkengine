/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/transmod.cpp,v 1.6 1999/02/08 14:56:31 XEMU Exp $
#include <loopapi.h>
#include <appagg.h>

#include <simstate.h>
#include <simbase.h>

#include <config.h>
#include <command.h>

#include <shkmulti.h>
#include <gamemode.h>
#include <transmod.h>

#include <netmsg.h>
#include <netsynch.h>
#include <iobjsys.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <transmid.h>

// brutal hack data
char gTransModeLevel[255];
int gTransModeMarker;
int gTransModeFlags;
int gTransModeSlot;
BOOL gTransSaveHack = FALSE;

eTransAction gTransModeAction;

// The network message to tell other players to switch at the same time
static cNetMsg *gpLevelTransMsg = NULL;

//------------------------------------------------------------
// LEVEL TRANSITION LOOP CLIENT 
//

static SimState gLevelTransSimState = { 0 }; 

class cLevelTransClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default>
{

public:

   static sLoopClientDesc gDesc;


   cLevelTransClient()
   {
   }

   ~cLevelTransClient()
   {
   }


   STDMETHOD_(short,GetVersion)() { return kVerLoopClient; }; 
   STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return &gDesc; }; 

   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData);


protected:
   void OnFrame(); 

};



//------------------------------------------------------------
// Descriptor and client factory 
//

static ILoopClient* LGAPI client_factory(sLoopClientDesc * pDesc, tLoopClientData data);

sLoopClientDesc cLevelTransClient::gDesc = 
{
   &LOOPID_LevelTrans,
   "Level Transition Client", 
   kPriorityNormal, 
   kMsgEnd | kMsgNormalFrame| kMsgsMode,

   kLCF_Callback,
   client_factory,
   
   NO_LC_DATA,
   
   {
      { kNullConstraint },
   }
}; 

static ILoopClient* LGAPI client_factory(sLoopClientDesc * pDesc, tLoopClientData data)
{
   return new cLevelTransClient; 
}

static ulong factory_id = -1; 

static void create_client_factory()
{
   if (factory_id == -1)
   {
      const sLoopClientDesc* descs[] = { &cLevelTransClient::gDesc, NULL}; 
      ILoopClientFactory* fact = CreateLoopFactory(descs);

      AutoAppIPtr_(LoopManager,pLoopMan); 
      pLoopMan->AddClientFactory(fact,&factory_id); 
      SafeRelease(fact); 
   }
}

static void destroy_client_factory()
{
   if (factory_id != -1)
   {
      AutoAppIPtr_(LoopManager,pLoopMan);
      pLoopMan->RemoveClientFactory(factory_id); 
   }

}




STDMETHODIMP_(eLoopMessageResult) cLevelTransClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData )
{
   switch (msg)
   {

      case kMsgEnterMode:
      case kMsgResumeMode:
         SimStateSet(&gLevelTransSimState); 
         break; 
      case kMsgNormalFrame:
      case kMsgPauseFrame:
         OnFrame(); 
         break; 
   }
   return kLoopDispatchContinue;
}


void cLevelTransClient::OnFrame()
{
   // Do your shit here Xemu
   switch (gTransModeAction)
   {
   case kTransActionSwitchLevel:
      ShockLevelTransport(gTransModeLevel, gTransModeMarker, gTransModeFlags);
      //ShockSaveGame(-1);
      gTransSaveHack = TRUE;
      break;
   case kTransActionLoadGame:
      ShockLoadGame(gTransModeSlot);
      break;
   }

   // Go to synch mode, which will go to game mode
   SwitchToNetSynchMode(FALSE);
}


//------------------------------------------------------------
// LEVEL TRANS MODE
//
// A trivial mode with 1 client 
//

static tLoopClientID* Clients[] =
{
   &LOOPID_LevelTrans,
}; 

#define NUM_CLIENTS (sizeof(Clients)/sizeof(Clients[0]))

static void setup_loop_mode()
{
   AutoAppIPtr_(LoopManager,pLoopMan);
   
   sLoopModeDesc desc; 
   sLoopModeName name = { &LOOPMODE_LevelTrans, "Level Trans Mode" }; 
   desc.name = name; 
   desc.nClients = NUM_CLIENTS; 

   desc.ppClientIDs = Clients; 
   pLoopMan->AddMode(&desc); 
}

static sLoopModeInitParm parm_list[] = 
{
   { NULL} // terminator
};

static sLoopInstantiator switch_me = 
{
   &LOOPMODE_LevelTrans, 
   0, 
   parm_list, 
}; 

// Does the actual switch; we break this out so it can be called by
// the network code
static void doSwitchToLevelTrans(const char *level, int marker, int flags)
{
   strncpy(gTransModeLevel, level, sizeof(gTransModeLevel));
   gTransModeMarker = marker;
   gTransModeFlags = flags;
   gTransModeAction = kTransActionSwitchLevel;

   AutoAppIPtr(Loop); 
   pLoop->ChangeMode(kLoopModeSwitch,&switch_me); 
}

static void doSwitchToLoadGame(int slot)
{
   gTransModeSlot = slot;
   gTransModeAction = kTransActionLoadGame;

   AutoAppIPtr(Loop); 
   pLoop->ChangeMode(kLoopModeSwitch,&switch_me); 
}

//////////
//
// Network code
//

// This will get called on all the non-initiating machines, to kick
// them into the transition as well.
static void handleShockLevelTransport(const char *newfile,
                                      int marker,
                                      uint flags)
{
   // Yes, this seems twisted and weird, but it makes sense. Network
   // messages are normally processed with the database locked, because
   // too many subsystems (especially frob handlers) assume that it
   // is locked. But we absolutely do *not* want it locked while
   // we're loading the game; that causes any deletes that happen
   // due to resets to happen instead after we unlock, which is
   // plainly broken...
   AutoAppIPtr(ObjectSystem);
   pObjectSystem->Unlock();
   doSwitchToLevelTrans(newfile, marker, flags);
   pObjectSystem->Lock();
}

// The descriptor for the network level-transition message:
sNetMsgDesc sLevelTransDesc = {
   kNMF_Broadcast,
   "LevelTrans",
   "LevelTrans",
   NULL,
   handleShockLevelTransport,
   {{kNMPT_String},
    {kNMPT_Int},
    {kNMPT_UInt},
    {kNMPT_End}}
};

////////////////////////////////////////////////////////////
// API
//

// call these at start/finish
void LevelTransInit()
{
   create_client_factory(); 
   setup_loop_mode(); 
   gpLevelTransMsg = new cNetMsg(&sLevelTransDesc);
}

////////////////////////////////////////////////////////////
void LevelTransTerm()
{
   destroy_client_factory(); 
   delete gpLevelTransMsg;
}

////////////////////////////////////////////////////////////
sLoopInstantiator* DescribeLevelTransMode()
{
   return &switch_me; 
}

////////////////////////////////////////////////////////////
// Call this to switch levels using this mode
void TransModeSwitchLevel(char *level, int marker, int flags)
{
   gpLevelTransMsg->Send(OBJ_NULL, level, marker, flags);
   doSwitchToLevelTrans(level, marker, flags);
}


////////////////////////////////////////////////////////////
// Call this to switch levels using this mode
// This probably need to become net.savvy as well, maybe?
void TransModeLoadGame(int slot)
{
   doSwitchToLoadGame(slot);   
}
