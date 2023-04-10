/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/vocore.h,v 1.1 1998/09/16 00:17:12 mahk Exp $
#pragma once  
#ifndef __VOCORE_H
#define __VOCORE_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// CORE VOICE-OVER SYSTEM 
//
// This is a tiny system for playing "voice overs" which are special
// schemas that can be omitted if their context isn't right. 
// 

typedef BOOL (LGAPI *tVOTestFunc)(void); 

//
// The voice-overs class
//


F_DECLARE_INTERFACE(IVoiceOverSys);
#undef INTERFACE 
#define INTERFACE IVoiceOverSys
DECLARE_INTERFACE_(IVoiceOverSys,IUnknown)
{
   DECLARE_UNKNOWN_PURE(); 

   //
   // Play a voiceover schema 
   // May actually choose not to play, if the conditions for playing 
   // the schema are not right.  
   //
   // Returns a schema handle 
   //
   STDMETHOD_(int,Play)(THIS_ int schema) PURE; 

   //
   // Observe whether a schema has been played.
   // Mark a schema as having been played, or not played already 
   //
   STDMETHOD_(BOOL,AlreadyPlayed)(THIS_ int schema) PURE; 
   STDMETHOD(SetAlreadyPlayed)(THIS_ int schema, BOOL played) PURE; 

   //
   // Set the game-specific callback that tests whether or not we
   // are "in combat."  Each call to Play() will call it.  
   //
   STDMETHOD(SetCombatTest)(THIS_ tVOTestFunc func) PURE; 

}; 
#undef INTERFACE 

EXTERN void VoiceOverCreate(void); 

#endif // __VOCORE_H
