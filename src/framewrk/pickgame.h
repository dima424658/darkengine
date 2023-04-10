/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/pickgame.h,v 1.2 2000/01/29 13:21:31 adurant Exp $
#pragma once

#ifndef PICKGAME_H
#define PICKGAME_H

////////////////////////////////////////////////////////////
// GAME SELECTION 
//
// For executables that implement multiple games, this 
// function creates an aggregate member for one of the 
// implemented games.
//
////////////////////////////////////////////////////////////

EXTERN void AppSelectGame(const char* game);


#endif // PICKGAME_H
