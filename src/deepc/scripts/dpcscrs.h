/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/deepc/scripts/dpcscrs.h,v 1.1 2000/01/26 19:05:22 porges Exp $
#pragma once
#ifndef __DPCSCRS_H
#define __DPCSCRS_H

#include <scrptbas.h>

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

};


#endif // __DPCSCRS_H


