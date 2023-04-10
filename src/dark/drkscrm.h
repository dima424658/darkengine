/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once  
#ifndef __DARKSCRM_H
#define __DARKSCRM_H

#include <scrptmsg.h>

////////////////////////////////////////////////////////////
// DARK GAME SCRIPT MESSAGES  
//
// Messages relevant to Dark Game state (Enter/Exit Mode)
//
struct sDarkGameModeScrMsg: public sScrMsg
{
   boolean resuming;
   boolean suspending;

   sDarkGameModeScrMsg(ObjID towhom = 0, boolean r = FALSE, boolean s = FALSE)
      : sScrMsg(towhom, "DarkGameModeChange"), resuming(r), suspending(s) {};
}; 

#ifdef SCRIPT

#define OnDarkGameModeChange() SCRIPT_MESSAGE_HANDLER_SIGNATURE_(DarkGameModeChange,sDarkGameModeScrMsg)

#define DefaultOnDarkGameModeChange() SCRIPT_CALL_BASE(DarkGameModeChange)

#define DARKGAMEMODECHANGE_MESSAGE_DEFAULTS() \
        SCRIPT_DEFINE_MESSAGE_DEFAULT_(DarkGameModeChange,sDarkGameModeScrMsg)

#define DARKGAMEMODECHANGE_MESSAGE_MAP_ENTRIES() \
           SCRIPT_MSG_MAP_ENTRY(DarkGameModeChange)

#endif // SCRIPT

#endif // __DARKSCRM_H
