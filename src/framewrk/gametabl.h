/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gametabl.h,v 1.2 2000/01/29 13:21:00 adurant Exp $
#pragma once

#ifndef GAMETABL_H
#define GAMETABL_H
#include <lg.h>

////////////////////////////////////////////////////////////
// THE GAME TABLE
//
// This is used by AppSelectGame to select which game to run. 
//
// Each executable instantiates it differently. 
//
////////////////////////////////////////////////////////////

typedef struct GameTableElem
{
   char* name;           // Game name
   void (LGAPI *Create)(void); // Game's creation func, to be called at aggregate creation time   
} GameTableElem;

//
// A table of game names, the last of which has the name "NULL", and the default create function
//
EXTERN const GameTableElem GameTable[];  


#endif // GAMETABL_H
