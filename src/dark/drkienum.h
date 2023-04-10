/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/drkienum.h,v 1.1 1998/04/23 15:15:53 dc Exp $
// enums for dark inventory

#pragma once
#ifndef __DRKIENUM_H
#define __DRKIENUM_H

typedef enum {
   kDrkInvCapCycle,
   kDrkInvCapWorldFrob,
   kDrkInvCapWorldFocus,
   kDrkInvCapInvFrob,
   kDrkInvCapIntMax=0xffffffff,
} eDrkInvCap;

typedef enum {
   kDrkInvControlOn,
   kDrkInvControlOff,
   kDrkInvControlToggle,
   kDrkInvControlIntMax=0xffffffff,
} eDrkInvControl;

#endif  // __DRKIENUM_H
