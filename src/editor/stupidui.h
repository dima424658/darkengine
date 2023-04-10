/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/stupidui.h,v 1.3 2000/01/29 13:13:15 adurant Exp $
#pragma once

#ifndef __STUPIDUI_H
#define __STUPIDUI_H

#include <lg.h>
#include <event.h>

//
// This is an editor-only stupid polling interface to the ui.
//

EXTERN void stupid_ui_start(void);
EXTERN void stupid_ui_end(void);

//
// get the next ui event.  returns false iff there are none yet.
//

EXTERN bool get_stupid_ui_event(uiEvent* ev);


#endif // __STUPIDUI_H
