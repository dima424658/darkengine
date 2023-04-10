/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamemode.h,v 1.9 2000/01/29 13:20:57 adurant Exp $
#pragma once
#ifndef __GAMEMODE_H
#define __GAMEMODE_H

////////////////////////////////////////////////////////////
// GAME MODE LOOPMODE DESCRIPTION
//

DEFINE_LG_GUID(LOOPID_GameMode, 0x2b);
EXTERN struct sLoopModeDesc GameLoopMode;

//
// Game minor modes
//


typedef enum 
{
   mmGameMission,
   mmGameMeta,
   mmGameNoChange = -1,
   mmGameDefault = mmGameMission,
} GameMinorMode;

////////////////////////////////////////////////////////////
// GAME-SPECIFIC GAME MODE CLIENTS
//
// Game-specific systems should call this with the GUID 
// of a client or loopmode to add to game mode.  Only the most 
// recent call matters.
//

EXTERN void GameModeSetGameSpecClient(const GUID* clientID);


//////////////////////////////////////////////////////////////
// Instantiate the Game mode.
// a "null" descriptor will leave values unchanged from last time, 
// or use defaults if there was no last time.
//

// 
// Context data for starting game mode
//

typedef struct GameModeDesc
{
   struct sScrnMode* scrnmode; 
} GameModeDesc;


EXTERN struct sLoopInstantiator* DescribeGameMode(GameMinorMode minorMode, GameModeDesc* desc);
  
//
// Game loop client
//

DEFINE_LG_GUID(LOOPID_Game, 0x2c);
EXTERN struct sLoopClientDesc GameLoopClientDesc;


//
// THIS FILE IS FOR GAME MODE CONTROL FLOW STUFF ONLY.  DO NOT PUT RANDOM PROTOTYPES HERE.
// DO NOT INCLUDE OTHER FILES FROM THIS FILE.
//


#endif // __GAMEMODE_H


