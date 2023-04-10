/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/metagame.cpp,v 1.21 1998/11/03 06:36:09 mahk Exp $
#include <appagg.h>

#include <metagame.h>
#include <metaui_r.h>
#include <res.h>
#include <gfile.h>
#include <cursors.h>

#include <scrnmode.h>
#include <scrnman.h>

#include <resloop.h>

#include <panlmode.h>
#include <loopapi.h>
#include <loopmsg.h>
#include <panlguid.h>
#include <uiloop.h>
#include <drkmislp.h>

#include <dispapi.H>

#include <drkuires.h>
#include <imgsrc.h>

#include <string.h>

#include <backup.h>
#include <gamemode.h>
#include <dbfile.h>
#include <dbasemsg.h>
#include <cfgtool.h>

#include <appapi.h>

#include <command.h>
#include <config.h>
#include <ctype.h>

#include <simman.h>
#include <playrobj.h>
#include <buttpanl.h>

//
// Include these last!
//
#include <dbmem.h>
#include <initguid.h>
#include <metaguid.h>

typedef cDynArray<IImageSource*> cImageArray; 

//------------------------------------------------------------
// Loop Client
//

class cMetaGame : public cModalButtonPanel
{
public:

   static sLoopClientDesc Descriptor; 

   cMetaGame();
   ~cMetaGame() {}

   void OnLoopMsg(eLoopMessage, tLoopMessageData hData);

   enum
   {
      kPalRes = RES_MetagamePal,
   };

   // @TODO: move this into dark! we're just not fooling anyone that this is 
   // engine code 
   enum eButtons
   {
      kQuit = 4, 
   }; 

protected: 


   void InitUI();
   void TermUI(); 


   int NumButtons()
   {
      return Rects().Size(); 
   }

   cRectArray& Rects()
   {  
      return mRects; 
   }

   cElemArray& Elems()
   {  
      return mElems; 
   }

   IImageSource* CursorImage()
   {
      return FetchUIImage("cursor"); 
   }


   void OnButtonList(ushort action, int button); 
   void OnFrame(); 

   cRectArray mRects; 
   cElemArray mElems; 
   cImageArray mDownButts;
   cImageArray mUpButts;
   IImageSource* mpCursorImg;
   Cursor mCursor; 
   int mMission; 
};


//
// Button List event handler
//




void cMetaGame::InitUI()
{
   FetchUIRects("demonm", mRects);
      
   int n = mRects.Size(); 
   mElems.SetSize(n);
   mDownButts.SetSize(n); 
   mUpButts.SetSize(n); 

   for (int i = 0; i < n ; i++)
   {
      DrawElement& elem = mElems[i];
      memset(&elem,0,sizeof(elem)); 

         
      IImageSource*& pUp = mUpButts[i]; 
      IImageSource*& pDown = mDownButts[i]; 

      // fetch the image
      char buf[16]; 
      sprintf(buf,"demof%03d.pcx",i); 
      pUp = FetchUIImage(buf); 
      sprintf(buf,"demon%03d.pcx",i); 
      pDown = FetchUIImage(buf); 

      elem.draw_type = DRAWTYPE_BITMAP;
      elem.draw_data = (void*)pUp->Lock(); 
      elem.draw_data2 = (void*)pDown->Lock(); 

   }
}

void cMetaGame::TermUI()
{
   int n = mUpButts.Size(); 
   for (int i = 0; i < n; i++)
   {
      mUpButts[i]->Unlock(); 
      SafeRelease(mUpButts[i]); 
      mDownButts[i]->Unlock(); 
      SafeRelease(mDownButts[i]); 
   }
   mUpButts.SetSize(0);
   mDownButts.SetSize(0); 
   mRects.SetSize(0); 
   mElems.SetSize(0); 
}

void cMetaGame::OnButtonList(ushort action, int button)
{
   if (action & BUTTONGADG_LCLICK)
   {
      
      int map[16];
      int cnt = 16;  
      memset(map,0,sizeof(map)); 
      config_get_value("metagame_mission_map", CONFIG_INT_TYPE, map, &cnt); 
      mMission = map[button]; 

      if (mMission <= 0) // quit button 
      {
         AutoAppIPtr_(Application,pApp);
         pApp->Quit(); 
         return; 
      }

      

      
   }
}


void cMetaGame::OnFrame()
{
   if (mMission > 0)
   {
      // set the next mission 
      SetNextMission(mMission); 

      AutoAppIPtr(Loop);
      pLoop->EndMode(0); 

      mMission = -1; 
   }
}

