/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkgame.h,v 1.9 2000/01/31 09:55:46 adurant Exp $
#pragma once

#ifndef SHKGAME_H
#define SHKGAME_H

////////////////////////////////////////////////////////////
// SHOCK GAME CODE
//

EXTERN void shock_sim_update_frame(int ms);
EXTERN void shock_rend_update_frame(void);
EXTERN void shock_start_gamemode(void);
EXTERN void shock_end_gamemode(void);
EXTERN void shock_init_game(void);
EXTERN void shock_term_game(void);
EXTERN void shock_hud(void);
EXTERN void shock_check_keys(void);

EXTERN bool MouseMode(bool mode, bool clear);
EXTERN void ThrowObj(ObjID o, ObjID src);

EXTERN bool shock_mouse;
extern bool mouse_icon;
extern ObjID drag_obj;
extern bool gNoMouseToggle;
EXTERN int shock_cursor_mode;

#endif // SHKGAME_H

