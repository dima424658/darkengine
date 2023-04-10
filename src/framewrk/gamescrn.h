/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamescrn.h,v 1.2 1998/10/31 07:13:53 mahk Exp $
#pragma once  
#ifndef __GAMESCRN_H
#define __GAMESCRN_H

////////////////////////////////////////////////////////////
// GAME SCREEN MODE SUPPORT
//
// This is where we track what screen mode we *will* be in when we go to 
// game mode.  
//
// Someday we may have a callback API to track changes. 
//
typedef struct sScrnMode sScrnMode; 

EXTERN const sScrnMode* SetGameScreenMode(const sScrnMode* mode); 
EXTERN const sScrnMode* GetGameScreenMode(void); 

//
// Set a function that enforces constraints on the game screen mode. 
//

typedef void (*tScrnConstraintFunc)(sScrnMode* mode); 

EXTERN void ConstrainGameScreenMode(tScrnConstraintFunc func); 

#endif // __GAMESCRN_H

