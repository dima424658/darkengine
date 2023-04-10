/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shklding.cpp,v 1.11 1999/08/31 14:38:50 Justin Exp $
#include <appagg.h>
#include <shklding.h>

#include <scrnmode.h>
#include <scrnman.h>

#include <drkpanl.h>

#include <drkuires.h>
#include <imgsrc.h>
#include <resapi.h>
#include <respaths.h>
#include <fonrstyp.h>

#include <string.h>
#include <str.h>

#include <appapi.h>

#include <command.h>
#include <config.h>
#include <cfgdbg.h>
#include <ctype.h>

#include <simman.h>
#include <simtime.h>
#include <buttpanl.h>
#include <guistyle.h>
#include <uigame.h>

#include <dbfile.h>
#include <dbasemsg.h>
#include <tagfile.h>
#include <dbtagfil.h>
#include <scrnman.h>

#include <gcompose.h>
#include <shkuitul.h>

// For status dials
#include <stubfile.h>
#include <gshelapi.h>
#include <gameinfo.h>

// For shock game start 
#include <campaign.h>
#include <gamemode.h>
#include <dbfile.h>
#include <dbasemsg.h>
#include <drkdiff.h>
#include <simman.h>
#include <playrobj.h>
#include <shkmulti.h>
#include <shkmain.h>
#include <shkmenu.h>
#include <shkutils.h>
#include <shkgame.h>
#include <questapi.h>
#include <netsynch.H>
#include <shkdiff.h>

// For game load
#include <transmod.h>

// For level trans
#include <netman.h>
#include <netmsg.h>


//
// Include these last!
//
#include <dbmem.h>
#include <initguid.h>

// Loopmode id for loading mode, which no one should need. 
DEFINE_LG_GUID(LOOPID_ShockLoading, 0x2c5);



//------------------------------------------------------------
// CLASS: cLoading
//
class cShockLoadingPanel; 

class cShockLoadingPanel : public cDarkPanel
{
   static sDarkPanelDesc vars;

public:

   cShockLoadingPanel()
      : cDarkPanel(&vars)
   {
      
   }

   virtual ~cShockLoadingPanel()
   {
   }

   enum eRects
   {
      // No buttons 
      kNumButts,  

      kFineMeter = kNumButts, 
      kCoarseMeter,

      kTextLine, 

      kNumRects,
      
      kFirstAnim = kFineMeter,
   }; 

   enum
   {
      kNumDiffLevels = 3,
      kNumAnimImages = 20, 
      kNumAnims = 1, 
      kFineMeterRate = 16384,  // bytes per frame 
      kSynchMeterRate = 64, // bytes per millisecond 
      kFineMeterFrames = 20,
   };

   void Message(const char* msg)
   {
      Rect& r = mRects[(int)kTextLine];
      ShockDrawString(msg,r,0,&mStyle); 

      // We might be over-writing the 'hit escape' message 
      mHitEscape = FALSE; 
   }

   // Add a file's size to our totals
   void AddFile(ITagFile* file)
   {
      cAutoIPtr<ITagFileIter> iter = file->Iterate(); 
      for (iter->Start(); !iter->Done(); iter->Next())
         mFileTotal += file->BlockSize(iter->Tag());
   }

   void UpdateFineMeter(int bytes)
   {
      int old = mFineMeterCur; 
      mFineMeterCur += bytes; 

      int frame = mFineMeterCur/mMeterRate;
      int oldframe = old/mMeterRate; 
      if (frame != oldframe)
      {
         frame %= kNumAnimImages; 
         DrawAnimImage(kFineMeter,frame); 
      }
   }



   void UpdateMeters(int bytes)
   {
      int old = mFileCur;
      mFileCur += bytes; 

      //ConfigSpew("meter_spew",("bytes = %d delta = %d\n",mFileCur,bytes)); 

      // update fine meter
      UpdateFineMeter(bytes);

      // update coarse meter 
      {
         // Test to see whether the coarse meter would visually change
         int w = RectWidth(&mRects[(int)kCoarseMeter]); 
         int frame = mFileCur*w/mFileTotal; 
         int oldframe = old*w/mFileTotal;
         if (frame != oldframe)
         {
            DrawProgressBar(mFileCur,mFileTotal); 
         }
      }

      // pump events
      pGameShell->PumpEvents(kPumpAll);             
   }


   //
   // File proxy that snoops reads and updates meters
   //

