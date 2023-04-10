/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/editor/vmwincfg.h,v 1.5 2000/01/29 13:13:28 adurant Exp $
//
// view manager window configuation controller 
#pragma once

#ifndef __VMWINCFG_H
#define __VMWINCFG_H

// if you just want to know if soloed or not
EXTERN BOOL vmSoloed;

// if you want to put the parent handler in modal state
EXTERN void vmWinCfgSetMode(int new_mode);
#define vmWINCFG_MODE_NORMAL 0
#define vmWINCFG_MODE_SCROLL 1
EXTERN int vmWinCfgGetMode(void);

// when we get a new overall parent and size - set up mouse handlers and such
EXTERN void vmWinCfgCreate(Region* parent, Rect* bounds);
EXTERN void vmWinCfgDestroy(void);

// get origin and size of a region
EXTERN BOOL vmGetRegionInfo(int i, Point *origin, Point *size);

// utilities to find out about sizes/shapes of windows at the moment
// in relation to expected values
EXTERN void vmGetReferenceSize(Point *size);
EXTERN float vmGetSizeRatio(int r, float *xrat, float *yrat);

// draw the current window config outlines
EXTERN void vmDrawWindowSkeletons(void);

// go in and out of vmSoloed mode
EXTERN void vmToggleGUIFull(void);

// layout controls - all but C_NOUPDATE do a screen update and such
EXTERN void vmWinCfgCycleLayout(int new_layout);
#define vmWINCFG_C_NOUPDATE (-1)  // special cycle with no refresh/screen update
#define vmWINCFG_CYCLE      ( 0)  // cycle to next layout
#define vmWINCFG_2by2       ( 1)  
#define vmWINCFG_1by3_VERT  ( 2)  
#define vmWINCFG_1by3_HORIZ ( 3)
#define vmWINCFG_RECENTER   ( 4)  // just recenter positional controler, leave layout form alone
#define vmWINCFG_SOLO       ( 5)  // really just a convient aux entry to vmToggleGUIFull

EXTERN int vmWinCfgGetLayout(void);

// resize the overall GUI area
EXTERN void vmWinCfgResize(Rect *bounds);

// support to scroll a camera view around
BOOL vmScrollCamera(int c);

#endif  // __VMWINCFG_H
