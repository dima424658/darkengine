/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
// DEEP COVER "Master Control" Loopmode 
//

#include <loopapi.h>
#include <appagg.h>

#include <dpcmenu.h>
#include <dpcmain.h>
#include <panltool.h>
#include <config.h>
#include <cfgdbg.h>
#include <simbase.h>
#include <simstate.h>
#include <netsynch.h>
#include <simman.h>
#include <questapi.h>
#include <dpcgoalt.h>
#include <gamestr.h>

// Include these last
#include <initguid.h>
#include <dpcmniid.h>

#include <dbmem.h>

void PushMovieOrBookMode(const char* name)
{
   char buf[16];
   sprintf(buf,"%s.avi",name);
   MoviePanel(buf);
#if 0
   // If we don't find the movie, show a book instead
   if (!MoviePanel(buf))
   {
      // look up which art to use in cfg
      char art[16];
      char var[32];
      sprintf(var,"%s_text_bg",name);
      if (!config_get_raw(var,art,sizeof(art)))
          config_get_raw("default_text_bg",art,sizeof(art));

      // strip trailing whitespace
      for (char* t = art+strlen(art)-1; t >= art && isspace(*t); t--)
         *t = '\0';

      SwitchToDarkBookMode(TRUE, name,art);
   }
#endif
}

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

   const char* LoadEndMovie(int mission);

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
   &LOOPID_DPCMaster,
   "DPC Master Control Client", 
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

const char* cMasterClient::LoadEndMovie(int mission)
{
  char buf[64];
  char endmovie[64];
  int  gotnewmovie = 0;
  sprintf(buf,"miss_%d_endmovie",mission);

  AutoAppIPtr(GameStrings);

  // Look for end movie
  cStr nextstr = pGameStrings->FetchString("missflag",buf);
  gotnewmovie = sscanf((const char*)nextstr,"%s",&endmovie);

  return (gotnewmovie > 0) ? endmovie : "success";
}

////////////////////////////////////////////////////////////
// ACTUAL "MASTER CONTROL" FLOW
//

enum eStates
{
   kIntro,
   kMainMenu, 
   kSuccessMovie,
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
         SwitchToDPCMainMenuMode(FALSE); 
      }
      NEXT(); 

      case kSuccessMovie:
         {
            AutoAppIPtr(QuestData);
            if (pQuestData->Get(MISSION_COMPLETE_VAR))
               // TBD:   correct mission number thief-like
               PushMovieOrBookMode(LoadEndMovie(0));
            else
               PushMovieOrBookMode("death");

            gModeState = kMainMenu;
            break;
         }


   }
}

//------------------------------------------------------------
// MASTER CONTROL MODE
//
// A trivial mode with 1 client 
//

static tLoopClientID* Clients[] =
{
   &LOOPID_DPCMaster,
}; 

#define NUM_CLIENTS (sizeof(Clients)/sizeof(Clients[0]))

static void setup_loop_mode()
{
   AutoAppIPtr_(LoopManager,pLoopMan);
   
   sLoopModeDesc desc; 
   sLoopModeName name = { &LOOPMODE_DPCMaster, "DPC Master Control Mode" }; 
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
   &LOOPMODE_DPCMaster,
   0, 
   parm_list, 
}; 

////////////////////////////////////////

void UnwindToMissionLoop()
{
   AutoAppIPtr(Loop);
   pLoop->ChangeMode(kLoopModeUnwindTo, &switch_me);
}

void DPCMasterModeInit()
{
   create_client_factory();
   setup_loop_mode(); 
}

void DPCMasterModeTerm()
{
   destroy_client_factory();
}

sLoopInstantiator* DescribeDPCMasterMode()
{
   return &switch_me; 
}

static int state_map[] = { kIntro }; // , kCutscene1}; 
#define STATE_MAP_SIZE (sizeof(state_map)/sizeof(state_map[0]))

void DPCSetMasterMode(int state)
{
   Assert_(state >= 0 && state < STATE_MAP_SIZE); 
   gModeState = state_map[state]; 
}
