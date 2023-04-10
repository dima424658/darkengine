/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/ui/bugterm.h,v 1.4 2000/01/29 13:42:10 adurant Exp $
#pragma once

#ifndef __BUGTERM_H
#define __BUGTERM_H

#ifdef PLAYTEST

//
// Bug terminal flags
//

enum BugTermFlags
{
   kBugTermNoFlags = 0, 
   kBugTermHideUnfocused = (1 << 0), 
   kBugTermBeginFocused = (1 << 1), 
};

EXTERN void CreateBugTerminal(struct _LGadRoot* root, struct Rect* bounds, ulong flags);
EXTERN void DestroyBugTerminal(void);

#else

#define CreateBugTerminal(root,bounds,flags)
#define DestroyBugTerminal()

#endif

#endif // __CMDTERM_H




