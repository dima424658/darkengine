/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vbrush.h,v 1.9 2000/01/29 13:13:22 adurant Exp $
// virtual brush/multi brush concentrator abstractions
#pragma once

#ifndef __VBRUSH_H
#define __VBRUSH_H

#include <tagfile.h>

///////// major systems
// init and free the vBrush system
EXTERN void vBrushInit(void);
EXTERN void vBrushReset(void);
EXTERN void vBrushFree(void);

// for save/load of files
EXTERN BOOL vBrush_NameSave(ITagFile *file);
EXTERN BOOL vBrush_NameLoad(ITagFile *file);
// pass res from save to load (group id, really)
EXTERN int  vBrushGetSaveData(void);
EXTERN int  vBrushParseLoadData(int saved);
EXTERN int  vBrush_GetFreeGroup(void);   // for loading subgroup sort of things

//////// playing with vBrushes
// the actual vBrush_Op's
#define vBrush_OP_ADD    1
#define vBrush_OP_REM    2
#define vBrush_OP_TOG    3
#define vBrush_OP_SAVE   4
#define vBrush_OP_RESET  5

// ways of manipulating the current vBrush
EXTERN BOOL vBrush_editBrush_Op(editBrush *us, int op);

//////// finding the "current brush"
// tells you if the given brush is in the current group
EXTERN BOOL vBrush_inCurGroup(editBrush *us);
// editBrush get gets you the virtual or normal editBrush for real ops
EXTERN editBrush *vBrush_editBrush_Get(void);
// GetSel gets you the current singular selection, for things like face selection?
EXTERN editBrush *vBrush_GetSel(void);

#define VIRTUAL_BRUSH_ID (-1)
// used by UndoRedo when the changed brush is really the vbrush
// you give it old_br, it makes old last_Br, returns vbrush current to stuff
EXTERN editBrush *vBrush_UndoVBrush(editBrush *old_br);

///////// doing things with the "current brush"
// delete or clone cur brush, vBrush aware
EXTERN void vBrush_DeletePtr(editBrush *toast);
EXTERN void vBrush_CloneCur(void);
EXTERN void vBrush_NewBrush(editBrush *new_br);
// selection/cycle
EXTERN void vBrush_SelectBrush(editBrush *sel);
EXTERN BOOL vBrush_cycle(int dir);
#define vBrush_getToCurGroup() vBrush_cycle(1)
// fancy setup for clicks
EXTERN void vBrush_click(editBrush *us, int flags, int x, int y);

// should be called whenever you initiate a brush focus switch behind the scenes
EXTERN void we_switched_brush_focus(void);

// time hacking craziness?
EXTERN void vBrush_go_EOT(void);

// go to the group of this groupid
EXTERN BOOL vBrush_GoToGroup(int group_id);

///////// actual thing to call a lot, as it were
// frame callback - complete means we are "done", false is for modalui/drag sort of stuff
EXTERN void vBrush_FrameCallback(void/*BOOL Complete*/);

// you can set this if you want changes, but there is only one of them for now
// of course, it should really be an array of callback or something
EXTERN void (*vBrush_Change_Callback)(editBrush *brush);

// this runs the op on the current vBrush group, or all brushes if run_all is TRUE
EXTERN void vBrush_GroupOp(BOOL run_all, void (*br_op)(editBrush *us));

// silly global meaning "dont update things with focus callbacks"
// mostly (only, as of now) used to disable GFH update during level load
EXTERN BOOL vBrush_DontUpdateFocus;
#endif  // __VBRUSH_H
