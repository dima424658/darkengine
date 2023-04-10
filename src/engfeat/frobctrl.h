/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/frobctrl.h,v 1.12 2000/01/29 13:19:34 adurant Exp $
// frob control system
#pragma once

#ifndef __FROBCTRL_H
#define __FROBCTRL_H

#include <objtype.h>
#include <frobscrt.h>

// all data needed to execute a frob
typedef struct {
   int      ms_down; // ms for which frob was "down"
   ObjID    src_obj; // src (tool) or obj for solo
   ObjID    dst_obj; // dst (receive) or NULL for solo
   eFrobLoc src_loc; // where tool or obj is
   eFrobLoc dst_loc; // where dst is (if there is one)
   ObjID    frobber; // who did the frobbing
   ulong    flags;   // any interesting flags about this frob
} sFrobActivate;

// Flags that go in that structure:
#define FROBFLG_RETURN_RECEIPT 0x01  // The frobber wants to be notified
                                     // when the frob completes, via
                                     // frobAcknowledge()

// would frobbing this object pick it up from the world?
EXTERN BOOL FrobWouldPickup(sFrobActivate *situation);
// actually "do the deed"
EXTERN void FrobExecute(sFrobActivate *situation);
// do a down followed by and up of ms_down ms
EXTERN void FrobInstantExecute(sFrobActivate *situation);
// halt the current frob, pass NULL to just use last of type loc... (loc None for any)
EXTERN void FrobAbort(eFrobLoc loc, sFrobActivate *abort_situation);

// set the current loc/lev frob obj
EXTERN void FrobSetCurrent(eFrobLoc loc, eFrobLev lev, ObjID obj);

// Send script message
EXTERN void FrobSendInvSelectionMsg(eFrobLoc loc, eFrobLev lev, ObjID old_obj, ObjID obj);

// check to see if this is a valid tool frob
EXTERN BOOL IsToolFrob(ObjID tool, ObjID target);
EXTERN BOOL IsTool(ObjID obj);

// Prepare and shutdown Frobbing
EXTERN void       FrobInit(void);
EXTERN void       FrobTerm(void);

////////////////
// global frob variables

// selected Objs are the current selection
EXTERN ObjID frobWorldSelectObj;
EXTERN ObjID frobInvSelectObj;

// focused objects are for things like cursors and such
EXTERN ObjID frobWorldFocusObj;
EXTERN ObjID frobInvFocusObj;
// NOTE: in games which dont have inv->inv and world->world, these will
//   probably never be used, since one might as well move the select as
//   cursor, since there is only one active object per system
EXTERN BOOL (*frobInvRemObj)(ObjID o);
EXTERN BOOL (*frobInvTakeObjFromWorld)(ObjID o, ObjID owner);
EXTERN BOOL (*frobInvThrowBackToWorld)(ObjID o, ObjID src);
EXTERN BOOL (*frobInvToolCursor)(ObjID o);
EXTERN BOOL (*frobOnSetCurrent)(eFrobLoc loc, eFrobLev lev, ObjID newobj, ObjID oldobj);
EXTERN BOOL (*frobInvDeselect)(eFrobLoc loc, ObjID obj, ObjID frobber);
// Notification when a frob completes. Note that FrobInstantExecute will
// send *two* acknowledgements.
EXTERN void (*frobAcknowledge)(ObjID o);

#endif  // __FROBCTRL_H
