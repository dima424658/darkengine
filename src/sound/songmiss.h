/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sound/songmiss.h,v 1.2 2000/01/31 10:02:43 adurant Exp $
#pragma once

#ifndef SONGMISS_H
#define SONGMISS_H

////////////////////////////////////////////////////////////
// Mission Song Params
//

typedef struct sMissionSongParams
{
   char songName[32];
} sMissionSongParams;

//
// Init/Term
//
EXTERN void MissionSongInit(); 
EXTERN void MissionSongTerm(); 

//
// Get/Set
//
EXTERN const sMissionSongParams* GetMissionSongParams(void); 
EXTERN void SetMissionSongParams(const sMissionSongParams* params); 


#endif // SONGMISS_H
