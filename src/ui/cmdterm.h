/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/cmdterm.h,v 1.4 2000/01/31 10:04:19 adurant Exp $
#pragma once

#ifndef __CMDTERM_H
#define __CMDTERM_H

struct _LGadRoot;
struct Rect;

// free the history buffers and exit 
//   (these might write to history.cfg? should that be implied or built?)
EXTERN void history_free_all(void);
EXTERN void history_start(void);

//
// Command terminal flags
//

enum CmdTermFlags
{
   kCmdTermNoFlags = 0, 
   kCmdTermHideUnfocused = (1 << 0), 
   kCmdTermBeginFocused = (1 << 1), 
   kCmdTermScrollable = (1 << 2),
   kCmdTermNoPause = (1 << 3),
};


EXTERN void CreateCommandTerminal(struct _LGadRoot* root, struct Rect* bounds, ulong flags);
EXTERN void DestroyCommandTerminal(void);
EXTERN void cmdterm_print(const char* text);
EXTERN void cmdterm_redraw(void);
EXTERN void cmdterm_capture_status(const char* text);
EXTERN void cmdterm_execute_command(const char* command);

#define CMDTERM_MAX_LINES 4096
#define CMDTERM_LINE_LENGTH 256
typedef void (*CmdTermFocusHandler)(const char *prefix, BOOL toggle);
EXTERN void cmdterm_set_editor_focus(CmdTermFocusHandler handler);
EXTERN int cmdterm_editor_output(char (*lines)[CMDTERM_LINE_LENGTH], unsigned *revision);
EXTERN void cmdterm_editor_execute(char *command);
EXTERN void cmdterm_editor_visible_range(int first, int count);
EXTERN const char *cmdterm_editor_history(int direction);
EXTERN const char *cmdterm_editor_complete(const char *input);
EXTERN const char *cmdterm_editor_suggestion(const char *input, int index);

#endif // __CMDTERM_H




