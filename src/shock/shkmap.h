/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkmap.h,v 1.4 2000/01/31 09:57:54 adurant Exp $
#pragma once

#ifndef __SHKMAP_H
#define __SHKMAP_H

#include <objtype.h>
extern "C"
{
#include <event.h>
}

EXTERN void ShockMapInit(int which);
EXTERN void ShockMapTerm(void);
EXTERN void ShockMapDraw(void);
EXTERN void ShockMapStateChange(int which);

EXTERN void ShockMiniMapInit(int which);
EXTERN void ShockMiniMapTerm(void);
EXTERN void ShockMiniMapDraw(void);
EXTERN void ShockMiniMapStateChange(int which);
EXTERN bool ShockMiniMapCheckTransp(Point pt);

EXTERN void ShockMapSetExplored(int loc, char val);
EXTERN void ShockMapDrawContents(Point dp);

#endif