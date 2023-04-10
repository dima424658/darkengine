/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkvo.h,v 1.1 1998/09/16 01:09:37 mahk Exp $
#pragma once  
#ifndef __DRKVO_H
#define __DRKVO_H
#include <objtype.h>

////////////////////////////////////////////////////////////
// DARK VOICE-OVER STUFF
//

EXTERN void DarkVoiceOverInit(void); 
EXTERN void DarkVoiceOverTerm(void); 

//
// Voice over events
// 

enum eVoiceEvents
{
   kVoiceWorldHighlight = 1 << 0,  // we world-highlight the object
   kVoicePickup         = 1 << 1,  // we pick up the object 
}; 

//
// Play a voice over for a particular object & event 
//
EXTERN BOOL DarkVoiceOverEvent(ObjID obj, ulong event); 

#endif // __DRKVO_H






