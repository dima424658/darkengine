/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef DPCGAME_H
#define DPCGAME_H

////////////////////////////////////////////////////////////
// Deep Cover Game Code
//

EXTERN void DPC_sim_update_frame(int ms);
EXTERN void DPC_rend_update_frame(void);
EXTERN void DPC_start_gamemode(void);
EXTERN void DPC_end_gamemode(void);
EXTERN void DPC_init_game(void);
EXTERN void DPC_term_game(void);
EXTERN void DPC_hud(void);
EXTERN void DPC_check_keys(void);

EXTERN bool MouseMode(bool mode, bool clear);
EXTERN void ThrowObj(ObjID o, ObjID src);

EXTERN bool  DPC_mouse;
extern bool  mouse_icon;
extern ObjID drag_obj;
extern bool  gNoMouseToggle;
EXTERN int   DPC_cursor_mode;

EXTERN BOOL  isEndingGameMode();

#endif // DPCGAME_H

