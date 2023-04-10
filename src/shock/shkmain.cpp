/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmain.cpp,v 1.2 1999/04/30 18:23:49 mahk Exp $

#include <loopapi.h>
#include <appagg.h>

#include <shkmenu.h>
#include <shkmain.h>
#include <panltool.h>
#include <config.h>
#include <cfgdbg.h>
#include <simbase.h>
#include <simstate.h>
#include <netsynch.h>
#include <simman.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <shkmniid.h>

//
// SHOCK "Master Control" Loopmode 
//

//------------------------------------------------------------
// "Master Control" Loop client 
//

static SimState gMasterSimState = { 0 }; 

class cMasterClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default>
{

public:
   static sLoopClientDesc gDesc; 

   STDMETHOD_(short,GetVersion)() { return kVerLoopClient; }; 
   STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return &gDesc; }; 

   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData);

protected:
   void OnFrame(); 

public:
   cMasterClient(); 
}; 

//------------------------------------------------------------
// Descriptor and client factory 
//

static ILoopClient* LGAPI client_factory(sLoopClientDesc * pDesc, tLoopClientData data);

sLoopClientDesc cMasterClient::gDesc = 
{
   &LOOPID_ShockMaster,
   "Shock Master Control Client", 
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
   return new cMasterClient; 
}

static ulong factory_id = -1; 

static void create_client_factory()
{
   if (factory_id == -1)
   {
      const sLoopClientDesc* descs[] = { &cMasterClient::gDesc, NULL}; 
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

STDMETHODIMP_(eLoopMessageResult) cMasterClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData )
{
   switch (msg)
   {

      case kMsgEnterMode:
      case kMsgResumeMode:
         SimStateSet(&gMasterSimState); 
         break; 
      case kMsgNormalFrame:
      case kMsgPauseFrame:
         OnFrame(); 
         break; 
   }
   return kLoopDispatchContinue;
};

////////////////////////////////////////////////////////////
// ACTUAL "MASTER CONTROL" FLOW
//

enum eStates
{
   kIntro,
   kMainMenu, 
   // kCutscene1,
   // kNewGame,
}; 

static int gModeState = kIntro; 

#define NEXT() if (TRUE) { gModeState++; break; } else 

cMasterClient::cMasterClient()
{
}
   

void cMasterClient::OnFrame()
{
   ConfigSpew("master_mode_spew",("Master mode frame, state = %d\n",gModeState));

   switch (gModeState)
   {
      case kIntro: 
      {
         const char* skip_intro = "skip_intro"; 
         if (config_is_defined("always_play_intro") 
             || !config_is_defined(skip_intro))
         {
            MoviePanel("intro.avi");   
            // set skip_intro for future runs 
            config_set_string(skip_intro,""); 
         }
      }
      NEXT(); 

      case kMainMenu:
      {
         SwitchToShockMainMenuMode(FALSE); 
      }
      NEXT(); 

   }
}

//------------------------------------------------------------
// MASTER CONTROL MODE
//
// A trivial mode with 1 client 
//

static tLoopClientID* Clients[] =
{
   &LOOPID_ShockMaster,
}; 

#define NUM_CLIENTS (sizeof(Clients)/sizeof(Clients[0]))

static void setup_loop_mode()
{
   AutoAppIPtr_(LoopManager,pLoopMan);
   
   sLoopModeDesc desc; 
   sLoopModeName name = { &LOOPMODE_ShockMaster, "Shock Master Control Mode" }; 
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
   &LOOPMODE_ShockMaster,
   0, 
   parm_list, 
}; 

////////////////////////////////////////

void ShockMasterModeInit()
{
   create_client_factory();
   setup_loop_mode(); 
}

void ShockMasterModeTerm()
{
   destroy_client_factory();
}

sLoopInstantiator* DescribeShockMasterMode()
{
   return &switch_me; 
}

static int state_map[] = { kIntro }; // , kCutscene1}; 
#define STATE_MAP_SIZE (sizeof(state_map)/sizeof(state_map[0]))

void ShockSetMasterMode(int state)
{
   Assert_(state >= 0 && state < STATE_MAP_SIZE); 
   gModeState = state_map[state]; 
}
