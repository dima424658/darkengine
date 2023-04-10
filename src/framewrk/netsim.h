/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/netsim.h,v 1.2 2000/01/29 13:21:27 adurant Exp $
//
// Network controller for the simulation
//
// The NetSim module is responsible for making sure that the sim only
// runs if all players are in game mode. If anyone is slow to enter
// game mode, it pauses the sim; if anyone leaves game mode, it pauses
// the sim. This helps us not get too horribly out-of-synch, and
// helps prevent weird misbehaviours.
//
#pragma once

#ifndef __NETSIM_H
#define __NETSIM_H

//
// Startup/Shutdown
//
EXTERN void NetSimInit();
EXTERN void NetSimTerm();

//
// Application callback
//
// The NetSim system will periodically pause and unpause the simulation,
// if other players leave or enter. Ideally, this should be reflected in
// the UI in some fashion. This method allows you to register an
// app-specific callback, to tell the user what's up.
//
// The "pausing" parameter tells you whether we are pausing or unpausing;
// if pausing, the "enterGame" param tells you whether *you* are entering
// game mode or not, so that you can have subtly different msgs if
// desired.
//
typedef void (*tNetSimCallback)(BOOL pausing,
                                BOOL enterGame,
                                void *pClientData);
void NetSimRegisterCallback(tNetSimCallback callback,
                            void *pClientData);

#endif // !__NETSIM_H
