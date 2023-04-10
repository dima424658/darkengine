/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/medflbox.h,v 1.3 2000/01/29 13:12:24 adurant Exp $
#pragma once

#ifndef __MEDFLBOX_H
#define __MEDFLBOX_H

#include <gadbox.h>

EXTERN uint edit_flag;

EXTERN uint FlagBoxGetCurrentFlag();
EXTERN void FlagBoxDrawCallback(void *data, LGadBox *vb);
EXTERN void FlagBoxSetMark(int frame,uchar side);
EXTERN void FlagBoxResetMarks(int num_frames);
EXTERN bool FlagBoxAddFlag(ulong value);

#endif
