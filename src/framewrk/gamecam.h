/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/gamecam.h,v 1.2 2000/01/29 13:20:54 adurant Exp $
#pragma once

#ifndef __GAMECAM_H
#define __GAMECAM_H

#include <matrixs.h>

////////////////////////////////////////////////////////////
//
// Functions
//

EXTERN void move_game_cam (mxs_vector *pos);
EXTERN void move_game_cam_str (char *args);

// Does player position stay the same when we leave and reenter game mode?
extern bool persistent_player_pos;

#endif // __GAMECAM_H
