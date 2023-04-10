/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <lg.h>
#include <loopapi.h>

#include <dpcloop.h>
#include <engfloop.h>

////////////////////////////////////////////////////////////
// DEEP COVER GAME MODE CLIENTS
//

static tLoopClientID* GameClients[] =
{
   &LOOPID_DPCSim,
   &LOOPID_DPCRender,
   //&LOOPID_DPCRender2,
};


sLoopModeDesc DPCGameClients =
{
   { &LOOPID_DPCGameClients, "DeepCover Game mode clients"}, 
   GameClients,
   sizeof(GameClients)/sizeof(GameClients[0]),
};

////////////////////////////////////////////////////////////
// DEEP COVER BASE MODE CLIENTS
//

static tLoopClientID* BaseClients[] =
{
   &LOOPID_DPCSim,
   &LOOPID_EngineFeaturesClients,
};


sLoopModeDesc DPCBaseClients =
{
   { &LOOPID_DPCBaseClients, "Deep Cover Base mode clients"}, 
   BaseClients,
   sizeof(BaseClients)/sizeof(BaseClients[0]),
};


////////////////////////////////////////

