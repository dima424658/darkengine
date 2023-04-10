/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmislp.cpp,v 1.25 2000/01/27 14:34:41 adurant Exp $
#include <loopapi.h>
#include <appagg.h>
#include <drkmislp.h>

#include <metagame.h>
#include <dbfile.h>
#include <playrobj.h>
#include <drkdiff.h>
#include <simman.h>
#include <gamemode.h>
#include <drkldout.h>
#include <drkdebrf.h>
#include <scrnmode.h>
#include <dbasemsg.h>
#include <simstate.h>
#include <simbase.h>
#include <panltool.h>
#include <questapi.h>
#include <drkgoalt.h>
#include <drkloot.h>
#include <drkmenu.h>
#include <filevar.h>
#include <simtime.h>
#include <drksave.h>
#include <tagfile.h>
#include <drkbook.h>
#include <ctype.h>
#include <drkamap.h>

// For flags
#include <gamestr.h>
#include <sdesbase.h>
#include <sdestool.h>

#include <config.h>
#include <command.h>

// Include these last
#include <dbmem.h>
#include <initguid.h>
#include <drkmisid.h>

//------------------------------------------------------------
// MISSION LOOP CLIENT 
//

static SimState gMissionLoopSimState = { 0 }; 

class cMissionLoopClient : public cCTUnaggregated<ILoopClient, &IID_ILoopClient, kCTU_Default>
{

public:

   static sLoopClientDesc gDesc;


   cMissionLoopClient()
   {
   }

   ~cMissionLoopClient()
   {
   }


   STDMETHOD_(short,GetVersion)() { return kVerLoopClient; }; 
   STDMETHOD_(const sLoopClientDesc*, GetDescription)() { return &gDesc; }; 

   STDMETHOD_(eLoopMessageResult, ReceiveMessage)(eLoopMessage, tLoopMessageData hData);



   ulong LoadFlags(int mission); 
   ulong LoadNextMission(int mission); 
   const char* LoadEndMovie(int mission);

protected:
   void OnFrame(); 

};



//------------------------------------------------------------
// Descriptor and client factory 
//

static ILoopClient* LGAPI client_factory(sLoopClientDesc * pDesc, tLoopClientData data);

sLoopClientDesc cMissionLoopClient::gDesc = 
{
   &LOOPID_Mission,
   "Mission Loop Client", 
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
   return new cMissionLoopClient; 
}

static ulong factory_id = -1; 

