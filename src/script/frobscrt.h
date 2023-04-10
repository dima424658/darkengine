/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/script/frobscrt.h,v 1.6 1998/09/24 22:39:06 mahk Exp $
// internal definitions

#pragma once

#ifndef __FROBSCRT_H
#define __FROBSCRT_H


enum eFrobFlags
{
   kFrobMove         = 1 << 0,
   kFrobScript       = 1 << 1, 
   kFrobDelete       = 1 << 2,
   kFrobIgnore       = 1 << 3,
   kFrobFocusScript  = 1 << 4,
   kFrobToolCursor   = 1 << 5, 
   kFrobUseAmmo      = 1 << 6, // decrement stack count, delete if necessary
   kFrobDefault      = 1 << 7, // Do A/R prop, etc
   kFrobDeselect     = 1 << 8, // de-select 
};

// how to tell frob where something is
typedef enum {
   kFrobLocWorld=0,
   kFrobLocInv,
   kFrobLocTool,      // only for message info
   kFrobLocNone,      // only for empty/nonexistent messages
   kFrobNumActions = kFrobLocTool + 1,


   kFrobLocSpaceWasterJoy=0xffffffff,
} eFrobLoc;

typedef enum eFrobLev {
   kFrobLevFocus,
   kFrobLevSelect,
   kFrobLevSpaceWasterJoy=0xffffffff,
} eFrobLev;


// actions on frobs
typedef struct sFrobInfo {
   ulong actions[kFrobNumActions];  // what happens in a frob of this
   ulong pad[4-kFrobNumActions];        // pad space
} sFrobInfo;

#endif  // __FROBSCRT_H


