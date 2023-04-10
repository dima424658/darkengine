/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCMULTI_H
#define __DPCMULTI_H

#ifndef __TAGFILE_H
#include <tagfile.h>
#endif // !__TAGFILE_H

#define SMF_NONE       0
#define SMF_ELEVATOR   0x1
#define SMF_NETWORK_ON 0x2

EXTERN void DPCGetSaveDir(int slot, char *buf);

EXTERN void DPCLevelTransport(const char *newfile, int marker, uint flags);
EXTERN void DPCMultiLevelInit(void);
EXTERN void DPCMultiLevelTerm(void);

EXTERN void DPCQuickSave();
EXTERN void DPCQuickSaveHack();
EXTERN void DPCQuickLoad();

EXTERN HRESULT DPCLoadGame(int slot);
EXTERN HRESULT DPCSaveGame(int slot, char *savedesc = NULL);

EXTERN void DPCLoadFull(const char *fileroot);

EXTERN void DPCGetLevelName(char *str, int len);

EXTERN void DPCBeginGame();

EXTERN BOOL DPCGetSaveGameDesc(ITagFile *tagfile, char *buf, int buflen);

// Hacks that are exposed so that dpcsloop can initiate delayed saves:
EXTERN BOOL gTransSaveHack;
EXTERN int gQuickSaveHack;
EXTERN int gRemoteSaveHack;
EXTERN int gRemoteSaveSlot;
EXTERN char *gpRemoteSaveDesc;
EXTERN HRESULT DoDPCSaveGame(int slot, char *savedesc);

EXTERN ITagFile* DPCOpenPatchedTagFile(const char* name); 
 
enum eSaveResults
{
   kDPCSaveBlockOpen   = -1024, // Couldn't open a tag block
   kDPCSaveReadWrite,           // Couldn't read or write to file
   kDPCSaveBlockClose,          // Couldn't close a tag block
   kDPCSaveCampaign,            // Couldn't read or write campaign
   kDPCSaveMission,             // Couldn't read or write mission
   kDPCSaveVersionInvalid,      // Invalid version 
}; 

enum eLoadResult
{
   kDPCLoadNoFile    = -1024,
   kDPCLoadFailed    = -1023,
   kDPCLoadWrongHost = -1022,     // The save game has a different MP host
};

#endif  // !__DPCMULTI_H
