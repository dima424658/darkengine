/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netsynch.cpp,v 1.10 2000/02/19 13:16:28 toml Exp $
//
// Network Synchronization Major Mode
//
// This module defines a fairly simple mode that we interject just before
// Game Mode. It deals with making sure that the players have exchanged
// any information that needs to get exchanged before we can really play
// the game in multiplayer.
//
// Other systems should invoke this mode instead of Game Mode; it will
// always exit into Game Mode. In non-networked games, this mode is
// essentially trivial, but it's simpler to always call it anyway.
//
// In the long run, we'll need to figure out how we want to deal with the
// graphics for this in a game-specific way...
//
// This code is all heavily bound up with networking. If NEW_NETWORK_ENABLED
// is not defined, you generally shouldn't call anything here, except
// maybe SwitchToNetSynchMode() (which is equivalent to starting game mode).
//

#include <appagg.h>
#include <netman.h>
#include <drkpanl.h>
#include <gamemode.h>
#include <playrobj.h>
#include <simman.h>

#include <netsynch.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

static void StartGameMode(eLoopModeChangeKind how)
{
   // Unwind to game mode
   GameModeDesc desc = { NULL, };

   AutoAppIPtr(Loop);
   sLoopInstantiator* gamemode = 
      DescribeGameMode(mmGameDefault,&desc); 
   pLoop->ChangeMode(how,gamemode);
}

class cSynchPanel : public cDarkPanel
{
   static sDarkPanelDesc g_Desc;

public:
   INetManager *m_pNetMan;

   cSynchPanel()
      : cDarkPanel(&g_Desc), m_ToGame(kLoopModeSwitch)
   {
      m_pNetMan = AppGetObj(INetManager);
   }

   ~cSynchPanel()
   {
      SafeRelease(m_pNetMan);
   }

   void SetChange(eLoopModeChangeKind kind)
   {
      m_ToGame = kind; 
   }


protected:
   BOOL m_Started;
   eLoopModeChangeKind m_ToGame; 

   void OnLoopMsg(eLoopMessage msg, tLoopMessageData data) 
   {
      switch(msg) {
         case kMsgEnterMode:
            // Do any pre-prep setup
            m_Started = FALSE;
            break;
         case kMsgNormalFrame:
         {
            if (!m_Started) {
               if (PlayerObject() == OBJ_NULL) {
                  // Make sure we have a player object before we
                  // synchronize:
                  PlayerCreate();
               }
               // Kick off synchronization
               m_pNetMan->StartSynch();
               m_Started = TRUE;
            } else if (m_pNetMan->Networking()
                       // We need to check for the network being lost, so
                       // we don't hang:
                       || !m_pNetMan->IsNetworkGame()) 
            {
               // We're ready to rumble...
               StartGameMode(m_ToGame);
            }
         }
         break;
      }
      cDarkPanel::OnLoopMsg(msg, data);
   }

   // This must be defined, but since we don't have any buttons...
   void OnButtonList(ushort action, int button)
   {
   }

   // Don't yet allow escaping out of this screen:
   void OnEscapeKey()
   {
   }
};

sDarkPanelDesc cSynchPanel::g_Desc =
{
   "synch",
   0,
   0,
   0,
   NULL,
   NULL,
   0,
   0
};

static cSynchPanel *g_pSynchPanel = NULL;
static BOOL g_bDisabled = FALSE;
static BOOL g_bDisableRemaps = FALSE;
static const sLoopInstantiator* g_pSynchMode = NULL;  

void SwitchToNetSynchMode(eLoopModeChangeKind how) 
{
   AutoAppIPtr(NetManager);
   if (!IsNetSynchEnabled())
      StartGameMode(how); 

   // If we've installed a synch mode, use it. 
   if (g_pSynchMode)
   {
      AutoAppIPtr(Loop);
      pLoop->ChangeMode(how,(sLoopInstantiator*)g_pSynchMode); 
      return; 
   }

   if (!g_pSynchPanel)
      g_pSynchPanel = new cSynchPanel; 
   

   g_pSynchPanel->SetChange(how);

   cAutoIPtr<IPanelMode> panel = g_pSynchPanel->GetPanelMode(); 
   panel->Switch(kLoopModeSwitch); 
}

#ifdef NEW_NETWORK_ENABLED
BOOL IsNetSynchEnabled()
{
   if (g_bDisabled)
      return FALSE; 

   AutoAppIPtr(NetManager); 
   return pNetManager->IsNetworkGame(); 
}
#else
BOOL IsNetSynchEnabled()
{
   return FALSE;
}
#endif 



void NetSynchDisable()
{
   g_bDisabled = TRUE;

   AutoAppIPtr(NetManager); 
   pNetManager->NonNetworkLevel();
}

void NetSynchEnable()
{
   g_bDisabled = FALSE;

   AutoAppIPtr(NetManager); 
   pNetManager->NormalLevel();
}

void SetNetSynchMode(const struct sLoopInstantiator* mode)
{
   g_pSynchMode = mode; 
}


// @HACK: this isn't really the right place for this, but there really
// isn't a good place that I can find. When we are coming out of disabled
// mode, it's important that the briefcase's contents *not* send remap
// messages, or things get All Fouled Up. So we have a little hook here
// to transmit that signal through.
void NetSynchDisableRemapMsgs()
{
   g_bDisableRemaps = TRUE;
}
void NetSynchEnableRemapMsgs()
{
   g_bDisableRemaps = FALSE;
}
BOOL NetSynchRemapMsgsDisabled()
{
   return g_bDisableRemaps;
}

void NetSynchInit()
{

}

void NetSynchTerm()
{
   delete g_pSynchPanel;
}



