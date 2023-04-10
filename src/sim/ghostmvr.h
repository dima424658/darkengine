/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/ghostmvr.h,v 1.3 1999/08/05 18:11:11 Justin Exp $

#ifndef __GHOSTMVR_H
#pragma once
#define __GHOSTMVR_H

// used by MVR to figure out what is up
EXTERN BOOL IsLocalGhost(ObjID obj);
EXTERN BOOL IsRemoteGhost(ObjID obj);

typedef enum 
{
   kGT_None   = 0,
   kGT_Local  = (1<<0),
   kGT_Remote = (1<<1),
   kGT_Player = (1<<2),
   kGT_AI     = (1<<3),
   kGT_Object = (1<<4),
} eGhostType;

// this should really be in a sound header somewhere
const int kGhostListenTypes = (kGT_Remote | kGT_Player);

EXTERN int GetGhostType(ObjID obj);

// used by motion system to ask what motion number it should be using
EXTERN int GetGhostMotionNumber(void);

// for passing a mocap from the mvr code back to ghost, so ghost can play it...
EXTERN void GhostSendMoCap(ObjID ghost, int schema, int motion, BOOL is_gloco);

// does the ghost currently want to be floored?
EXTERN BOOL GhostIsFloored(ObjID ghost);

#endif
