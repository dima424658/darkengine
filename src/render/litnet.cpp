/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/litnet.cpp,v 1.2 1999/04/02 12:04:19 Justin Exp $
//
// Networking for lights, specifically AnimLights
//

#include <lg.h>
#include <appagg.h>

#include <netmsg.h>

#include <litprops.h>
#include <litprop.h>
#include <litnet.h>

// Must be last:
#include <dbmem.h>

static cNetMsg *g_pLightModeMsg = NULL;

// The specified light has changed modes:
static void handleLightMode(ObjID obj, short mode)
{
   sAnimLightProp *anim_light;
   if (ObjAnimLightGet(obj, &anim_light))
   {
      anim_light->animation.mode = mode;
      anim_light->animation.refresh = TRUE;
      ObjAnimLightSet(obj, anim_light);
   }
}

static sNetMsgDesc sLightModeDesc =
{
   kNMF_Broadcast,
   "LightMode",
   "Anim Light Mode",
   NULL,
   handleLightMode,
   {{kNMPT_SenderObjID, kNMPF_IfHostedHere, "Obj"},
    {kNMPT_Short, kNMPF_None, "Mode"},
    {kNMPT_End}}
};

// Tell the other players that this light is changing, assuming that
// we own it.
// Assumption: this isn't going to get called terribly frequently.
void BroadcastAnimLightMode(ObjID obj, short mode)
{
   g_pLightModeMsg->Send(OBJ_NULL, obj, mode);
}

// Set things up
void AnimLightNetInit()
{
   g_pLightModeMsg = new cNetMsg(&sLightModeDesc);
}

void AnimLightNetTerm()
{
   delete g_pLightModeMsg;
}