//
// Loop message handler
//

void cMetaGame::OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
{
   LoopMsg info; 
   info.raw = data; 

   switch (msg)
   {
      case kMsgNormalFrame:
      case kMsgPauseFrame:
         OnFrame();
         break;
   }   

}

//
// Static: descriptor
//
static ILoopClient* LGAPI MetaGameClientFactoryFunc(sLoopClientDesc * pDesc, tLoopClientData data);

sLoopClientDesc cMetaGame::Descriptor = 
{
   &LOOPID_MetaGame,
   "MetaGame Panel Client", 
   kPriorityNormal, 
   kMsgEnd | kMsgsMode | kMsgsFrame ,
   

   kLCF_None,
   NULL,
   
   NO_LC_DATA,

   {
      { kConstrainAfter, &LOOPID_Panel, kMsgsMode|kMsgsFrame},
      { kNullConstraint },
   }
}; 




//------------------------------------------------------------
// PANEL MODE
//

static sScrnMode scrnmode_desc = 
{
   kScrnModeDimsValid,
   640, 480, 
}; 

static sPanelModeDesc metagame_desc = 
{ 
   &LOOPMODE_MetaGame,
   0, 
   &LOOPID_MetaGame,
   &scrnmode_desc,
   INTERFACE_PATH"/demoff"
}; 

//
// Construction/Destruction
//

cMetaGame::cMetaGame()
   : mMission(-1), cModalButtonPanel(&metagame_desc, &Descriptor)
{
}


static cMetaGame* gpMetaGame = NULL; 

void SwitchToMetaGameMode(BOOL push) 
{
   if (gpMetaGame)
   {
      IPanelMode* panel = gpMetaGame->GetPanelMode(); 

      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
      SafeRelease(panel); 
   }
}

static void create_panel_mode()
{
   cMetaGame* metagame = new cMetaGame; 
   IPanelMode* panel = metagame->GetPanelMode(); 
   IImageSource* bg = FetchUIImage("demoff"); 
   panel->SetImage(kPanelBG,bg);
   SafeRelease(bg);
   SafeRelease(panel);
   
   gpMetaGame = metagame; 
   
}

const sLoopInstantiator* DescribeMetaGameMode(void)
{
   if (gpMetaGame)
   {
      cAutoIPtr<IPanelMode> panel = gpMetaGame->GetPanelMode(); 

      return panel->Instantiator(); 
   }
   return NULL; 
}

//------------------------------------------------------------------
// INIT/TERM
//

void init_commands(); 

void MetaGameInit()
{
   create_panel_mode(); 
   init_commands(); 
}

void MetaGameTerm()
{
   if (gpMetaGame) {
      delete gpMetaGame;
   }
}

//------------------------------------------------------------
// CHeesy endgame mode 
//

static sPanelModeDesc endgame_desc = 
{ 
   &LOOPMODE_EndGame,
   kPanelCenterFG|kPanelESC|kPanelGrabBG,
   &GUID_NULL,
   NULL
}; 

void EndgameImagePanel(const char* respath, const char* image)
{
   if (config_is_defined("quit_from_endgame"))
   {
      endgame_desc.transition.mode = NULL; 
   }
   else
   {
      endgame_desc.transition.change = kLoopModeSwitch; 
      endgame_desc.transition.mode = DescribeMetaGameMode(); 
   }


   IPanelMode* panel = CreatePanelMode(&endgame_desc); 
   IImageSource* fg = CreateResourceImageSource(respath,image); 
   panel->SetImage(kPanelFG,fg);
   SafeRelease(fg);
   panel->Switch(kLoopModeSwitch); 
   SafeRelease(panel); 
}


//----------------------------------------
// COMMANDS
//

static void do_endgame(char* arg)
{
   if (config_is_defined("no_endgame"))
      return ;
   // chomp trailing whitespace
   char *s = arg+strlen(arg) - 1; 
   while (s > arg && isspace(*s)) 
      *s-- = '\0'; 
   EndgameImagePanel("books\\",arg); 
}

static void do_metagame()
{
   SwitchToMetaGameMode(TRUE); 
}


static Command commands[] = 
{
   { "metagame", FUNC_VOID, do_metagame, "Go to metagame UI.", HK_ALL }, 
   { "endgame", FUNC_STRING, do_endgame, "Go to endgame mode.", HK_ALL }, 

}; 


static void init_commands()
{
   COMMANDS(commands,HK_ALL); 
}



