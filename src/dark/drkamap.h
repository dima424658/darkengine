/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkamap.h,v 1.9 2000/02/29 19:10:56 patmac Exp $
#pragma once  
#ifndef __DRKAMAP_H
#define __DRKAMAP_H

#include <dbfile.h>
#include <dbtagfil.h>
#include <tagpatch.h>
#include <dbasemsg.h>

////////////////////////////////////////////////////////////
//
// Automap API  
//
struct sMapSourceData
{
    int sourcemiss; //mission num for info
    float compassdiff; //for redefining NORTH
};

typedef struct sMapSourceData sMapSourceData;

EXTERN void DarkAutomapInit(void);
EXTERN void DarkAutomapTerm(void); 
EXTERN void DarkAutomapDatabaseNotify( ulong msg, ITagFile *pFile );


EXTERN void DarkAutomapFirstFrameInit(void);

EXTERN void SwitchToDarkAutomapMode(BOOL push); 

EXTERN int GetMapSourceNum();
EXTERN float GetCompassOffset();
EXTERN void TransferMapInfo();
EXTERN void LoadMapSourceInfo(ITagFile* file);
EXTERN void SaveMapSourceInfo(ITagFile* file);
EXTERN void MapSourceInfoInit();
EXTERN void MapSourceInfoTerm();

EXTERN BOOL DarkAutomapGetLocationVisited( int page, int location );
EXTERN void DarkAutomapSetLocationVisited( int page, int location );
//
// Quest variables
//
#define MIN_PAGE_QVAR "MAP_MIN_PAGE"
#define MAX_PAGE_QVAR "MAP_MAX_PAGE"

#define OOB_PAGE_QVAR "MAP_OOB_PAGE"
#define OOB_LOC_QVAR  "MAP_OOB_LOC"

#define LOCS_VISITED_QVAR "MAP_LOCS_VISITED"



#endif // __DRKAMAP_H
