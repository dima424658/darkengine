/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmulti.h,v 1.19 2000/01/31 09:58:06 adurant Exp $
#pragma once

#ifndef __SHKMULTI_H
#define __SHKMULTI_H

#include <tagfile.h>

#define SMF_NONE       0
#define SMF_ELEVATOR   0x1
#define SMF_NETWORK_ON 0x2

EXTERN void ShockGetSaveDir(int slot, char *buf);

EXTERN void ShockLevelTransport(const char *newfile, int marker, uint flags);
EXTERN void ShockMultiLevelInit(void);
EXTERN void ShockMultiLevelTerm(void);

EXTERN void ShockQuickSave();
EXTERN void ShockQuickSaveHack();
EXTERN void ShockQuickLoad();

EXTERN HRESULT ShockLoadGame(int slot);
EXTERN HRESULT ShockSaveGame(int slot, char *savedesc = NULL);

EXTERN void ShockLoadFull(const char *fileroot);

EXTERN void ShockGetLevelName(char *str, int len);

EXTERN void ShockBeginGame();

EXTERN BOOL ShockGetSaveGameDesc(ITagFile *tagfile, char *buf, int buflen);

// Hacks that are exposed so that shksloop can initiate delayed saves:
EXTERN BOOL gTransSaveHack;
EXTERN int gQuickSaveHack;
EXTERN int gRemoteSaveHack;
EXTERN int gRemoteSaveSlot;
EXTERN char *gpRemoteSaveDesc;
EXTERN HRESULT DoShockSaveGame(int slot, char *savedesc);

EXTERN ITagFile* ShockOpenPatchedTagFile(const char* name); 
 
enum eSaveResults
{
   kShockSaveBlockOpen   = -1024, // Couldn't open a tag block
   kShockSaveReadWrite,           // Couldn't read or write to file
   kShockSaveBlockClose,          // Couldn't close a tag block
   kShockSaveCampaign,            // Couldn't read or write campaign
   kShockSaveMission,             // Couldn't read or write mission
   kShockSaveVersionInvalid,      // Invalid version 
}; 

enum eLoadResult
{
   kShockLoadNoFile = -1024,
   kShockLoadFailed = -1023,
   kShockLoadWrongHost = -1022,     // The save game has a different MP host
};

#endif
