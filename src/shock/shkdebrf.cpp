/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkdebrf.cpp,v 1.11 2000/02/19 13:20:11 toml Exp $
#include <shkdebrf.h>
#include <appagg.h>
#include <drkpanl.h>
#include <shkoptmn.h>
#include <shksavui.h>

#include <resapi.h>
#include <imgrstyp.h>

#include <gcompose.h>
#include <config.h>
#include <uigame.h>

#include <shkutils.h>

#include <panltool.h>
#include <appapi.h>
#include <mprintf.h>
#include <metasnd.h>

#include <command.h>
#include <uiamov.h>
#include <uianim.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// SHOCK DEBRIEF SCREEN
//

//
// Base Menu class
//

extern grs_font *gShockFontBlue;

static Rect text_rect = {{216,92},{216 + 336,92 + 294}};
static Rect mission_rect = {{6,6},{6 + 200,6 + 75}};
static Rect logo_rect = {{212,4},{212 + 353, 4 + 80}};
static Rect garethart_rect = {{4, 320}, { 4 + 204, 320 + 156 }};

IRes *gLogoHnds[3];

class cDebriefPanel: public cDarkPanel, public cUiAnim
{
public: 
   static sDarkPanelDesc gDesc; 
   int m_mission;

   cDebriefPanel()
      : cDarkPanel(&gDesc), cUiAnim(gDesc.panel_name,INTERFACE_PATH)
   {
   }; 

   ~cDebriefPanel() {}; 

   enum eRects
   {
      kContinue,

      kNumRects
   };

protected:
   void InitUI()
   {
      cDarkPanel::InitUI(); 
      cUiAnim::InitUI();

      int i;
      for (i=0; i < 3; i++)
      {
         char temp[255];
         sprintf(temp,"cgtitle%d",i+1);
         gLogoHnds[i] = LoadPCX(temp);
      }
   }

   void TermUI()
   {
      cUiAnim::TermUI();
      cDarkPanel::TermUI(); 

      int i;
      for (i=0; i < 3; i++)
      {
         SafeFreeHnd(&gLogoHnds[i]);
      }
   }

   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
   {
      cUiAnim::OnLoopMsg(msg,data);
      cDarkPanel::OnLoopMsg(msg,data);
   }

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
      }
   }

   void RedrawDisplay()
   {
      // Draw the text 
      GUIcompose c;
      GUIsetup(&c,&text_rect,ComposeFlagRead,GUI_CANV_ANY); 

      char temp[4096],t2[255], mission[255];
      ShockStringFetch(temp,sizeof(temp),"mission","chargen",m_mission);
      gr_font_string_wrap(gShockFontBlue,temp,RectWidth(&text_rect));
      gr_font_string(gShockFontBlue,temp,0,0); 

      GUIdone(&c); 

      // Draw the mission
      GUIsetup(&c,&mission_rect,ComposeFlagRead,GUI_CANV_ANY); 

      ShockStringFetch(t2,sizeof(t2),"missiontitle","chargen");
      ShockStringFetch(mission,sizeof(mission),"post","usemsg",m_mission);
      sprintf(temp,t2,mission);
      gr_font_string_wrap(gShockFont,temp,RectWidth(&mission_rect));
      gr_font_string(gShockFont,temp,0,0); 

      GUIdone(&c); 

      // Draw the service icon
      GUIsetup(&c,&logo_rect,ComposeFlagRead,GUI_CANV_ANY); 

      int service;
      Point pt;
      if ((m_mission < 1) || (m_mission> 27))
      {
         Warning(("RedrawDisplay: mission is %d!\n",m_mission));
         m_mission = 1;
      }
      // we use -1 since mission 9 is in set 0, and so forth
      service = ((m_mission - 1)/ 9);
      if ((service < 0) || (service > 2))
      {
         Warning(("RedrawDisplay: service is %d! (mission %d)\n",service,m_mission));
         service = 0;
      }
      pt.x = 0;
      pt.y = 0;
      DrawByHandle(gLogoHnds[service],pt);

      GUIdone(&c); 

      // draw the cool debriefing art
      IRes *hnd;
      ShockStringFetch(temp,sizeof(temp),"missionpic","chargen",m_mission);
      if (strlen(temp) > 0)
      {
         GUIsetup(&c, &garethart_rect, ComposeFlagRead, GUI_CANV_ANY);

         hnd = LoadPCX(temp);
         pt.x = 0;
         pt.y = 0;
         DrawByHandle(hnd, pt);
         SafeFreeHnd(&hnd);

         GUIdone(&c);
      }
   }
}; 

//
// Panel descriptor 
//

static const char* debrief_button_names[] = 
{
   "continue",
}; 

sDarkPanelDesc cDebriefPanel::gDesc = 
{
   "debrief",
   cDebriefPanel::kNumRects,
   cDebriefPanel::kNumRects,
   cDebriefPanel::kNumRects,
   debrief_button_names, 
   NULL, // font
   0,
   kMetaSndPanelMain,
}; 


static cDebriefPanel* gpDebriefPanel = NULL; 

void SwitchToShockDebriefMode(BOOL push, int mission) 
{
   if (gpDebriefPanel)
   {
      gpDebriefPanel->m_mission = mission;
      cAutoIPtr<IPanelMode> panel = gpDebriefPanel->GetPanelMode(); 
      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
   }
}


const sLoopInstantiator* DescribeShockDebriefMode(void)
{
   if (gpDebriefPanel)
   {
      cAutoIPtr<IPanelMode> panel = gpDebriefPanel->GetPanelMode(); 
      return panel->Instantiator(); 
   }
   return NULL; 
}

//------------------------------------------------------------------
// INIT/TERM
//


void init_commands(); 

void ShockDebriefPanelInit()
{
   gpDebriefPanel = new cDebriefPanel; 

   init_commands(); 
}

void ShockDebriefPanelTerm()
{
   delete gpDebriefPanel; 
}


//------------------------------------------------------------------
// COMMANDS
//

static void do_debrief(int mission)
{
   SwitchToShockDebriefMode(TRUE,mission); 
}


static Command commands[] = 
{
   { "debrief", FUNC_INT, do_debrief, "Go to debrief screen, mission N.", HK_ALL }, 
}; 


static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}