   class cLoadingFile: public cTagFileProxy
   {
      cShockLoadingPanel* mpUs;
   public: 
      cLoadingFile(ITagFile* file, cShockLoadingPanel* us) 
         : cTagFileProxy(file), mpUs(us)
      {
         mpUs->AddFile(file); 
      } 

      STDMETHOD_(long,Read)(char* buf, int buflen)
      {
         mpUs->UpdateMeters(buflen); 
         return cTagFileProxy::Read(buf,buflen); 
      }

      STDMETHOD_(long,Move)(char* buf, int buflen)
      {
         mpUs->UpdateMeters(buflen); 
         return cTagFileProxy::Move(buf,buflen); 
      }


   }; 


   //  
   // DB file factory that generates cLoadingFile objects wrapped 
   // around regular disk tag files.
   //

   class cLoadingFileFactory : public cDBFileFactory
   {
      cShockLoadingPanel* mpUs; 
   public: 
      cLoadingFileFactory(cShockLoadingPanel* us)
         : mpUs(us)
      {
      }; 

      ITagFile* Open(const char* filename, TagFileOpenMode mode)
      {
         // if we're writing, just open the damn file.  None of this 
         // proxy crap.  
         if (mode == kTagOpenWrite)
            return TagFileOpen(filename,mode); 

         char buf[256]; 
         ConfigSpew("loading_open_spew",("Opening %s\n",filename)); 
         // The factory is required to find the file
         dbFind(filename,buf); 

         cAutoIPtr<ITagFile> file = TagFileOpen(buf,mode);
         return new cLoadingFile(file,mpUs); 
      }
   }; 


protected:
   IDataSource* mpAnimImages[kNumAnims][kNumAnimImages]; 
   IDataSource* mpProgBar; 

   enum ePhase
   {
      kLoad,
      kStartSynch,
      kWaitForSynch,
      kPostLoad, 
      kStartGame, 
   }; 

   int mFileTotal;
   int mFileCur; 
   int mMeterRate;
   int mFineMeterCur;  // current fine meter position 
   int mPhase; 
   BOOL mHitEscape; 

   void InitUI()
   {
      cDarkPanel::InitUI(); 

       for (int i = 0; i < kNumAnims; i++)
         for(int j = 0; j < kNumAnimImages; j++)
         {
            char buf[16];
            sprintf(buf,"load%c_%02d",'A'+i,j+1);
            ConfigSpew("meter_spew",("Binding %s\n",buf)); 
            mpAnimImages[i][j] = FetchUIImage(buf,mResPath+"\\meters"); 
         }

       mpProgBar = FetchUIImage("progress",mResPath+"\\meters"); 

       if (mFileTotal <= 0)
          mFileTotal = 1;  // make sure this is not zero 

       mMeterRate = kFineMeterRate; 
       config_get_int("progress_meter_rate",&mMeterRate); 

      uiHideMouse(NULL); 
      mHitEscape = FALSE; 

   }

   void TermUI()
   {
      uiShowMouse(NULL); 

      for (int i = 0; i < kNumAnims; i++)
         for(int j = 0; j < kNumAnimImages; j++)
         {
            SafeRelease(mpAnimImages[i][j]); 
         }

      SafeRelease(mpProgBar); 

      cDarkPanel::TermUI(); 
   }

   void DrawAnimImage(int meter, int image)
   {
      GUIcompose c; 

      if (image >= kNumAnimImages)
         image = kNumAnimImages - 1; 
      Rect& area = mRects[meter]; 
      IDataSource* img = mpAnimImages[meter-kFirstAnim][image]; 

      //      ConfigSpew("meter_spew",("[%d %d] ",meter-kFirstAnim,image)); 


      int compose_flags = ComposeFlagRead; 
      GUIsetup(&c,&area,(GUIcomposeFlags)compose_flags,GUI_CANV_ANY); 

      grs_bitmap* bm = (grs_bitmap*)img->Lock();
      gr_bitmap(bm,0,0); 
      img->Unlock(); 

      GUIdone(&c); 

      ScrnForceUpdateRect(&area); 
   }

   void DrawProgressBar(int cur, int total)
   {
      GUIcompose c; 

      Rect& area = mRects[(int)kCoarseMeter]; 
      IDataSource* img = mpProgBar; 

      int compose_flags = ComposeFlagRead; 
      GUIsetup(&c,&area,(GUIcomposeFlags)compose_flags,GUI_CANV_ANY); 

      // Set the cliprect based on cur and total 
      int w = RectWidth(&area); 
      int h = RectHeight(&area); 
      gr_set_cliprect(0,0,w*cur/total,h); 

      grs_bitmap* bm = (grs_bitmap*)img->Lock();
      gr_bitmap(bm,0,0); 
      img->Unlock(); 

      GUIdone(&c); 

      ScrnForceUpdateRect(&area); 
   }

