/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkgame.h,v 1.6 2000/01/31 09:39:54 adurant Exp $
#pragma once

#ifndef DRKGAME_H
#define DRKGAME_H

////////////////////////////////////////////////////////////
// DARK GAME CODE
//

EXTERN void dark_sim_update_frame(void);
EXTERN void dark_rend_update_frame(void);
EXTERN void dark_start_gamemode(BOOL resuming);
EXTERN void dark_end_gamemode(BOOL suspending);
EXTERN void dark_init_game(void);
EXTERN void dark_term_game(void);

// this has to be set so that the update_frame can decide what to do with 
//  the currently viewed object and highlighting
EXTERN BOOL world_use_in_progress;

#endif // DRKGAME_H

