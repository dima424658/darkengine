/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/modalui.h,v 1.3 2000/01/29 13:12:35 adurant Exp $
#pragma once

// Modal-UI system

#ifndef __MODALUI_H
#define __MODALUI_H

#include <stupidui.h>

// defines for ui_start's btn_check field
#define MODAL_CHECK_NONE   (0)
#define MODAL_CHECK_LEFT   (0x1)
#define MODAL_CHECK_RIGHT  (0x2)
#define MODAL_CHECK_ALL    (MODAL_CHECK_LEFT|MODAL_CHECK_RIGHT)

EXTERN BOOL modal_ui_start(int btn_check);
EXTERN void modal_ui_end(void);
EXTERN void modal_ui_update(void);

  // bool modal_ui_get_event(uiEvent *ev)
  //    returns FALSE if there are no events;
  //    and returns FALSE once per frame
  //    otherwise, stuffs the passed in structure
#define modal_ui_get_event get_stupid_ui_event

#endif
