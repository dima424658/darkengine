/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkloop.c,v 1.3 2000/02/19 12:27:21 toml Exp $
#include <lg.h>
#include <loopapi.h>

#include <drkloop.h>
#include <engfloop.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////
//  GAME MODE CLIENTS
//

static tLoopClientID* GameClients[] =
{
   &LOOPID_DarkSim,
   &LOOPID_DarkRender,
};


sLoopModeDesc DarkGameClients =
{
   { &LOOPID_DarkGameClients, "Dark Game mode clients"}, 
   GameClients,
   sizeof(GameClients)/sizeof(GameClients[0]),
};

////////////////////////////////////////////////////////////
//  BASE MODE CLIENTS
//


static tLoopClientID* BaseClients[] =
{
   &LOOPID_DarkSim,
   &LOOPID_EngineFeaturesClients,
};


sLoopModeDesc DarkBaseClients =
{
   { &LOOPID_DarkBaseClients, "Dark Base mode clients"}, 
   BaseClients,
   sizeof(BaseClients)/sizeof(BaseClients[0]),
};


////////////////////////////////////////









