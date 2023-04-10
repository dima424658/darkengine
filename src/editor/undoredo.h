/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/undoredo.h,v 1.3 1998/08/08 16:52:27 dc Exp $

#pragma once

#ifndef __UNDOREDO_H
#define __UNDOREDO_H

EXTERN void  undoStackInit(void);
EXTERN void  undoStackFree(void);
EXTERN BOOL  undoStoreAction(void *action);
EXTERN BOOL  undoUndoReplace(void *ptr);
EXTERN BOOL  undoRedoReplace(void *ptr);
EXTERN void *undoPeek(void);
EXTERN void *undoDoUndo(void);
EXTERN void *undoDoRedo(void);
EXTERN void  undoClearUndoStack(void);
EXTERN void  undoKillStackTop(void);

#ifdef DBG_ON
EXTERN void undoDumpStack(void (*cback)(void *action,bool next));
#endif

#endif  // __UNDOREDO_H
