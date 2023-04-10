/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brundo.h,v 1.5 2000/01/29 13:11:19 adurant Exp $
// brush specific undo and redo
#pragma once

#ifndef __BRUNDO_H
#define __BRUNDO_H

#include <editbrs.h>

// start saves off current brush state
EXTERN void editUndoStoreStart(editBrush *us);
EXTERN void editUndoStoreBrush(editBrush *us);
EXTERN void editUndoStoreDelete(editBrush *us);
EXTERN void editUndoStoreCreate(editBrush *us);

// this is used to store off block starts and stops
// so that you can treat a set of undo's as a block
// and a single call to undo/redo will do all of them
EXTERN void editUndoStoreBlock(BOOL start_of_block);

// as we get more specific and hateful
EXTERN void editUndoStoreGroup(int old_group, int new_group);

// if you want to do stuff undo/redo cant see/deal with
EXTERN void editUndoSuspend(BOOL suspend);

// actually go do the undo and redo
EXTERN bool editUndoDoUndo(void);
EXTERN bool editUndoDoRedo(void);

#ifdef DBG_ON
EXTERN void editUndoStackPrint(void);
#endif

#endif  // __BRUNDO_H
