/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/uiedit.h,v 1.8 2000/01/29 13:13:20 adurant Exp $
#pragma once

#ifndef __UIEDIT_H
#define __UIEDIT_H
#include <guistyle.h>
#include <uigame.h>

EXTERN void EditorCreateGUI(void);
EXTERN void EditorDestroyGUI(void);

EXTERN void uieditStyleSetup(void);
EXTERN void uieditStyleCleanup(void); 
EXTERN void uieditRedrawAll(void);

#define uieditFixupColor guiScreenColor
EXTERN void redraw_all_cmd(void);

   
#endif // __UIEDIT_H





