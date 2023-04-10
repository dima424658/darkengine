/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/brlist.h,v 1.8 2000/01/29 13:11:13 adurant Exp $
#pragma once

#ifndef __BRLIST_H
#define __BRLIST_H

#include <editbrs.h>

//
// Brush List (brlist.h)
//   stores the actual sequential brush list.  has calls to insert a brush
// into the list, to delete a brush from the list, to set the brush list
// cursor position, return current brush, and other low level brush needs. 
//     - define seek_types - SEEK_SET, SEEK_CUR, SEEK_END
//     * brush_list *blistAlloc(int size_guess), blistFree(brush_list *bl)
//         allocate and free the master brush list
//     * brush_list *blistSetCurrent(brush_list *bl)
//         sets the current used brush list for the system
//     * bool blistSeek(int pos, int seek_type)
//         set blist cursor to pos, uses fseek style SEEK types
//     * int blistInsert(editBrush *new), editBrush *blistDelete()
//         works on the current cursor position
//     * editBrush *blistGet()
//         retrieves pointer to the current brush
//     * int blistCheck(editBrush *chk)
//         returns where the brush is in the blist, or -1 if not there
//     * int blistSetPostoBrush(editBrush *targ)
//         sets the blist cursor to point at targ, returns pos or -1
//         (ie. this really just calls check then seek_set)
// 
// note this basically uses the dstruct lib llist 
//
// really need to break this up

typedef struct _brush_list brush_list;

// attempt to allocate the master brush list, return NULL if you fail
EXTERN brush_list *blistAlloc(int size_guess);

// free a brush list and all associated brushes
EXTERN bool blistFree(brush_list *bl);

// set the current brush list (implicit arg to all blist functions
EXTERN void blistSetCurrent(brush_list *bl);

// set blist cursor to pos, uses fseek style SEEK types, false if out of range
EXTERN bool blistSeek(int pos, int seek_type);

// insert to the current brush list position
EXTERN int blistInsert(editBrush *newbr);

// removes from the current brush list position
EXTERN editBrush *blistDelete(void);

// retrieves pointer to brush structure for the current brush list pos
EXTERN editBrush *blistGet(void);

// returns where the brush is in the blist, or -1 if not there
EXTERN int blistCheck(editBrush *chk);

// sets the blist cursor to point at targ, returns pos or -1
//         (ie. this really just calls check then seek_set)
EXTERN int blistSetPostoBrush(editBrush *targ);

EXTERN void (*blist_new_cursor_func)(struct _editBrush* ); // call me whenever the cursor changes

// take a wild guess
EXTERN int blistCount(void);

// return order in list of cursor
EXTERN int blistGetPos(void);

///////////////////////
// iteration

// dont screw with the list while calling these, or panic
// the idea is, you call IterStart to setup and get a handle back, then IterNext to continue
// IterDone says "im done with the handle"
// NOTE: YOU MUST USE DIFFERENT STARTS HANDLES IN A STACK FASHION!!!
// ie you can IterStart within a different Iter, but you MUST FINISH the new Iter and "Done" it
//   before IterNext'ing the old one

// start an iteration, get start of list and handle
EXTERN editBrush *blistIterStart(int *iter_handle);
// continue to iterate on this handle (MUST BE SAME AS LAST IterStart HANDLE, or popped handle from Done)
// NOTE: auto-pops when it reaches the end!!! ie. it calls IterDone
EXTERN editBrush *blistIterNext(int iter_handle);
// finish an Iter, returns handle of previous ("popped") iteration
//  (in case you need to just abort everything??? you could just keep IterDone'ing its own return values
//   im not actually sure this is a good idea, but i dont see why it wouldnt work)
EXTERN int        blistIterDone(int iter_handle);

/////////////////
// debugging tool, dumps brush list to mono screen
EXTERN void blistDump(void);

//////////////////////
// for the seek parameters - you probably already have these in stdio.h or something
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

#ifndef SEEK_END
#define SEEK_END 2
#endif

#endif  // __BRLIST_H