static void create_client_factory()
{
   if (factory_id == -1)
   {
      const sLoopClientDesc* descs[] = { &cMissionLoopClient::gDesc, NULL}; 
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




STDMETHODIMP_(eLoopMessageResult) cMissionLoopClient::ReceiveMessage(eLoopMessage msg, tLoopMessageData )
{
   switch (msg)
   {

      case kMsgEnterMode:
      case kMsgResumeMode:
         SimStateSet(&gMissionLoopSimState); 
         break; 
      case kMsgNormalFrame:
      case kMsgPauseFrame:
         OnFrame(); 
         break; 
   }
   return kLoopDispatchContinue;
}

//------------------------------------------------------------
// STATE MACHINE DATA
//

// states for our state machine
// This enum no longer reflects ordering.  Its values get saved to disk,
// so add new members to the end! 
enum eMissionStates
{
   kStartApp, // initial state
   kIntro,     // intro cutscene
   kMainMenu,  // Main Menu 
   kBeginLoop, // begin mission loop
   kMetagame,  // mission selection menu, for now 
   kLoadFlags, // Load the mission flags 
   kBriefing,  // briefing movie
   kLoading,   // loading/objectives screen
   kPrepMission, // prep mission for difficulty & stuff 
   kLoadout,   // buy equipment
   kSim,       // run the sim.  
   kStopSim,       // stop the sim.  
   kSuccessMovie, // success movie
   kDebrief,   // debrief 
   kCutscene,  // Run victory/defeat/special cutscene
   kNextMission, // go to next mission 
   kSimMenu,     // go to sim menu
   kEndLoop, 
   kRestartMission, // special "restart mission" state
   kCheckFailure,   // Failure check
   
};

// 
// Mission=specific flags
//

enum eMissionFlags
{
   kSkipMission   = 1 << 0,  // skip this mission entirely
   kSkipBriefing  = 1 << 1,  // No briefing movie
   kSkipLoadout   = 1 << 2,  // No loadout 
   kPlayCutscene  = 1 << 3,  // play a special cutscene 
   kEndGame       = 1 << 4,  // Last mission of the game 
}; 

//
// Standard Loop Order 
// This array governs the order in which the states occur normally. 
//

static int state_sequence[]  = 
{
   kStartApp, // initial state
   kIntro,     // intro cutscene
   kMainMenu,  // Main Menu 
   kBeginLoop, // begin mission loop
   kMetagame,  // mission selection menu, for now 
   kLoadFlags, // Load the mission flags 
   kBriefing,  // briefing movie
   kLoading,   // loading/objectives screen
   kPrepMission, // prep mission for difficulty & stuff 
   kLoadout,   // buy equipment
   kSim,       // start & run the sim.  
   kStopSim,       // stop the sim.  
   kSuccessMovie, // success movie
   kDebrief,   // debrief 
   kCheckFailure, 
   kCutscene,  // Run victory/defeat/special cutscene
   kNextMission, // go to next mission 
   kSimMenu,     // go to sim menu 
   kEndLoop, 
}; 

#define STATE_SEQUENCE_LEN (sizeof(state_sequence)/sizeof(state_sequence[0]))

//
// Find the next state in the sequence
//

static int get_next_state(int state)
{  
   for (int i = 0; i < STATE_SEQUENCE_LEN - 1; i++)
      if (state == state_sequence[i])
         return state_sequence[i+1];
   
   CriticalMsg1("No next mission loop state for %d",state); 
   return kStartApp;
}

//------------------------------------------------------------
// STATE MACHINE STATE
//

struct sMissLoopState
{
   int mission;  // which mission are we on
   ulong flags;  // mission-specific flags
   int state;    // which state are we in
   int next_state; // which state are we going to 
   int next_mission;  // Which mission is next 

   void Next()
   {
      if (next_mission)
         mission = next_mission; 
      else
         mission++; 
   }
}; 

static sMissLoopState def_state = { 1, 0, kStartApp, kStartApp, }; 

//
// File var descriptor 
//
sFileVarDesc gMissLoopStateDesc =
{
   kCampaignVar,
   "MISSLOOP",
   "Mission Loop State",
   FILEVAR_TYPE(sMissLoopState),
   { 1, 0 },
   { 1, 0},
   "dark",
};

// The global machine state
class cMissLoopState : public cFileVar<sMissLoopState,&gMissLoopStateDesc>
{
   void Reset()
   {
      *(sMissLoopState*)this = def_state; 
   }

   void Update()
   {
      // On edit or load, we don't want to go to the next state
      next_state = state; 
   }
} gMissLoop; 


//------------------------------------------------------------
// STATE MACHINE LOGIC 
//


void PushCutsceneMovieMode(int mission); 
void backup_game(); 
void restore_game(); 

#define STATE(x) case x
#define GOTO(x) if (1) { gMissLoop.next_state = x; break; } else 
#define NEXT() if (1) { gMissLoop.next_state = get_next_state(gMissLoop.state); break; } else 

void cMissionLoopClient::OnFrame()
{
   // Transition to the next state
   gMissLoop.state = gMissLoop.next_state; 

   switch (gMissLoop.state)
   {

      STATE(kIntro):
         {
            if (!config_is_defined("skip_intro"))
            {
               PushMovieOrBookMode("intro"); 
               if (!config_is_defined("always_play_intro"))
                  config_set_string("skip_intro",""); 
            }
         }
      NEXT(); 

      STATE(kMainMenu):
         {
            SwitchToMainMenuMode(TRUE); 
         }
      NEXT(); 

      STATE(kBeginLoop):
         {
            // Stop the sim and reset the db, just in case we haven't.
            AutoAppIPtr_(SimManager,pSimMan); 
            pSimMan->StopSim(); 
            dbReset(); 
            backup_game(); 
         }
      NEXT(); 

      STATE(kMetagame): 
#ifdef PLAYTEST
         if (config_is_defined("select_missions"))
         {
            SwitchToMetaGameMode(TRUE); 
         }
#endif 
      NEXT(); 

      STATE(kLoadFlags):
         {
            gMissLoop.flags = LoadFlags(gMissLoop.mission); 
            gMissLoop.next_mission = LoadNextMission(gMissLoop.mission); 
            if (gMissLoop.flags & kSkipMission)
            {
               gMissLoop.Next(); 
               GOTO(kLoadFlags); 
            }
         }
      NEXT(); 

      STATE(kBriefing): 
         if (!(gMissLoop.flags & kSkipBriefing))
         {
            PushBriefingMovieMode(gMissLoop.mission); 
         }
      NEXT(); 

      
      STATE(kLoading):
         {
            // Go to the loading screen 
            SwitchToLoadingMode(TRUE); 
         }
      NEXT(); 

      STATE(kPrepMission): 
         {
            AutoAppIPtr(QuestData);    
            pQuestData->Delete(MISSION_COMPLETE_VAR); 
            DarkPrepLevelForDifficulty(); 
            PlayerCreate(); 
            DarkLootPrepMission(); 
         }
      NEXT(); 

      STATE(kLoadout):
         TransferMapInfo(); //update map.
         if (!(gMissLoop.flags & kSkipLoadout))
         {
            SwitchToLoadoutMode(TRUE); 
         }
      NEXT(); 



      STATE(kSim):
         {
            AutoAppIPtr_(SimManager,pSimMan); 
            pSimMan->StartSim(); 

            GameModeDesc desc = { NULL, };

            AutoAppIPtr(Loop);
            sLoopInstantiator* gamemode = DescribeGameMode(mmGameDefault,&desc); 
            pLoop->ChangeMode(kLoopModePush,gamemode);
         }
      NEXT(); 

      STATE(kStopSim):
         {
            AutoAppIPtr_(SimManager,pSimMan); 
            pSimMan->StopSim(); 
         }
      NEXT(); 

      STATE(kSuccessMovie):
         {
            AutoAppIPtr(QuestData); 
            if (pQuestData->Get(MISSION_COMPLETE_VAR))
	       PushMovieOrBookMode(LoadEndMovie(gMissLoop.mission)); 
            else
               PushMovieOrBookMode("death"); 
         }
      NEXT(); 

      STATE(kDebrief):
         {
            SwitchToDebriefMode(TRUE); 
         }
      NEXT(); 

      STATE(kCheckFailure):
         {
            AutoAppIPtr(QuestData); 
            if (!pQuestData->Get(MISSION_COMPLETE_VAR))
               GOTO(kRestartMission); 
         }
      NEXT(); 



      STATE(kCutscene):
         {
            if (gMissLoop.flags & kPlayCutscene)
               PushCutsceneMovieMode(gMissLoop.mission); 
         }
      NEXT(); 


      STATE(kNextMission):
         {
            BOOL endgame = (gMissLoop.flags & kEndGame) != 0;
#ifdef THIEFTWODEMO
            // Prohibit demo users from ending any mission
            //if (gMissLoop.mission >= 2)
               endgame = TRUE; 
#endif 

            if (endgame)
            {
               gMissLoop.mission = 1; 
#ifdef THIEFTWODEMO
	       gMissLoop.mission = 11;
#endif
               GOTO(kMainMenu);   
            }

            gMissLoop.Next(); 

            // so that save game titles are correct
            // @TODO: actually reset the whole database here? 
            SetSimTime(0); 
         }
      NEXT(); 

      STATE(kSimMenu):  
         {
            // Go to the sim menu, give the player a chance to save & stuff
            // Actually, disabling this until we can disable the map/objective
            // buttons 
            //            SwitchToSimMenuMode(TRUE); 
         }
      NEXT(); 

      STATE(kEndLoop):
         GOTO(kBeginLoop); 

      STATE(kRestartMission):
         {
            restore_game(); 
         }
      GOTO(kBeginLoop); 
      
      default:
         NEXT(); 

 
   }
}

//------------------------------------------------------------
// BACKUP/RESTORE GAME
//

static void backup_game()
{
   DarkSaveCheckpoint(); 
}

static void restore_game()
{
   AutoAppIPtr(QuestData); 
   // preserve difficulty 
   int diff = pQuestData->Get(DIFF_QVAR); 
   DarkRestoreCheckpoint(); 
   pQuestData->Set(DIFF_QVAR,diff); 
}

//------------------------------------------------------------
// Load the mission flags
//

static const char* flag_names[] = 
{ 
   "skip", 
   "no_briefing",
   "no_loadout",
   "cutscene", 
   "end",
};

#define NUM_FLAGS (sizeof(flag_names)/sizeof(flag_names[0]))


// Just an fdesc, no sdesc 
static sFieldDesc flag_fdesc = { "flags", kFieldTypeBits, sizeof(ulong), 0, kFieldFlagNone, 0, NUM_FLAGS, NUM_FLAGS, flag_names }; 

ulong cMissionLoopClient::LoadFlags(int mission)
{
   char buf[64]; 
   sprintf(buf,"miss_%d",mission); 

   AutoAppIPtr(GameStrings); 
   cStr flagstr = pGameStrings->FetchString("missflag",buf); 
   
   AutoAppIPtr_(StructDescTools,pTools); 
   ulong flags = 0; 
   pTools->ParseField(&flag_fdesc,(const char*)flagstr,&flags); 


   return flags; 
}

ulong cMissionLoopClient::LoadNextMission(int mission)
{
   char buf[64]; 
   sprintf(buf,"miss_%d_next",mission);

   AutoAppIPtr(GameStrings); 

   // Look for next mission 
   cStr nextstr = pGameStrings->FetchString("missflag",buf); 
   int next = 0; 
   sscanf((const char*)nextstr,"%d",&next);
   
#ifdef THIEFTWODEMO
   return 11;
#endif

   return (next) ? next : mission+1; 
}

const char* cMissionLoopClient::LoadEndMovie(int mission)
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

//------------------------------------------------------------
// MISSION LOOP MODE
//
// A trivial mode with 1 client 
//


static tLoopClientID* Clients[] =
{
   &LOOPID_Mission,
}; 

#define NUM_CLIENTS (sizeof(Clients)/sizeof(Clients[0]))

static void setup_loop_mode()
{
   AutoAppIPtr_(LoopManager,pLoopMan);
   
   sLoopModeDesc desc; 
   sLoopModeName name = { &LOOPMODE_Mission, "Mission Loop Mode" }; 
   desc.name = name; 
   desc.nClients = NUM_CLIENTS; 

   desc.ppClientIDs = Clients; 
   pLoopMan->AddMode(&desc); 
}


////////////////////////////////////////////////////////////
// API
//
void setup_commands(); 

void MissionLoopInit()
{
   setup_commands(); 
   create_client_factory(); 
   setup_loop_mode(); 
}

void MissionLoopTerm()
{
   destroy_client_factory(); 
}

int GetNextMission()
{
#ifdef THIEFTWODEMO
   return 11;
#endif
   return gMissLoop.mission; 
}

void SetNextMission(int m)
{
   gMissLoop.mission = m; 
#ifdef THIEFTWODEMO
   gMissLoop.mission = 11;
#endif
}

static sLoopModeInitParm parm_list[] = 
{
   { NULL} // terminator
};

static sLoopInstantiator switch_me = 
{
   &LOOPMODE_Mission, 
   0, 
   parm_list, 
}; 


sLoopInstantiator* DescribeMissionLoopMode()
{
   return &switch_me; 
}

void UnwindToMissionLoop()
{
   AutoAppIPtr(Loop); 
   pLoop->ChangeMode(kLoopModeUnwindTo,&switch_me); 
}

void MissionLoopReset(int to_where)
{
   static int reset_vec[] = { kMainMenu, kBeginLoop, kRestartMission}; 
   Assert_(to_where >= 0 && to_where < sizeof(reset_vec)/sizeof(reset_vec[0])); 

   // Stop the sim, just in case we haven't.
   AutoAppIPtr_(SimManager,pSimMan); 
   pSimMan->StopSim(); 

   gMissLoop.next_state = reset_vec[to_where]; 
}

void PushBriefingMovieMode(int mission)
{
   char buf[16];  
   sprintf(buf,"B%02d",mission); 
   PushMovieOrBookMode(buf); 
}


void PushMovieOrBookMode(const char* name)
{
   char buf[16]; 
   sprintf(buf,"%s.avi",name); 
   // If we don't find the movie, show a book instead
   if (!MoviePanel(buf))  
   {
      //if we only allow movies, then better to crash than try to play
      //a book that doesn't exist.
      if (config_is_defined("only_movies"))
	return;
      // look up which art to use in cfg 
      char art[16]; 
      char var[32]; 
      sprintf(var,"%s_text_bg",name);
      if (!config_get_raw(var,art,sizeof(art)))
          config_get_raw("default_text_bg",art,sizeof(art));

      // strip trailing whitespace
      for (char* t = art+strlen(art)-1; t >= art && isspace(*t); t--)
         *t = '\0'; 

      if (!TestBookExists(name,art))
	return;

      SwitchToDarkBookMode(TRUE, name,art); 
   }
}

void PushCutsceneMovieMode(int mission)
{
   char buf[16];  
   sprintf(buf,"CS%02d",mission); 
   PushMovieOrBookMode(buf); 
}

BOOL CanChangeDifficultyNow(void)
{
   if (gMissLoop.state >= kPrepMission && gMissLoop.state <= kStopSim)
      return FALSE; 
   return TRUE; 
}



static Command commands[] = 
{
   { "mission_loop", FUNC_VOID, UnwindToMissionLoop, "Return to (or start) the mission loop" }, 
}; 

static void setup_commands()
{
   COMMANDS(commands,HK_ALL); 
}


