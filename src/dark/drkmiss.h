/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkmiss.h,v 1.4 1999/01/11 11:37:59 mahk Exp $
#pragma once  
#ifndef __DRKMISS_H
#define __DRKMISS_H
#include <comtools.h>

////////////////////////////////////////////////////////////
// DARK MISSION STUFF
//

#pragma pack(push, 1)
struct sMissionData
{
   int num; // mission num 
   char path[9];  // path name 
}; 
#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(sMissionData) == 13, "sMissionData is stored as a 13-byte file variable");
#endif

typedef struct sMissionData sMissionData; 

EXTERN const sMissionData* GetMissionData(); 

// Pre-load mission data
F_DECLARE_INTERFACE(ITagFile); 
EXTERN void LoadMissionData(ITagFile* file); 
EXTERN void SaveMissionData(ITagFile* file); 

//
// Init/Term
//

EXTERN void MissionDataInit(); 
EXTERN void MissionDataTerm(); 

//
// Open a mission file 
//
EXTERN ITagFile* OpenMissionFile(int missnum); 

#endif // __DRKMISS_H




