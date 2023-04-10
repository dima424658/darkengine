/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shknet.cpp,v 1.41 1999/08/31 16:44:47 Justin Exp $

#include <lg.h>
#include <ctype.h>   // for isalnum()
#include <appagg.h>
#include <netman.h>
#include <playrobj.h>
#include <netmsg.h>
#include <netnotif.h>
#include <netsim.h>
#include <config.h>

// These are all for ShockPlayerCrash:
#include <iobjnet.h>
#include <contain.h>
#include <rendprop.h>
#include <physapi.h>
#include <objpos.h>
#include <phprops.h>
#include <phprop.h>
#include <rect.h>     // for Point
#include <shkinvpr.h>
#include <shkinv.h>
#include <objdef.h>   // for kObjectConcrete
#include <traitman.h>

#include <shkfsys.h>
#include <shknet.h>
#include <shkaipbs.h>
#include <shkovcst.h>
#include <shkovrly.h>
#include <shkcont.h>
#include <shkutils.h>
#include <shkprop.h>

// Other subsystems, which we want to initialize networking for:
#include <shkchat.h>
#include <shkrsrch.h>
#include <shkelev.h>

#include <ghostshk.h>

// Must be last:
#include <dbmem.h>

//////////
//
// Host name
//
// The host should have a distinct name for sessions he runs; this is mainly
// used to keep savegames from different hosts distinct.
//
static char gHostName[MAX_HOSTNAME_LEN + 1];

void ShockSetHostName(const char *name)
{
   strncpy(gHostName, name, MAX_HOSTNAME_LEN);
   // Make sure it's made up only of legal characters:
   char *pChar;
   for (pChar = gHostName;
        *pChar != '\0';
        pChar++)
   {
      if (!isalnum(*pChar) &&
          (*pChar != ' ') &&
          (*pChar != '_'))
      {
         *pChar = '_';
      }
   }
}

const char *ShockGetHostName()
{
   return gHostName;
}

////////////////////////////////////
//
// MESSAGES
//

//////////
//
// Display text
//
static void handleAddText(const char *pText, int time)
{
   ShockOverlayAddText(pText, time);
}

//
// Display the text on the specified player's HUD.
//
static cNetMsg *g_pAddTextAllMsg = NULL;

static sNetMsgDesc sAddTextAllDesc = {
   kNMF_Broadcast,
   "AddText",
   "Add Text",
   NULL,
   handleAddText,
   {{kNMPT_String},
    {kNMPT_Int},
    {kNMPT_End}}
};

//
// Display the text on a specific player's HUD.
static cNetMsg *g_pAddTextMsg = NULL;

static sNetMsgDesc sAddTextDesc = {
   kNMF_None,
   "AddText",
   "Add Text",
   NULL,
   handleAddText,
   {{kNMPT_String},
    {kNMPT_Int},
    {kNMPT_End}}
};

void ShockSendAddText(ObjID player, const char *pText, int time)
{
   if (player == OBJ_NULL) {
      g_pAddTextAllMsg->Send(OBJ_NULL, pText, time);
   } else {
      g_pAddTextMsg->Send(player, pText, time);
   }
}

///////
//
// The remove-from-container message
//
static cNetMsg *g_pRemoveContaineeMsg = NULL;

static void handleRemoveContainee(ObjID o, ObjID cont)
{
   ShockContainerCheckRemove(o, cont);
   AutoAppIPtr(ContainSys);
   pContainSys->Remove(cont, o);
}

static sNetMsgDesc sRemoveContaineeDesc =
{
   kNMF_Broadcast,
   "RemoveCont",
   "Remove From Container",
   NULL,
   handleRemoveContainee,
   {{kNMPT_SenderObjID, kNMPF_None, "Containee"},
    // This must be a global, because it is possible for Container and
    // Containee to have different owners. This will happen in the case
    // of pre-placed loot within a handed-off AI:
    {kNMPT_GlobalObjID, kNMPF_None, "Container"},
    {kNMPT_End}}
};

