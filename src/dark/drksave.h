/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drksave.h,v 1.6 1998/11/03 06:38:05 mahk Exp $
#pragma once  
#ifndef __DRKSAVE_H
#define __DRKSAVE_H

#include <comtools.h>

////////////////////////////////////////////////////////////
// DARK SAVE GAME API
//

#define SAVE_PATH "saves"
#define QUICK_SAVE "quick.sav"
#define CHECKPOINT_SAVE "startmis.sav"

//----------------------------------------
// Init/Term
//
EXTERN void DarkSaveGameInit(void); 
EXTERN void DarkSaveGameTerm(void); 

//----------------------------------------
// Saving/Loading 
//
F_DECLARE_INTERFACE(ITagFile); 

//
// Return codes
// Not really HRESULTs, I know.  But this is a tiny module.
//

enum eSaveResults
{
   kDarkSaveBlockOpen   = -1024, // Couldn't open a tag block
   kDarkSaveReadWrite,           // Couldn't read or write to file
   kDarkSaveBlockClose,          // Couldn't close a tag block
   kDarkSaveCampaign,            // Couldn't read or write campaign
   kDarkSaveMission,             // Couldn't read or write mission
   kDarkSaveVersionInvalid,      // Invalid version 
   kDarkSaveCheckpoint,        // Couldn't read or write checkpoint file
}; 

// Save the game
EXTERN long DarkSaveGame(const char* description, ITagFile* file); 
EXTERN long DarkSaveGameFile(const char* desc, const char* fname); 
 
// Load the game, client should unwind to mission loop afterwards
EXTERN long DarkLoadGame(ITagFile* file); 
EXTERN long DarkLoadGameFile(const char* fname); 

// Saves the "checkpoint" file at start of mission
EXTERN void DarkSaveCheckpoint(void); 
EXTERN void DarkRestoreCheckpoint(void); 

// 
// Load just the string description 
//
EXTERN BOOL DarkLoadGameDescription(ITagFile* file, char* buf, int buflen); 

// 
// Are we mid-mission
//
EXTERN BOOL DarkGameIsMidMission(ITagFile* file);

//------------------------------------------------------------
// Save/load to the special "quick" save game slot. 
// Useful for testing 
//

EXTERN void DarkQuickSaveGame(void); 
EXTERN void DarkQuickLoadGame(void); 

enum eQuickLoadResult
{
   kQuickLoadNoFile = -1024,
   kQuickLoadFailed = -1023,
};

// Changes mode on success.  Returns code on failure 
EXTERN long DarkQuickLoadGameFile(const char* filename); 
#endif // __DRKSAVE_H



