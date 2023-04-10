/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////
// Deep Cover METAGAME MENUS 
//

#include <dpcmenu.h>
#include <appagg.h>
#include <drkpanl.h>
#include <dpcoptmn.h>
#include <dpcsavui.h>
#include <dpcnetui.h>

#include <resapi.h>
#include <imgrstyp.h>
#include <schema.h>

#include <drawelem.h>
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

#include <netman.h>

#include <dpcmulti.h>
#include <dpcgame.h>
#include <dpcutils.h>
#include <playrobj.h>
#include <dpclding.h>
#include <dpcpgapi.h>


#include <netsynch.h>


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

// Avoid name conflicts
class cDPCMainMenu; 
typedef cDPCMainMenu cMainMenu; 

EXTERN void SwitchToDPCNewGameMode(BOOL push);


class cDPCMainMenu: public cMenuBase
{
   static sDarkPanelDesc gDesc; 

public:
   cMainMenu() : cMenuBase(&gDesc)
   {
   };

   cMainMenu(const sDarkPanelDesc* pdesc) : cMenuBase(pdesc)
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

      //SchemaPlay((Label *)"menubutt",NULL);

      switch (button)
      {
         case kNewGame:
            SwitchToDPCNewGameMode(TRUE); 
            break; 

         case kLoadGame:
            SwitchToDPCLoadGameMode(TRUE); 
            break; 

         case kOptions:
            SwitchToDPCOptionsMode(TRUE);
            break; 

         case kCredits:
            MoviePanel("credits.avi"); 
            break; 

         case kIntro:
            MoviePanel("cs1.avi"); 
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

//------------------------------------------------------------
// NET MAIN MENU 
//

class cDPCNetMainMenu: public cDPCMainMenu
{
   static sDarkPanelDesc gDesc; 
   typedef cDPCMainMenu cParent; 

public:
   cDPCNetMainMenu() : cParent(&gDesc)
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
      kMultiPlayer,

      kNumRects
   };

   
protected:

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      switch (button)
      {
         case kMultiPlayer:
            SwitchToDPCMultiplayUIMode(TRUE); 
            break; 

         default:
            cParent::OnButtonList(action,button); 
            break; 
      }
   }


   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
   {
      cParent::OnLoopMsg(msg,data);
      switch (msg)
      {
         case kMsgEnterMode:
         case kMsgResumeMode:
         {
            // Disconnect any net session we may be in
            AutoAppIPtr(NetManager);
            pNetManager->Leave(); 
            // If we had network synch disabled, make sure it's normal
            // again:
            NetSynchEnable();
         }
         break; 
      }
   }



}; 

//
// Main Menu descriptor 
//

static const char* netmain_button_names[] = 
{
   "new_game",
   "load_game",
   "options",
   "credits",
   "intro",
   "quit",
   "multiplayer", 
}; 

sDarkPanelDesc cDPCNetMainMenu::gDesc = 
{
   "netmain",
   cDPCNetMainMenu::kNumRects,
   cDPCNetMainMenu::kNumRects,
   cDPCNetMainMenu::kNumRects,
   netmain_button_names, 
   NULL, // font
   0,
   kMetaSndPanelMain,
}; 




static cMainMenu* gpMainMenu = NULL; 