//
// Tell the clients to take the given object out of the container. This
// will contact all clients, *including this one*
// Host code
//
void ShockBroadcastRemoveContainee(ObjID o, ObjID cont)
{
   if (cont == OBJ_NULL)
      return;

   g_pRemoveContaineeMsg->Send(OBJ_NULL, o, cont);

   // Tell our own database as well as the other clients
   handleRemoveContainee(o, cont);
}

//////////
//
// Send Host's name
//
static cNetMsg *g_pHostNameMsg = NULL;

static void handleHostName(const char *name)
{
   ShockSetHostName(name);
}

static sNetMsgDesc sHostNameDesc =
{
   kNMF_MetagameBroadcast,
   "HostName",
   "Host Name",
   NULL,
   handleHostName,
   {{kNMPT_String, kNMPF_None, "Name"},
    {kNMPT_End}}
};

//////////
//
// Hilight an object
//
static cNetMsg *g_pHilightObjectMsg = NULL;

#define SHOW_HILIGHT_TIME 10000

static void handleHilightObject(ObjID obj, ObjID from)
{
   if (!IsPlayerObj(from))
   {
      char objName[128];
      ObjGetObjShortNameSubst(obj, objName, sizeof(objName));
      AutoAppIPtr(NetManager);
      const char *fromName = pNetManager->GetPlayerName(from);
      char temp[255];
      if (ShockStringFetch(temp, sizeof(temp), "HilightString", "misc"))
      {
         char buf[255];
         sprintf(buf, temp, objName, fromName);
         ShockOverlayAddText(buf, SHOW_HILIGHT_TIME);
      }
   }

   int expireTime;
   if (gPropHUDTime->Get(obj, &expireTime) && (expireTime == 0))
   {
      // Don't do anything; this sucker's under our cursor, and should
      // be highlighted until it's explicitly turned off
   } else {
      gPropHUDTime->Set(obj, GetSimTime() + SHOW_HILIGHT_TIME);
   }
}

static sNetMsgDesc sHilightObjectDesc =
{
   kNMF_Broadcast | kNMF_AppendSenderID,
   "Hilight",
   "Hilight an Object",
   NULL,
   handleHilightObject,
   {{kNMPT_GlobalObjID, kNMPF_None, "Object"},
    {kNMPT_End}}
};

void ShockBroadcastHilightObject(ObjID obj)
{
   g_pHilightObjectMsg->Send(OBJ_NULL, obj);
   handleHilightObject(obj, PlayerObject());
}

////////////////////////////////////
//
// NETWORK LISTENERS
//

