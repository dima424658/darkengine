/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkiftul.h,v 1.4 2000/01/31 09:56:44 adurant Exp $
#pragma once

#ifndef __SHKIFTUL_H
#define __SHKIFTUL_H

EXTERN void ShockMFDNavButtonsInit();
EXTERN void ShockMFDNavButtonsTerm();
EXTERN void ShockMFDNavButtonsBuild(Rect *r);
EXTERN void ShockMFDNavButtonsDestroy();
EXTERN void ShockMFDNavButtonsDraw();

EXTERN void SetLeftMFDRect(int which, Rect full_rect);
EXTERN void SetRightMFDRect(int which, Rect full_rect);

#endif
