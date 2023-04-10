/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCCONT_H
#define __DPCCONT_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // _OBJTYPE_H
extern "C"
{
#include <event.h>
}

EXTERN void  DPCContainerInit(int which);
EXTERN void  DPCContainerTerm(void);
EXTERN void  DPCContainerDraw(unsigned long inDeltaTicks);
EXTERN bool  DPCContainerHandleMouse(Point pt);
EXTERN bool  DPCContainerDragDrop(Point pt, BOOL start);
EXTERN void  DPCContainerStateChange(int which);
EXTERN void  DPCContainerOpen(ObjID o);
EXTERN bool  DPCContainerCheckTransp(Point pt);
EXTERN ObjID DPCContainerFindObj(Point mpos, int *slotp = NULL);
EXTERN int   DPCContainerFindObjSlot(Point mpos);
EXTERN void  DPCContainerCheckRemove(ObjID o, ObjID cont);

#endif  // __DPCCONT_H