//
// Deal with a player who has crashed out of the game. This will get called
// by the networking system (or possibly at other times) if needed. This
// isn't trying to be perfect -- it's trying to clean up so that the
// remaining players can keep going...
//
// Explicit assumption: containment links are replicated on all machines,
// so we know what the player had in his inventory.
//
static void shockPlayerCrash(DWORD data)
{
#define MAX_TEMPLATE_LEN 64
   char templ[MAX_TEMPLATE_LEN];
   ObjID player = (ObjID) data;

   if (player == OBJ_NULL)
   {
      // Looks like the crash was sometime very early in the game, before
      // we had a player ID for him. In which case, there's not much we
      // can do here, except print a generic message.
      if (ShockStringFetch(templ, MAX_TEMPLATE_LEN,
                           "UnknownDisconnected", "Network", -1))
      {
         ShockSendAddText(OBJ_NULL, templ, 5000);
         ShockOverlayAddText(templ, 5000);
      }

      return;
   }

   // This should only get called on the default host...
   AutoAppIPtr(NetManager);
   Assert_(pNetManager->AmDefaultHost());

   // Say that the player's gone:
   if (ShockStringFetch(templ, MAX_TEMPLATE_LEN,
                        "PlayerDisconnected", "Network", -1))
   {
      char msgBuf[MAX_PLAYER_NAME_LEN + MAX_TEMPLATE_LEN];
      sprintf(msgBuf, 
              templ,
              pNetManager->GetPlayerName(player));
      ShockSendAddText(OBJ_NULL, msgBuf, 5000);
      ShockOverlayAddText(msgBuf, 5000);
   }

   // Pick up all of the player's stuff, reassign it back to the world,
   // and put it in a nice box. This code is liberally adapted from the
   // throw-to-world code; we might want to restructure that a little and
   // use it.
   // First, set up for the boxes.
   ObjID currentBox = OBJ_NULL;
   AutoAppIPtr(ObjectSystem);
   ObjID boxArch = pObjectSystem->GetObjectNamed("Personal Effects Box");
   if (boxArch == OBJ_NULL) {
      Warning(("Couldn't create personal effects boxes!\n"));
      return;
   }
   mxs_real boxRadius = 0;
   // Get the dimensions for each box:
   Point boxDims = ContainDimsGetSize(boxArch);
   // Buffer to keep track of the layout within the box, so that we
   // don't overfill it:
   ObjID *containerContents = 
      (ObjID *) malloc((boxDims.x * boxDims.y) * sizeof(ObjID));
   // The slot to slap each object into:
   int slot = 0;
   // Figure out the location to place the lowest box. Basically,
   // we're trying to place the boxes within the two big spheres
   // that make up the bulk of the player's body, because that's
   // the only space we can be pretty sure is clear. Note that we
   // implicitly assume that the boxes are pretty small, probably
   // less than a foot. Note that we can't use the physics location
   // of the player, because we can't count upon him being physical
   // any more.
   mxs_vector boxLoc = *ObjPosGetLocVector(player);
   // PhysGetSubModLocation(player, PLAYER_BODY, &boxLoc);
   boxLoc.z -= (PLAYER_RADIUS / 2);
   mxs_angvec nullAngle = {0, 0, 0};

   // Find the player's keyring, so we don't actually copy it. I'm
   // sure there must be a way to do this without hardcoding the name
   // of the damned archetype, but I'm not thinking of it right now...
   ObjID keyarch = pObjectSystem->GetObjectNamed("fakekeys");

   // Now actually go through the objects...
   AutoAppIPtr(ObjectNetworking);
   AutoAppIPtr(ContainSys);
   sContainIter *pIter;
   for (pIter = pContainSys->IterStart(player);
        !pIter->finished;
        pContainSys->IterNext(pIter))
   {
      // For each object the player had...
      ObjID hisObj = pIter->containee;

      // ... take that object over. This is a little tricky; since we're
      // getting the objnum from the dead player's inventory, we have to
      // remember that we're currently holding a proxy representation;
      // we now make that the official one.
      // @TBD (justin 5/16): What? That makes no sense...
      // ObjID obj = pObjectNetworking->ObjGetProxy(player, hisObj);
      ObjID obj = hisObj;
      pObjectNetworking->ObjTakeOver(obj);

      // If it's a fakekeys, just destroy it. There's no point
      // in putting it back in world.
      AutoAppIPtr(TraitManager);
      ObjID arch = pTraitManager->GetArchetype(obj);
      if (arch == keyarch) {
         pObjectSystem->Destroy(obj);
         continue;
      }

      // Put the object back into the world. Figure out which slot to
      // stick the object into:
      if ((currentBox == OBJ_NULL) ||
          (!ShockInvFindSpace(containerContents, boxDims, obj, &slot)))
      {
         // Either there's no box, or the previous one is full, so
         // create a new one:
         currentBox = pObjectSystem->Create(boxArch, kObjectConcrete);
         PhysRegisterSphereDefault(currentBox);
         // Get the physical height of the box, so we can stack them up:
         cPhysDimsProp *pBoxSize;
         if (!g_pPhysDimsProp->Get(currentBox, &pBoxSize)) {
            Warning(("Personal Effects box has no size!\n"));
            return;
         }
         boxRadius = pBoxSize->radius[0];
         // Put the box where the player was.
         // @TBD: give the boxes different orientations
         PhysSetModLocation(currentBox, &boxLoc);
         boxLoc.z += (boxRadius * 4);
         int i;
         for (i=0; i < boxDims.x * boxDims.y; i++)
            containerContents[i] = OBJ_NULL;
         // Now find the right slot in this new box:
         if (!ShockInvFindSpace(containerContents, boxDims, obj, &slot)) {
            Warning(("Empty personal effects box can't hold obj %d!\n", obj));
            return;
         }
      }
      // Take the object out from the player. We don't need to do this
      // explicitly for our own sake, but we might for the other players
      // who are watching:
      pContainSys->Remove(player, obj);
      // Now slap the object into that slot:
      pContainSys->Add(currentBox, obj, slot, CTF_NONE);
      ShockInvFillObjPos(obj, containerContents, slot, boxDims);
   }
   pContainSys->IterEnd(pIter);
}

