/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkscrs.h,v 1.8 2000/02/29 19:10:52 patmac Exp $
#pragma once
#ifndef __DRKSCRS_H
#define __DRKSCRS_H

#include <scrptbas.h>
#include <uigame.h>

////////////////////////////////////////////////////////////
// DARK GAME SCRIPT SERVICE
//




DECLARE_SCRIPT_SERVICE(DarkGame,0x1b4)
{

   //
   // Kill the player
   //
   STDMETHOD(KillPlayer)() PURE;

   //
   // End the mission
   //
   STDMETHOD(EndMission)() PURE;

   //
   // Fade out the screen
   STDMETHOD(FadeToBlack)(float time) PURE;

   // For now, this just notes the object as being found, so it's "secret" bit is reset.
   STDMETHOD(FoundObject)(ObjID obj) PURE;

   STDMETHOD_(BOOL, ConfigIsDefined)(const char *name) PURE;

   STDMETHOD_(BOOL, ConfigGetInt)(const char *name, int ref value) PURE;

   STDMETHOD_(BOOL, ConfigGetFloat)(const char *name, float ref value) PURE;

   STDMETHOD_(real, BindingGetFloat)(const char *name) PURE;

   STDMETHOD_(BOOL, GetAutomapLocationVisited)(int page, int location) PURE;

   STDMETHOD(SetAutomapLocationVisited)(int page, int location) PURE;
};


#endif // __DRKSCRS_H


