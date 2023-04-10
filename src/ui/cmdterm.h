/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/cmdterm.h,v 1.4 2000/01/31 10:04:19 adurant Exp $
#pragma once

#ifndef __CMDTERM_H
#define __CMDTERM_H

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
};


EXTERN void CreateCommandTerminal(struct _LGadRoot* root, struct Rect* bounds, ulong flags);
EXTERN void DestroyCommandTerminal(void);

#endif // __CMDTERM_H