//
// Utility method, to add a fetched network string.
//
#define MAX_NETSTRING_LEN 64
static void shockNetAddText(const char *msgName, int time)
{
   char msg[MAX_NETSTRING_LEN];
   if (ShockStringFetch(msg, MAX_NETSTRING_LEN, msgName, "Network", -1)) {
      ShockOverlayAddText(msg, time);
   }
}

//
// What to do when our connection to the server gets lost. For the time
// being, we just put a message on the HUD if he's in sim mode. We don't
// yet do anything if he's in metagame.
//
// @TBD: Do something clearer and more user-friendly. Be nice to the poor
// user who has just gotten his session cut off. Recommend running with
// a longer timeout, if he doesn't think he should have been cut off.
// Remember that this may occur when the host simply exits normally.
static void shockNetworkLost()
{
   if (IsSimTimePassing()) {
      shockNetAddText("NetworkLost", 30000);
   }
}

//
// A new player has connected. If we're the host, broadcast our host name.
// This is a smidgeon inefficient, since it means we'll send once per
// player, but that's not too bad.
//
static void shockPlayerConnect()
{
   AutoAppIPtr(NetManager);
   if (pNetManager->AmDefaultHost()) {
      g_pHostNameMsg->Send(OBJ_NULL, gHostName);
   }
}

//
// @HACK: this is basically ripped out of netman. Properly speaking, it
// *should* be here, because it's a very app-dependent kinda function.
// In the long run, netman should define an interface that the app is
// supposed to implement, to return app-dependent stuff like this. But
// I'm not messing around at that level this late...
//
#define DEFAULT_AVATAR_NAME "Default Avatar"
static ObjID MyAvatarArchetype()
{
   char abstractName[128];
   ObjID abstractPlayer = OBJ_NULL;
   AutoAppIPtr(NetManager);
   AutoAppIPtr(ObjectSystem);
   int MyPlayerNum = pNetManager->MyPlayerNum();

   // The net_abstract_player is the archetype to be created for
   // our player on other player's machines.
   if (!config_get_raw("net_avatar", 
                       abstractName, 
                       sizeof abstractName))
   {
      // Use the default player
      sprintf(abstractName, "%s %d", DEFAULT_AVATAR_NAME, MyPlayerNum);
      abstractPlayer = pObjectSystem->GetObjectNamed(abstractName);
      AssertMsg1(abstractPlayer,
                 "Default avatar %s not in gamesys!",
                 abstractName);
   } else {
      abstractPlayer = pObjectSystem->GetObjectNamed(abstractName);
      if (abstractPlayer == OBJ_NULL) {
         Warning(("Unknown abstract player object %s",abstractName));
         // Use the default player
         sprintf(abstractName, "%s %d", DEFAULT_AVATAR_NAME, 
                 MyPlayerNum);
         abstractPlayer = pObjectSystem->GetObjectNamed(abstractName);
         AssertMsg1(abstractPlayer,
                    "Default avatar %s not in gamesys!",
                    abstractName);
      }
   }
   
   return abstractPlayer;
}

//
// We've begun networking. Make sure that our own map icon is colored
// appropriately. (The other players get their colors from their avatars.)
//
static void shockPlayerIconSetup()
{
   ObjID myAvatar = MyAvatarArchetype();
   Label *iconName;
   if ((myAvatar != OBJ_NULL) &&
       gPropMapObjIcon->Get(myAvatar, &iconName))
   {
      // Set the player's map icon to the avatar's version
      gPropMapObjIcon->Set(PlayerObject(), iconName);
   }
}

