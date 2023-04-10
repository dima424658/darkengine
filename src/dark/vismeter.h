/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/dark/vismeter.h,v 1.2 1998/06/23 13:38:58 mahk Exp $
#pragma once  
#ifndef __VISMETER_H
#define __VISMETER_H
#include <objtype.h>
#include <rect.h>

////////////////////////////////////////////////////////////
//
// PLAYER VISIBILITY METER
//

//
// Game mode prep/unprep
//

EXTERN void VisMeterEnterMode(void);
EXTERN void VisMeterExitMode(void); 

// update/draw vis meter in a rectangle of the current canvas 
EXTERN void VisMeterUpdate(ulong time,const Rect* screen_r); 

// set cutoffs between models
EXTERN void VisMeterSetCutoffs(const int* cutoffs, int n); 

#endif // __VISMETER_H
