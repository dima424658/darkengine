/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/framewrk/contexts.h,v 1.6 2000/01/29 13:20:26 adurant Exp $
#pragma once

// hotkey contexts

  // distinct contexts are powers of two
#define  HK_GAME_MODE      0x01
#define  HK_BRUSH_EDIT     0x02
#define  HK_OBJ_EDIT       0x04
#define  HK_PANEL_MODE     0x08
#define  HK_COMMAND_MODE   0x10
#define  HK_GAME2_MODE     0x20

// these bits are reserved for app-specific modes
#define  HK_APP_MODES      0xFF00

#define  HK_EDITOR       (HK_BRUSH_EDIT|HK_OBJ_EDIT)
#define  HK_ALL          0xffffffff

