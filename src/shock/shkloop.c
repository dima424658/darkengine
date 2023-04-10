/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkloop.c,v 1.5 2000/02/19 13:25:38 toml Exp $
#include <lg.h>
#include <loopapi.h>

#include <shkloop.h>
#include <engfloop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
// SHOCK GAME MODE CLIENTS
//

static tLoopClientID* GameClients[] =
{
   &LOOPID_ShockSim,
   &LOOPID_ShockRender,
   //&LOOPID_ShockRender2,
};


sLoopModeDesc ShockGameClients =
{
   { &LOOPID_ShockGameClients, "Shock Game mode clients"}, 
   GameClients,
   sizeof(GameClients)/sizeof(GameClients[0]),
};

////////////////////////////////////////////////////////////
// SHOCK BASE MODE CLIENTS
//




static tLoopClientID* BaseClients[] =
{
   &LOOPID_ShockSim,
   &LOOPID_EngineFeaturesClients,
};


sLoopModeDesc ShockBaseClients =
{
   { &LOOPID_ShockBaseClients, "Shock Base mode clients"}, 
   BaseClients,
   sizeof(BaseClients)/sizeof(BaseClients[0]),
};


////////////////////////////////////////

