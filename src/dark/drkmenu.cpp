/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmenu.cpp,v 1.26 2000/02/19 13:09:08 toml Exp $
#include <drkmenu.h>
#include <appagg.h>
#include <drkpanl.h>
#include <drkmislp.h>
#include <drksavui.h>
#include <drkamap.h>
#include <drkdebrf.h>

#include <resapi.h>
#include <imgrstyp.h>

#include <gcompose.h>
#include <questapi.h>
#include <drkdiff.h>
#include <config.h>
#include <uigame.h>

#include <panltool.h>
#include <appapi.h>
#include <mprintf.h>
#include <campaign.h>
#include <metasnd.h>

#include <command.h>
#include <uiamov.h>
#include <uianim.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


EXTERN void SwitchToNewGameMode(BOOL push) ;
EXTERN void SwitchToOptionsMode(BOOL push) ;

//starting difficulty
extern int g_diff;


////////////////////////////////////////////////////////////
// DARK METAGAME MENUS 
//

//
// Base Menu class
//
// Some day, I expect this will
// deal with levers and other festoonery
//

class cMenuBase: public cDarkPanel, public cUiAnim
{
public: 
   cMenuBase(const sDarkPanelDesc* desc)
      : cDarkPanel(desc), cUiAnim(desc->panel_name,INTERFACE_PATH)
   {
   }; 

   ~cMenuBase() {}; 

protected:
   void InitUI()
   {
      cDarkPanel::InitUI(); 
      cUiAnim::InitUI();
   }

   void TermUI()
   {
      cUiAnim::TermUI();
      cDarkPanel::TermUI(); 
   }

   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
   {
      cUiAnim::OnLoopMsg(msg,data);
      cDarkPanel::OnLoopMsg(msg,data);
   }
}; 

//------------------------------------------------------------
// MAIN MENU 
//

class cMainMenu: public cMenuBase
{
   static sDarkPanelDesc gDesc; 

public:
   cMainMenu() : cMenuBase(&gDesc)
   {
   };

   enum eRects
   {
      kNewGame,
      kLoadGame,
      kOptions,
      kCredits,
      kIntro,
      kQuit,

      kNumRects
   };

   
protected:

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      switch (button)
      {
         case kNewGame:
            SwitchToNewGameMode(TRUE); 
            break; 

         case kLoadGame:
            SwitchToLoadGameMode(TRUE); 
            break; 

         case kOptions:
            SwitchToOptionsMode(TRUE);
            break; 

         case kCredits:
            PushMovieOrBookMode("credits");
            break; 

         case kIntro:
            PushMovieOrBookMode("intro");
            break; 

         case kQuit:
         {
            AutoAppIPtr_(Application,pApp);
            pApp->Quit(); 
            return; 
         }
         break;
            
      }
   }

   void OnEscapeKey()  // trap escape
   {
   }
}; 

//
// Main Menu descriptor 
//

static const char* main_button_names[] = 
{
   "new_game",
   "load_game",
   "options",
   "credits",
   "intro",
   "quit",
}; 

sDarkPanelDesc cMainMenu::gDesc = 
{
   "main",
   cMainMenu::kNumRects,
   cMainMenu::kNumRects,
   cMainMenu::kNumRects,
   main_button_names, 
   NULL, // font
   0,
   kMetaSndPanelMain,
}; 


static cMainMenu* gpMainMenu = NULL; 

