/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/engfeat/inv_hack.h,v 1.9 1998/05/14 22:52:29 dc Exp $
// inventory basics till we have real code

#pragma once
#ifndef __INV_HACK_H
#define __INV_HACK_H

#include <objtype.h>
#include <label.h>

///////////////
// interface for inventory only

// add or delete to inv, does no change to object itself, just internal inv lists
//  updates focus in inventory: add focuses on new obj, rem sets focus to -1 if o is cur focus
EXTERN BOOL invAddObj(ObjID o);
EXTERN BOOL invRemObj(ObjID o);

///////////////
// interface to world for moving objs to and from inv

// take obj from world and put into inventory
// @TODO: should probably be symmetric, ie whose inv, when inv is link chains
EXTERN BOOL invTakeObjFromWorld(ObjID o, ObjID owner);
// take object in inventory (o) and put it back in world at src (or where it was, if src is null)
EXTERN BOOL invPutBackInWorld(ObjID o, ObjID src);
// take object in inventory (o) and launch it from source (src is NECESSARY)
EXTERN BOOL invThrowBackToWorld(ObjID o, ObjID src);

// if you want to rem the object and delete it from world too
EXTERN BOOL invDelObj(ObjID o);

//////////////
// model stuff

EXTERN Label *invGetLimbModelName(ObjID o);

///////////////
// graphics/control/update

EXTERN int  g_inv_timeout; // ms the inv obj will remain up without auto-downing
EXTERN BOOL g_inv_up;      // is the inv obj currently up

// tell it a frame is done and screen is draw, update if needed
EXTERN void inv_update_frame(int ms);

// lowers display, sets inventory to null beginning
EXTERN void inv_set_to_null(void);

///////////////
// control for interface/cycling/so on

EXTERN void invHackCycle(int cycle_dir);
EXTERN void invHackDisplay(int display_code);
EXTERN BOOL invHackSetActive(BOOL active);

///////////////
// initialize the system and its commands
EXTERN void invsysInit(void);
EXTERN void invsysTerm(void);

///////////////
// hacks for ects
EXTERN void inv_clear(int destroy);
EXTERN void PostLoadInitInv(BOOL prep_and_clear);

#endif  // __INV_HACK_H
