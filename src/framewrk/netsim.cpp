/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netsim.cpp,v 1.9 1999/08/31 14:44:40 Justin Exp $
//
// Network controller for the simulation
//
// This is a close relative of gamemode.c. The thing about networked games
// is that we really only want to run the simulation when all of the players
// are currently in the sim. So we have this module, which holds the sim
// paused at any times when we have reason to believe that one or more
// other players are currently out of the sim.
//
// The proximate cause of this module is that we want to hold everyone
// paused while we're loading the first frame, because this can take a
// goodly while on a slow machine...
//

#include <mprintf.h>
#include <appagg.h>
#include <simflags.h>
#include <simstate.h>

#include <loopmsg.h>
#include <loopapi.h>
#include <gamemode.h>  // for LOOPID_Game

#include <netmsg.h>

#include <netman.h>
#include <netnotif.h>
#include <netloop.h>
#include <netsim.h>

// This should be the last include:
#include <dbmem.h>

//////////
//
// Major State Variables
//

// The number of players who we currently think are in the sim. When this
// is the same as the number of players who we believe to be in the game,
// we let the sim run:
static int g_PlayersInSim = 0;
// TRUE iff we currently have the sim paused because we don't think
// everyone is here yet:
static BOOL g_SimPaused = FALSE;
// TRUE iff we think we're in the first frame of game mode. We don't say
// that we're in until *after* that frame, since it takes a while:
static BOOL g_EnteringGame = FALSE;
// TRUE iff we're in or entering game mode; FALSE iff we're out or exiting
// it:
static BOOL g_InGameMode = FALSE;
// TRUE iff we're currently accepting messages.
static BOOL g_AcceptMsgs = FALSE;
// The callback to call when we pause or unpause, and any data that the
// app wants us to pass through:
static tNetSimCallback g_AppSimCallback = NULL;
static void *g_pClientData = NULL;
// TRUE iff the network connection has been utterly lost:
static BOOL g_NetworkDead = FALSE;
// TRUE iff we've reset and are entering the game for the first time
// since then:
static BOOL g_FirstEntering = FALSE;

//////////
//
// Routines to handle players entering and exiting
//

// This should get called whenever the number of players in the sim
// changes. It will pause or unpause the sim appropriately:
static void netSimCheckPlayers(BOOL meEntering)
{
   AutoAppIPtr(NetManager);
   int curplayers = pNetManager->NumPlayers();

   // Always unpause if we're going out of game mode:
   if (!g_InGameMode || (g_PlayersInSim >= curplayers))
   {
      // Okay, everyone's here, so unpause...
      if (g_SimPaused) {
         SimStateUnpause();
         if (g_AppSimCallback)
            g_AppSimCallback(FALSE, FALSE, g_pClientData);
         g_SimPaused = FALSE;
      }
      if (g_PlayersInSim > curplayers) {
         Warning(("More people in the sim than I have players...\n"));
         g_PlayersInSim = curplayers;
      }
   } else {
      // At least one player isn't in the sim...
      if (!g_SimPaused) {
         SimStatePause();
         // We need to keep rendering, otherwise our overlays go away.
         // This is faintly evil, since we're not *really* supposed to
         // put flags into the paused state. But it seems to work:
         SimStateSetFlags(kSimRender, TRUE);
         if (g_AppSimCallback)
            g_AppSimCallback(TRUE, g_FirstEntering, g_pClientData);
         g_SimPaused = TRUE;
      }
   }
}

static void netSimPlayerEntered(BOOL me)
{
   g_PlayersInSim++;
   netSimCheckPlayers(me);
}

static void netSimPlayerLeft()
{
   g_PlayersInSim--;
   netSimCheckPlayers(FALSE);
}


//////////
//
// Network messages
//

// Message to tell the other players that we are entering or leaving
// sim mode
static cNetMsg *g_pGameModeMsg = NULL;

static void handleGameMode(BOOL entering)
{
   if (!g_AcceptMsgs)
      // We haven't synched yet
      return;

   if (entering)
      netSimPlayerEntered(FALSE);
   else
      netSimPlayerLeft();
}

static sNetMsgDesc sGameModeDesc = 
{
   kNMF_MetagameBroadcast,
   "GameMode",
   "Enter/Exit Game Mode",
   NULL,
   handleGameMode,
   {{kNMPT_BOOL, kNMPF_None, "Entering?"},
    {kNMPT_End}}
};


//////////
//
// The loop client
//
// This is responsible for tracking when we enter and leave game mode,
// and pausing as needed
//
typedef void Context;

typedef struct _StateRecord
{
   Context* context;
} StateRecord;