   void OnEscapeKey()
   {
      // we hit escape once already, abort
      if (mHitEscape)
      {
         const sLoopInstantiator* menu = DescribeShockMainMenuMode(); 
         AutoAppIPtr(Loop); 
         pLoop->ChangeMode(kLoopModeUnwindTo,(sLoopInstantiator*)menu); 
      }
      else
      {
         Message(FetchUIString(panel_name,"escape",mResPath)); 
         // Must do this after 'message' 
         mHitEscape = TRUE; 
      }
   }


   


   // Here's where we do the dirty work.
   virtual void DoFileLoad()
   {
   }
   
   virtual void DoPostLoad()
   {
   }

   void LoadPhase()
   {
      Message(FetchUIString(panel_name,"loading",mResPath)); 
      ScrnForceUpdate(); 
      // Load the mission, then go to next state
      mFileCur = 1; // reset meter state 
      mFineMeterCur = 1; 
      mFileTotal = 1; // no dividing by zero 

      // Set up the file factory 
      cLoadingFileFactory fact(this); 
      dbSetFileFactory(&fact); 

      // do whatever dbLoad ops we want...
      DoFileLoad(); 

      // draw meters at the very end 
      mFileCur = 0; 
      UpdateMeters(mFileTotal); 

      dbSetFileFactory(NULL); 
   }

   void OnFrame(sLoopFrameInfo* frame)
   {
      switch(mPhase)
      {
         case kLoad:
            LoadPhase(); 
            mPhase++; 
            break; 
            
#ifdef NEW_NETWORK_ENABLED
         case kStartSynch:
            if (IsNetSynchEnabled())
            {
               ConfigSpew("loading_synch_spew",("Starting Synch...\n"));
               Message(FetchUIString(panel_name,"synching",mResPath)); 

               if (!PlayerObjectExists())
                  PlayerCreate(); // need to have a player before synch-ing 
               AutoAppIPtr(NetManager);
               pNetManager->StartSynch(); 
            }
            mPhase++; 
            break; 
         
         case kWaitForSynch:
            if (IsNetSynchEnabled())
            {
               ConfigSpew("loading_synch_spew",("."));

#ifndef SHIP
               if (config_is_defined("synch_forever"))
                  break; 
#endif 

               AutoAppIPtr(NetManager);
               if (pNetManager->Networking() ||
                   !pNetManager->IsNetworkGame()) 
               {
                  ConfigSpew("loading_synch_spew",("Off we go!\n"));
                  mPhase++; 
               }

               UpdateFineMeter(frame->dTicks*kSynchMeterRate); 
            
            }  
            else
               mPhase++; 
            break; 
#endif // NEW_NETWORK_ENABLED

         case kPostLoad:
            DoPostLoad(); 
            mPhase++;
            break; 

         case kStartGame:                  
         {
            GameModeDesc desc = { NULL, };

            AutoAppIPtr(Loop);
            sLoopInstantiator* gamemode = 
               DescribeGameMode(mmGameDefault,&desc); 
            pLoop->ChangeMode(kLoopModeUnwindTo,gamemode);
         }
         break; 

         default:
            mPhase++;
            break; 

      }; 
   }

   void OnButtonList(ushort /* action */ , int /* button */ )
   {
   }


   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data)
   {
      switch(msg)
      {
         case kMsgEnterMode:
            mPhase = kLoad; 
            break; 
         case kMsgNormalFrame:
         case kMsgPauseFrame:
         {
            sLoopFrameInfo* frame = (sLoopFrameInfo*)data; 
            OnFrame(frame); 
         }
         break; 

      }
      cDarkPanel::OnLoopMsg(msg,data); 
   }
   
};




static const char* loading_button_names[] = 
{
   "",  // no strings
};

sDarkPanelDesc cShockLoadingPanel::vars = 
{
   "loading", 
   cShockLoadingPanel::kNumButts, 
   cShockLoadingPanel::kNumRects, 
   cShockLoadingPanel::kNumButts, 
   loading_button_names,
   NULL, 0, 0, // font, flags, panel sound
   &LOOPID_ShockLoading
}; 


//------------------------------------------------------------
// INITIAL GAME LOAD PANEL 
//

