/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkexp.cpp,v 1.3 2000/02/19 13:20:19 toml Exp $
//
// Deal with adding experience points when needed
//

#include <appagg.h>
#include <iobjsys.h>
#include <playrobj.h>

#include <netmsg.h>

#include <shkplayr.h>
#include <shkutils.h>
#include <shkovcst.h>
#include <shkovrly.h>

#include <shkexp.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

// Adds the specified number of points to the specified player
static void doShockAddExp(ObjID player, int amt, BOOL verbose)
{
   AutoAppIPtr(ShockPlayer);
   pShockPlayer->AddPool(player,amt);
   if (verbose)
   {
      char temp1[255], temp2[255];
      ShockStringFetch(temp1,sizeof(temp1),"AddExp","misc");
      // fill in amount of experience
      sprintf(temp2,temp1,amt);
      
      ShockOverlayAddText(temp2,DEFAULT_MSG_TIME);
   }
}

static cNetMsg *g_pAddExpMsg = NULL;

static void handleAddExp(int amount, BOOL verbose)
{
   doShockAddExp(PlayerObject(), amount, verbose);
}

static sNetMsgDesc sAddExpDesc =
{
   // @NOTE: this was originally a MetagameBroadcast, but damned if I
   // can see why...
   kNMF_Broadcast,
   "AddExp",
   "Add Experience",
   NULL,
   handleAddExp,
   {{kNMPT_Int, kNMPF_None, "Amount"},
    {kNMPT_BOOL, kNMPF_None, "Verbose"},
    {kNMPT_End}}
};

// Divvy the experience points up among the players. This is intended
// to be tuned. Currently assumes that, however we divide the experience,
// we do so evenly among the players.
static inline int ExpShare(int amt)
{
   return amt;
}

// Public interface
void ShockAddExp(ObjID player, int amt, BOOL verbose)
{
   int newAmt = ExpShare(amt);
   doShockAddExp(player, newAmt, verbose);
   g_pAddExpMsg->Send(OBJ_NULL, amt, verbose);
}

// Startup and Shutdown
void ShockExpInit(void)
{
   g_pAddExpMsg = new cNetMsg(&sAddExpDesc);
}

void ShockExpTerm(void)
{
   delete g_pAddExpMsg;
}