void SwitchToMainMenuMode(BOOL push) 
{
   if (gpMainMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpMainMenu->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeMainMenuMode(void)
{
   if (gpMainMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpMainMenu->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}

//------------------------------------------------------------
// SIM MENU 
//

class cSimMenu: public cMenuBase
{
   static sDarkPanelDesc gDesc; 

public:
   cSimMenu() : cMenuBase(&gDesc)
   {
   };

   enum eRects
   {
      kContinue,
      kOptions,
      kSaveGame,
      kLoadGame,
      kMap,
      kObjectives,
      kQuit,
      kRestart,

      kNumRects
   };
   
protected:

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      switch (button)
      {
         case kContinue:
         {
            cAutoIPtr<IPanelMode> mode = GetPanelMode(); 
            mode->Exit(); 
         }
         break; 

         case kLoadGame:
            SwitchToLoadGameMode(TRUE); 
            break; 

         case kOptions:
            SwitchToOptionsMode(TRUE);
            break; 

         case kSaveGame:
            SwitchToSaveGameMode(TRUE); 
            break; 

         case kMap:
            SwitchToDarkAutomapMode(TRUE); 
            break; 

         case kObjectives:
            SwitchToObjectivesMode(TRUE); 
            break; 

         case kRestart:
         {
            MissionLoopReset(kMissLoopRestartMission); 
            UnwindToMissionLoop(); 
         }
         break; 

         case kQuit:
         {
            MissionLoopReset(kMissLoopMainMenu); 
            UnwindToMissionLoop(); 
         }
         break;
            
      }
   }
}; 

//
// Sim Menu descriptor 
//

static const char* sim_button_names[] = 
{
   "continue",
   "options",
   "save_game",
   "load_game",
   "map",
   "objectives",
   "quit",
   "restart",
}; 

sDarkPanelDesc cSimMenu::gDesc = 
{
   "sim",
   cSimMenu::kNumRects,
   cSimMenu::kNumRects,
   cSimMenu::kNumRects,
   sim_button_names, 
}; 


static cSimMenu* gpSimMenu = NULL; 

void SwitchToSimMenuMode(BOOL push) 
{
   if (gpSimMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpSimMenu->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeSimMenuMode(void)
{
   if (gpSimMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpSimMenu->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}

//------------------------------------------------------------
// NewGame MENU 
//

class cNewGame: public cDarkPanel
{
   static sDarkPanelDesc gDesc; 

public:
   cNewGame() 
      : cDarkPanel(&gDesc)
   {
   };

   enum eRects
   {
      kSkipTraining,
      kOptions,
      kCancel, 
      kNumButts, 

      kDifficulty = kNumButts, 
      kDiff0,
      kDiff1,
      kDiff2,

      kNumRects,

      kNumDiffs = kNumRects - kDiff0,
   };

protected:
   guiStyle mDiffStyle; 
   LGadButtonList mDiffButtons; 
   DrawElement mDiffElems[kNumDiffs];
   cStr mDiffStrs[kNumDiffs]; 

   static bool DiffCB (ushort action, int button, void *data, LGadBox *)
   {
      g_diff = button;
      return FALSE;
   }

   void InitUI()
   {
      cDarkPanel::InitUI(); 
      
      // default to current style colors
      AssertMsg(GetCurrentStyle(),"No current style for diff defaults"); 
      memset(&mDiffStyle,0,sizeof(mDiffStyle));
      memcpy(mDiffStyle.colors,GetCurrentStyle()->colors,sizeof(mDiffStyle.colors)); 
      uiGameLoadStyle("diff_",&mDiffStyle,mResPath); 

      // set up drawlelems for diff 
      for (int i = 0; i < kNumDiffs; i++)
      {
         DrawElement& elem = mDiffElems[i];
         memset(&elem,0,sizeof(elem)); 

         char buf[16];
         sprintf(buf,"diff_%d",i); 
         mDiffStrs[i] = FetchUIString(panel_name,buf,mResPath); 
         elem.draw_type = DRAWTYPE_TEXT; 
         elem.draw_data = (void*)(const char*)mDiffStrs[i]; 
      }

      LGadButtonListDesc desc = 
      { 
         kNumDiffs,
         &mRects[(int)kDiff0], 
         mDiffElems,
         DiffCB,
         0,
         BUTTONLIST_RADIO_FLAG,
      };

      memset( &mDiffButtons, 0, sizeof(LGadButtonList) );
      LGadCreateButtonListDesc(&mDiffButtons,LGadCurrentRoot(),&desc); 

      //set the current difficulty setting
      LGadRadioButtonSelect (&mDiffButtons, g_diff); 

      LGadBoxSetStyle(&mDiffButtons,&mDiffStyle); 
   }

   void TermUI()
   {
      g_diff = LGadRadioButtonSelection(&mDiffButtons); 
      LGadDestroyButtonList(&mDiffButtons); 
      memset(&mDiffElems,0,sizeof(mDiffElems)); 
      for (int i = 0; i < kNumDiffs; i++)
         mDiffStrs[i] = ""; 

      uiGameUnloadStyle(&mDiffStyle); 
      cDarkPanel::TermUI(); 
   }

   void RedrawDisplay()
   {

      // Draw the difficulty string.  This is a lot of code to draw a lousy string. 

      cStr str = FetchUIString(panel_name,"difficulty",mResPath);  // copy the string 
      char* s = (char*)(const char*)str;  // get a mutable pointer

      Rect& r = mRects[(int)kDifficulty]; 

      GUIcompose c; 
      GUIsetup(&c,&r,GUIcomposeFlags(ComposeFlagClear|ComposeFlagRead),GUI_CANV_ANY); 

      guiStyleSetupFont(&mDiffStyle,StyleFontTitle); 
      gr_set_fcolor(guiStyleGetColor(&mDiffStyle,StyleColorText)); 

      gr_font_string_wrap(gr_get_font(),s,RectWidth(&r)); 

      short w,h; 
      gr_string_size(s,&w,&h); 

      gr_string(s,(RectWidth(&r) - w)/2,(RectHeight(&r) - h)/2); 
      //      gr_string(s,gr_char_width('X'),(RectHeight(&r) - h)/2); 

      guiStyleCleanupFont(&mDiffStyle,StyleFontTitle) ;

      GUIdone(&c); 

      // 
      // Make sure the buttonlist gets drawn
      //
      region_expose(LGadBoxRegion(&mDiffButtons),LGadBoxRect(&mDiffButtons)); 
      
   }

   void StartGame()
   {
      // Reset the campaign
      AutoAppIPtr(Campaign); 
      pCampaign->New(); 

      // Set the difficulty
      g_diff = LGadRadioButtonSelection(&mDiffButtons); 
      AutoAppIPtr(QuestData); 
      pQuestData->Create (DIFF_QVAR, g_diff, kQuestDataCampaign);

      // skip Nexit missions...
      int mission = GetNextMission(); 
      
      if (config_get_int("starting_mission",&mission))
      {
         static int mission_map[32] = { 1,2,4,5,6,7,9,8,10,11,14,15,12,13,16 }; 
         int cnt = sizeof(mission_map)/sizeof(mission_map[0]); 

         if (!config_get_value("mission_map",CONFIG_INT_TYPE,(config_valptr)mission_map,&cnt))
            cnt = 16;  // @HACK for back-compatibility with thief 1.   
         
         if (mission > 0 && (mission-1) < cnt && mission_map[mission-1] != 0)
         {
            mission = mission_map[mission-1]; //lose the zero index
         }
         else
            mission = GetNextMission(); 
      }
      
      SetNextMission(mission); 

      // Start the mission loop 
      MissionLoopReset(kMissLoopStartLoop); 
      UnwindToMissionLoop(); 
   }

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      switch (button)
      {

         case kSkipTraining:
         {
            StartGame(); 
         }
         break; 

         case kOptions:
            SwitchToOptionsMode(TRUE);
            break; 

         case kCancel:
         {
            cAutoIPtr<IPanelMode> mode = GetPanelMode(); 
            mode->Exit(); 
         }
         break; 
      }
   }
}; 

//
// NewGame Menu descriptor 
//

static const char* newgame_button_names[] = 
{
   "skip_training",
   "options",
   "cancel"
}; 

sDarkPanelDesc cNewGame::gDesc = 
{
   "newgame",
   cNewGame::kNumButts,
   cNewGame::kNumRects,
   cNewGame::kNumButts,
   newgame_button_names,
   NULL,
   0,
   kMetaSndPanelMain,
}; 


static cNewGame* gpNewGame = NULL; 

void SwitchToNewGameMode(BOOL push) 
{
   if (gpNewGame)
   {
      cAutoIPtr<IPanelMode> panel = gpNewGame->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeNewGameMode(void)
{
   if (gpNewGame)
   {
      cAutoIPtr<IPanelMode> panel = gpNewGame->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}



//------------------------------------------------------------------
// INIT/TERM
//


void init_commands(); 

EXTERN void OptionsMenuInit ();
EXTERN void OptionsMenuTerm ();


void DarkMenusInit()
{
   gpMainMenu = new cMainMenu; 
   gpSimMenu = new cSimMenu; 
   gpNewGame = new cNewGame; 
   OptionsMenuInit ();

   init_commands(); 
}

void DarkMenusTerm()
{
   delete gpMainMenu; 
   delete gpSimMenu; 
   delete gpNewGame;
   OptionsMenuTerm ();
}


//----------------------------------------
// COMMANDS
//

static void do_mainmenu()
{
   SwitchToMainMenuMode(TRUE); 
}

static void do_simmenu()
{
   SwitchToSimMenuMode(TRUE); 
}



static Command commands[] = 
{
   { "main_menu", FUNC_VOID, do_mainmenu, "Go to main menu.", HK_ALL }, 
   { "sim_menu", FUNC_VOID, do_simmenu, "Go to sim menu.", HK_ALL }, 

}; 


static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}