#define DEFAULT_FIRST_LEVEL "earth.mis"

class cShockInitGamePanel: public cShockLoadingPanel
{
protected:

   void DoFileLoad()
   {
      extern int g_diff; 

      ShockBeginGame();

      AutoAppIPtr(Campaign);
      pCampaign->New(); 

#ifdef NEW_NETWORK_ENABLED
      AutoAppIPtr(NetManager);
      if (pNetManager->IsNetworkGame())
         g_diff = kShockDiffMultiplay; 
#endif 

      AutoAppIPtr(QuestData); 
      pQuestData->Create(DIFF_QVAR,g_diff,kQuestDataCampaign); 

      char first_level[16];
      BOOL no_interface;
      no_interface = TRUE;

      // if we have overriden the starting level, then disable the 
      // initial interface suppression.
      char path[256]; 
      if (config_get_raw("first_level",first_level,sizeof(first_level)) 
          && dbFind(first_level,path))
         no_interface = FALSE;
      else 
         strcpy(first_level,DEFAULT_FIRST_LEVEL); 

            
#ifdef NEW_NETWORK_ENABLED
      if (no_interface)
      {
         // In these introductory levels, we don't want to have any
         // networking:
         NetSynchDisable();
      }
#endif

      // Load the initial level 
      ITagFile* level_file = ShockOpenPatchedTagFile(first_level); 

      dbLoadTagFile(level_file,kFiletypeAll); 
      dbSetCurrentFile(first_level); 

      SafeRelease(level_file); 
      
      // set the no-HUD quest data
      if (no_interface)
      {
         AutoAppIPtr(QuestData); 
         pQuestData->Create("HideInterface",1,kQuestDataCampaign);
      }

      // prep for difficulty
      DarkPrepLevelForDifficulty(); 
      if (!PlayerObjectExists())
         PlayerCreate(); 

   }

   void DoPostLoad()
   {
      // Just in case
      ClearCursor();

      // Start the sim 
      AutoAppIPtr_(SimManager,pSimMan); 
      pSimMan->StartSim(); 

   }
}; 


static cShockInitGamePanel* gpInitGame = NULL; 


void SwitchToShockInitGame(BOOL push) 
{
   if (!gpInitGame)
      gpInitGame = new cShockInitGamePanel; 
   if (gpInitGame)
   {
      IPanelMode* panel = gpInitGame->GetPanelMode(); 

      panel->Switch((push) ? kLoopModePush : kLoopModeSwitch);  
      SafeRelease(panel); 
   }
}

sLoopInstantiator* DescribeShockInitGameMode(void) 
{
   if (!gpInitGame)
      gpInitGame = new cShockInitGamePanel; 
   if (gpInitGame)
   {
      cAutoIPtr<IPanelMode> panel = gpInitGame->GetPanelMode(); 
      return (sLoopInstantiator*)panel->Instantiator(); 
   }
   return NULL; 
}


//------------------------------------------------------------
// INITIAL GAME LOAD PANEL 
//

static cNetMsg *gpLoadGameMsg = NULL;

void handleLoadGame(int slot);

// The descriptor for the network load-game message:
static sNetMsgDesc gLoadGameDesc =
{
   kNMF_MetagameBroadcast,
   "LoadGame",
   "Load Game",
   NULL,
   handleLoadGame,
   {{kNMPT_Int, kNMPF_None, "Slot"},
    {kNMPT_End}}
};


class cShockLoadGamePanel: public cShockLoadingPanel
{
public:
   cShockLoadGamePanel()
      : mSlot(-1)
   {
   }

   void SetSlot(int slot)
   {
      mSlot = slot;
   }; 

   void SendMsg()
   {
      gpLoadGameMsg->Send(OBJ_NULL,mSlot);
   }

protected:
   int mSlot; 


   void DoFileLoad()
   {
      ShockLoadGame(mSlot); 
   }

   void DoPostLoad()
   {
   }
}; 


static cShockLoadGamePanel* gpLoadGame = NULL; 


void TransModeLoadGame(int slot) 
{
   if (!gpLoadGame)
      gpLoadGame = new cShockLoadGamePanel; 
   if (gpLoadGame)
   {
      gpLoadGame->SetSlot(slot); 
      gpLoadGame->SendMsg();
      IPanelMode* panel = gpLoadGame->GetPanelMode(); 

      panel->Switch(kLoopModeUnwindTo);
      SafeRelease(panel); 
   }
}