//
// Listen into the network, and see if anything interesting happens...
//
static void networkListener(eNetListenMsgs situation,
                            DWORD data,
                            void * /* pClientData */)
{
   switch (situation) {
      case kNetMsgPlayerLost:
         shockPlayerCrash(data);
         break;
      case kNetMsgNetworkLost:
         shockNetworkLost();
         break;
      case kNetMsgPlayerConnect:
         shockPlayerConnect();
         break;
      case kNetMsgNetworking:
         shockPlayerIconSetup();
         break;
   }
}

//
// App-specific callback from netsim, to deal with when we have to
// pause the game while someone isn't in the sim.
//
// @TBD: change these messages to come from the strings file...
//
static BOOL g_enteringGame = FALSE;
static void shockNetDisplayPause(BOOL paused,
                                 BOOL enterGame,
                                 void * /* pClientData */)
{
   if (paused) {
      if (enterGame) {
         shockNetAddText("NetWait", 1);
         g_enteringGame = TRUE;
      } else {
         shockNetAddText("NetPause", 1);
         g_enteringGame = FALSE;
      }
   } else {
      if (g_enteringGame) {
         shockNetAddText("NetJoined", 5000);
      } else {
         shockNetAddText("NetResume", 5000);
      }
   }
}

//
// Get the color appropriate for the specified player, by either player
// number or object.
//
// These colors were chosen more or less arbitrarily; they're roughly the
// ones Gareth decided to put on the player armbands, so we might as well
// be consistent.
//
int ShockPlayerNumColor(int playerNum)
{
   static int red[3] = {255, 150, 150};
   static int blue[3] = {150, 150, 255};
   static int purple[3] = {255, 150, 255};
   static int yellow[3] = {255, 255, 150};
   switch(playerNum)
   {
      case 1:
         return FindColor(red);
      case 2:
         return FindColor(blue);
      case 3:
         return FindColor(purple);
      case 4:
         return FindColor(yellow);
      default:
         return gShockTextColor;
   }
}

int ShockPlayerColor(ObjID playerObj)
{
   AutoAppIPtr(NetManager);
   return ShockPlayerNumColor(pNetManager->ObjToPlayerNum(playerObj));
}

////////////////////////////////////
//
// STARTUP and SHUTDOWN
//
// Set up the network client. We assume that NetManager has been initialized
//
void ShockNetInit()
{
   AutoAppIPtr(AIManager);
   IAIBehaviorSet *pBehaviorSet = new cAIShockProxyBehaviorSet;
   pAIManager->InstallBehaviorSet(pBehaviorSet);
   SafeRelease(pBehaviorSet);

   // Tell the NetManager what to do if a player crashes:
   AutoAppIPtr(NetManager);
   pNetManager->Listen(networkListener, 
                       kNetMsgPlayerLost | kNetMsgNetworkLost
                       | kNetMsgPlayerConnect | kNetMsgNetworking,
                       NULL);

   // Tell the NetSim system what to do when the game is paused:
   NetSimRegisterCallback(shockNetDisplayPause, NULL);

   // Set up the initial host name:
   if (!config_get_raw("net_hostname",
                       gHostName,
                       sizeof gHostName))
   {
      // A backup default, just to make sure it's not garbage:
      ShockSetHostName("Host");
   }

   // Create the standalone messages:
   g_pAddTextMsg = new cNetMsg(&sAddTextDesc);
   g_pAddTextAllMsg = new cNetMsg(&sAddTextAllDesc);
   g_pRemoveContaineeMsg = new cNetMsg(&sRemoveContaineeDesc);
   g_pHostNameMsg = new cNetMsg(&sHostNameDesc);
   g_pHilightObjectMsg = new cNetMsg(&sHilightObjectDesc);

   // Also create any messages that other subsystems want created:
   ShockChatNetInit();
   ShockResearchNetInit();
   ShockElevNetInit();

   ShockGhostInit();
}

// Shut down Shock-specific networking. Doesn't currently need to do
// anything.
void ShockNetTerm()
{
   // Delete the standalone messages:
   delete g_pAddTextMsg;
   delete g_pAddTextAllMsg;
   delete g_pRemoveContaineeMsg;
   delete g_pHostNameMsg;
   delete g_pHilightObjectMsg;

   // And the messages in other subsystems:
   ShockChatNetTerm();
   ShockResearchNetTerm();
   ShockElevNetTerm();
}
