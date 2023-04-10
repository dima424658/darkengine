/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/scrnovls.h,v 1.2 1998/01/14 06:20:36 dc Exp $

#pragma once
#ifndef __SCRNOVLS_H
#define __SCRNOVLS_H

#define kScreenOverlaysOff 0
#define kScreenOverlaysOn 1
#define kScreenOverlaysToggle 2

EXTERN void ScreenOverlaysInit(void);
EXTERN void ScreenOverlaysFree(void);
EXTERN void ScreenOverlaysUpdate(void);
EXTERN void ScreenOverlaysChange(int which, int mode);

// how about a way to know what is going on?
EXTERN BOOL ScreenOverlayCurrentlyUp(int which);

#define SCRNOVLS_MAX_ELEM 6

#endif // __SCRNOVLS_H