static eLoopMessageResult LGAPI _SimLoopFunc(void* data,
                                             eLoopMessage msg,
                                             tLoopMessageData hdata)
{
   // useful stuff for most clients
   eLoopMessageResult result = kLoopDispatchContinue; 
   StateRecord* state = (StateRecord*)data;
   LoopMsg info;
   info.raw = hdata;

   switch(msg) {
      case kMsgResumeMode:
      case kMsgEnterMode:
      {
         if (!g_InGameMode) {
            g_InGameMode = TRUE;
            AutoAppIPtr(NetManager);
            // Are we entering game mode?
            if (IsEqualGUID(*info.mode->to.pID,LOOPID_GameMode) &&
                pNetManager->IsNetworkGame())
            {
               // Yep, so record that we're doing so:
               netSimPlayerEntered(TRUE);
               g_EnteringGame = TRUE;
               g_FirstEntering = FALSE;
            }
         }
         break;
      }
      case kMsgSuspendMode:
      case kMsgExitMode:
      {
         if (g_InGameMode) {
            g_InGameMode = FALSE;
            AutoAppIPtr(NetManager);
            // Are we leaving game mode?
            if (IsEqualGUID(*info.mode->from.pID,LOOPID_GameMode) &&
                pNetManager->IsNetworkGame())
            {
               // Yep, so record that we're doing so:
               netSimPlayerLeft();
               g_pGameModeMsg->Send(OBJ_NULL, FALSE);
            }
         }
         break;
      }
      case kMsgEndFrame:
         // Are we finishing up the first frame?
         if (g_EnteringGame) {
            g_pGameModeMsg->Send(OBJ_NULL, TRUE);
            g_EnteringGame = FALSE;
         }
         break;
      case kMsgEnd:
         delete state;
         break;
   }

   return result;
}

// Factory function for our loop client.
static ILoopClient* LGAPI _CreateSimClient(sLoopClientDesc * pDesc,
                                           tLoopClientData data)
{
   StateRecord* state;
   // allocate space for our state, and fill out the fields
   state = new StateRecord;

   return CreateSimpleLoopClient(_SimLoopFunc,
                                 state,
                                 &NetworkSimLoopClientDesc);
}

sLoopClientDesc NetworkSimLoopClientDesc =
{
   &LOOPID_NetworkSim,
   "Network Sim Loop Client",              
   kPriorityNormal,              
   kMsgsMode|kMsgEndFrame|kMsgEnd, // interests 

   kLCF_Callback,
   _CreateSimClient,
   
   NO_LC_DATA,

   {
      // We want to handle the EnterMode *after* the game mode system
      // itself. It will start the sim, which we may immediately pause:
      {kConstrainAfter, &LOOPID_Game, kMsgsMode},
      {kNullConstraint}
   }
};

//
// This gets called by the network system whenever a player enters
// or leaves. We check the balance in the sim when this happen.
//
static void netSimNumPlayersChanged(eNetListenMsgs situation,
                                    DWORD /* data */,
                                    void * /* pClientData */)
{
   switch (situation) {
      case kNetMsgReset:
         // Sneaky way of getting ourselves a universal dbReset check.
         // Since the database is being reset, we will assume that
         // everyone is out of sim.
         // It's not good enough to check in our own loop client,
         // because that only runs during game mode:
         if (!g_NetworkDead)
            // Have to be careful; a reset message also gets sent after
            // a NetworkLost one...
            g_PlayersInSim = 0;
         // Also, don't accept any messages until we resynchronize;
         // otherwise, we get bogus leaving-mode messages that the
         // other players sent before they reset:
         g_AcceptMsgs = FALSE;
         // Also, track that the next time we enter game mode will be
         // the first time for this level, so we send slightly
         // different messages:
         g_FirstEntering = TRUE;
         break;
      case kNetMsgSynchronize:
         // Okay, any mode-change messages from here on are real. This
         // must happen on synch, *not* on Networking, or we could lose
         // valid messages from other players.
         g_AcceptMsgs = TRUE;
         break;
      case kNetMsgNetworkLost:
         g_NetworkDead = TRUE;
         // Fall through
      default:
         // kNetMsgPlayerConnect || kNetMsgPlayerLost
         netSimCheckPlayers(FALSE);
         break;
   }
}

//////////
//
// API Stuff
//
void NetSimInit()
{
   g_pGameModeMsg = new cNetMsg(&sGameModeDesc);
   AutoAppIPtr(NetManager);
   pNetManager->Listen(netSimNumPlayersChanged,
                       (kNetMsgPlayerConnect |
                        kNetMsgPlayerLost |
                        kNetMsgReset |
                        kNetMsgSynchronize |
                        kNetMsgNetworkLost),
                       NULL);
}

void NetSimRegisterCallback(tNetSimCallback callback,
                            void *pClientData)
{
   g_AppSimCallback = callback;
   g_pClientData = pClientData;
}

void NetSimTerm()
{
   delete g_pGameModeMsg;
}
