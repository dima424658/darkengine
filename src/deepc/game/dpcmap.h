/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCMAP_H
#define __DPCMAP_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // !_OBJTYPE_H

extern "C"
{
#include <event.h>
}

EXTERN void DPCMapInit(int which);
EXTERN void DPCMapTerm(void);
EXTERN void DPCMapDraw(unsigned long inDeltaTicks);
EXTERN void DPCMapStateChange(int which);

EXTERN void DPCMiniMapInit(int which);
EXTERN void DPCMiniMapTerm(void);
EXTERN void DPCMiniMapDraw(unsigned long inDeltaTicks);
EXTERN void DPCMiniMapStateChange(int which);
EXTERN bool DPCMiniMapCheckTransp(Point pt);

EXTERN void DPCMapSetExplored(int loc, char val);
EXTERN void DPCMapDrawContents(Point dp);

#endif  // !__DPCMAP_H