void SwitchToDPCMainMenuMode(BOOL push) 
{
   if (gpMainMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpMainMenu->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeDPCMainMenuMode(void)
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

// Avoid name conflicts
class cDPCSimMenu; 
typedef cDPCSimMenu cSimMenu; 

class cDPCSimMenu: public cMenuBase
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
      kQuit,

      kNumRects
   };
   
protected:

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      //SchemaPlay((Label *)"menubutt",NULL);

      AutoAppIPtr(NetManager);

      switch (button)
      {
         case kContinue:
         {
            cAutoIPtr<IPanelMode> mode = GetPanelMode(); 
            mode->Exit(); 
         }
         break; 


         case kOptions:
            SwitchToDPCOptionsMode(TRUE);
            break; 

         case kSaveGame:
            if (pNetManager->IsNetworkGame())
            {
               char levelname[64];
               DPCGetLevelName(levelname,sizeof(levelname));
               // okay this is hack-a-riffic
               if ((stricmp(levelname,"station") == 0) || (stricmp(levelname,"earth") == 0))
                  break;
            }
            SwitchToDPCSaveGameMode(TRUE); 
            break; 

         case kLoadGame:
            SwitchToDPCLoadGameMode(TRUE); 
            break; 

         case kQuit:
         {
            if (gpMainMenu)
            {
               // Unwind to the main menu 
               cAutoIPtr<IPanelMode> panel = gpMainMenu->GetPanelMode(); 
               panel->Switch(kLoopModeUnwindTo);  
            }

            
         }
         break;

         default:
            Warning(("%s button is unimplemented\n",gDesc.string_names[button]));
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
   "quit",
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

void SwitchToDPCSimMenuMode(BOOL push) 
{
   if (gpSimMenu)
   {
      cAutoIPtr<IPanelMode> panel = gpSimMenu->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeDPCSimMenuMode(void)
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

#define MIN_DIFF 1 
extern int g_diff; 
#define DIFF2BUTT(x) (((x) < MIN_DIFF) ? MIN_DIFF - 1 : (x) - 1) 
#define BUTT2DIFF(x) ((x) + 1)

class cDPCNewGame: public cDarkPanel
{
   static sDarkPanelDesc gDesc; 

public:
   cDPCNewGame() 
      : cDarkPanel(&gDesc)
   {
      memset(&mDiffButtons,0,sizeof(mDiffButtons)); 
   };

   enum eRects
   {
      kStart, 
      kOptions,
      kCancel, 
      kNumButts, 

      kDifficulty = kNumButts, 
      kDiff0,
      kDiff1,
      kDiff2,
      kDiff3,

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
      g_diff = BUTT2DIFF(button);
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
         elem.draw_flags = INTERNAL(DRAWFLAG_INT_TRANSP); 
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

      LGadCreateButtonListDesc(&mDiffButtons,LGadCurrentRoot(),&desc); 

      //set the current difficulty setting
      if (g_diff < MIN_DIFF) g_diff = MIN_DIFF; 
      LGadRadioButtonSelect (&mDiffButtons, DIFF2BUTT(g_diff)); 
      int flags = LGadBoxFlags(&mDiffButtons); 
      LGadBoxSetFlags(&mDiffButtons, flags | BOXFLAG_CLEAR); 
      LGadBoxSetStyle(&mDiffButtons,&mDiffStyle); 
   }

   void TermUI()
   {
      g_diff = BUTT2DIFF(LGadRadioButtonSelection(&mDiffButtons)); 
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

   // Actually do the work of starting a game 

   void StartGame()
   {

      // Push the init game mode, and the initial cutscene
      // mode to the mode stack.  That way the cutscene happens first. 
      SwitchToDPCInitGame(FALSE); 
      MoviePanel("cs1.avi"); 

   }

   BOOL mGo; 

   void OnButtonList(ushort action, int button)
   {
      if (!(action & BUTTONGADG_LCLICK))
         return ;

      //SchemaPlay((Label *)"menubutt",NULL);

      switch (button)
      {
         case kStart:
         {
            mGo = TRUE; 
         }
         break; 

         case kOptions:
            SwitchToDPCOptionsMode(TRUE);
            break; 

         case kCancel:
         {
            cAutoIPtr<IPanelMode> mode = GetPanelMode(); 
            mode->Exit(); 
         }
         break; 
      }
   }



   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
   {
      switch(msg)
      {
         case kMsgEnterMode: 
            mGo = FALSE; 
            break; 
         case kMsgNormalFrame:
            if (mGo) StartGame(); 
            break; 
      }

      cDarkPanel::OnLoopMsg(msg,data); 
   }


}; 

//
// NewGame Menu descriptor 
//

static const char* newgame_button_names[] = 
{
   "start",
   "options",
   "cancel"
}; 

sDarkPanelDesc cDPCNewGame::gDesc = 
{
   "newgame",
   cDPCNewGame::kNumButts,
   cDPCNewGame::kNumRects,
   cDPCNewGame::kNumButts,
   newgame_button_names,
   NULL,
   0,
   kMetaSndPanelMain,
}; 


static cDPCNewGame* gpNewGame = NULL; 

void SwitchToDPCNewGameMode(BOOL push) 
{
   if (gpNewGame)
   {
      cAutoIPtr<IPanelMode> panel = gpNewGame->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeDPCNewGameMode(void)
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


void DPCMenusInit()
{
   if (config_is_defined("netplay_ui"))
      gpMainMenu = new cDPCNetMainMenu; 
   else
      gpMainMenu = new cMainMenu; 
   gpSimMenu = new cSimMenu; 
   gpNewGame = new cDPCNewGame; 

   init_commands(); 

   // default difficulty 
   g_diff = MIN_DIFF + 1; 

   // What the hell.  Initialize the loading screens too
   DPCLoadingInit(); 
   DPCMultiplayUIInit(); 
}

void DPCMenusTerm()
{
   delete gpMainMenu; 
   delete gpSimMenu; 
   delete gpNewGame;

   DPCLoadingTerm(); 
   DPCMultiplayUITerm(); 
}


//----------------------------------------
// COMMANDS
//

static void do_mainmenu()
{
   SwitchToDPCMainMenuMode(TRUE); 
}

static void do_simmenu()
{
   // don't do anything if the trigger is pulled
   AutoAppIPtr(PlayerGun);
   ObjID gun = pPlayerGun->Get();
   if ((gun != OBJ_NULL) && pPlayerGun->IsTriggerPulled())
      return;

   // throw any objects on the cursor into the world
   if (drag_obj != OBJ_NULL)
   {
      ThrowObj(drag_obj,PlayerObject());
      ClearCursor();
   }
   SwitchToDPCSimMenuMode(TRUE); 
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