// Almost the same thing, without the message:
static void handleLoadGame(int slot) 
{
   if (!gpLoadGame)
      gpLoadGame = new cShockLoadGamePanel; 
   if (gpLoadGame)
   {
      gpLoadGame->SetSlot(slot); 
      IPanelMode* panel = gpLoadGame->GetPanelMode(); 

      panel->Switch(kLoopModeUnwindTo);
      SafeRelease(panel); 
   }
}

//------------------------------------------------------------
// LEVEL TRANSITION PANEL 
//

static cNetMsg *gpLevelTransMsg = NULL;

void handleShockLevelTransport(const char *newfile,
                                      int marker,
                                      uint flags);

// The descriptor for the network level-transition message:
static sNetMsgDesc gLevelTransDesc = 
{
   kNMF_Broadcast,
   "LevelTrans",
   "LevelTrans",
   NULL,
   handleShockLevelTransport,
   {{kNMPT_String, kNMPF_None, "Dest Level"},
    {kNMPT_Int, kNMPF_None, "Dest Loc"},
    {kNMPT_UInt, kNMPF_None, "Flags"},
    {kNMPT_End}}
};
BOOL gTransSaveHack = FALSE; 


class cShockLevelTransPanel: public cShockLoadingPanel
{
public:
   cShockLevelTransPanel()
      : mMarker(0),mFlags(0)
   {
   }


   void SetTransition(const char* level, int marker, uint flags)
   {
      mLevel = level; 
      mMarker = marker; 
      mFlags = flags; 
   }; 

   void SendMsg()
   {
      gpLevelTransMsg->Send(OBJ_NULL,(const char *)mLevel,mMarker,mFlags); 
   }

protected:
   cStr mLevel; 
   int mMarker; 
   uint mFlags; 


   void DoFileLoad()
   {
      ShockLevelTransport(mLevel,mMarker,mFlags); 
      gTransSaveHack = TRUE; // autosave on first frame
   }

   void DoPostLoad()
   {
      gbShockTransporting = FALSE;
   }
}; 


static cShockLevelTransPanel* gpLevelTrans = NULL; 

BOOL gbShockTransporting;

void TransModeSwitchLevel(const char* level, int marker, uint flags) 
{
   // It is essential that this be called before any subsequent net msgs:
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking())
   {
      pNetManager->PreFlush();
   }
   gbShockTransporting = TRUE;

   if (!gpLevelTrans)
      gpLevelTrans = new cShockLevelTransPanel; 
   if (gpLevelTrans)
   {
      gpLevelTrans->SetTransition(level,marker,flags); 
      gpLevelTrans->SendMsg(); 
      IPanelMode* panel = gpLevelTrans->GetPanelMode(); 

      panel->Switch(kLoopModeSwitch);
      SafeRelease(panel); 
   }
}


// same as above, but don't send the message
static void handleShockLevelTransport(const char *level,
                                      int marker,
                                      uint flags)
{
   // It is essential that this be called before any subsequent net msgs:
   AutoAppIPtr(NetManager);
   if (pNetManager->Networking())
   {
      pNetManager->PreFlush();
   }

   if (!gpLevelTrans)
      gpLevelTrans = new cShockLevelTransPanel; 
   if (gpLevelTrans)
   {
      gpLevelTrans->SetTransition(level,marker,flags); 
      IPanelMode* panel = gpLevelTrans->GetPanelMode(); 

      panel->Switch(kLoopModeSwitch);
      SafeRelease(panel); 
   }
}

//------------------------------------------------------------
// RAW SYNCH MODE PANEL 
//


class cShockSynchPanel: public cShockLoadingPanel
{
public:
   cShockSynchPanel()
   {
   }



protected:

   void DoFileLoad()
   {
   }

   void DoPostLoad()
   {
   }
}; 


static cShockSynchPanel* gpSynch = NULL; 



//------------------------------------------------------------------
// INIT/TERM
//



void ShockLoadingInit()
{
   gpLevelTransMsg = new cNetMsg(&gLevelTransDesc);
   gpLoadGameMsg = new cNetMsg(&gLoadGameDesc);
   gbShockTransporting = FALSE;

   // Set up synch mode 
   gpSynch = new cShockSynchPanel; 
   cAutoIPtr<IPanelMode> panel = gpSynch->GetPanelMode(); 
   SetNetSynchMode(panel->Instantiator()); 
}

void ShockLoadingTerm()
{
   delete gpInitGame;
   delete gpLevelTransMsg;
   delete gpLoadGameMsg;
   delete gpSynch; 
}

