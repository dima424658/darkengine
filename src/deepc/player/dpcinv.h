/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCINV_H
#define __DPCINV_H

extern "C" 
{
    #include <event.h>
}   

#ifndef OBJSYS_H
#include <objsys.h>
#endif // !OBJSYS_H

#ifndef ___2D_H
#include <2d.h>
#endif // !___2D_H

struct sContainDims;

EXTERN bool     DPCInvAddObj(ObjID container, ObjID obj, int noDrop = 0);
EXTERN void     DPCInvDraw(unsigned long inDeltaTicks);
extern void     DPCInvObjDraw(ObjID o, int dx, int dy);
EXTERN void     DPCInvInit(int which);
EXTERN void     DPCInvTerm(void);
EXTERN void     DPCInvStateChange(int which);
EXTERN bool     DPCInvCheckTransp(Point pt);
EXTERN bool     DPCInvHandleMouse(Point pos);
EXTERN bool     DPCInvDoubleClick(Point pos);
EXTERN bool     DPCInvDragDrop(Point pos, BOOL start);
EXTERN void     DPCInvClear(void);
EXTERN bool     DPCInvPayNanites(int quan);
EXTERN int      DPCInvNaniteTotal(void);
extern ObjID    DPCInvFindObject(Point mpos);
extern ObjID    DPCInvFindObject(int slot);
EXTERN bool     DPCInvLoadCursor(int o);
extern int      DPCInvFindObjSlot(Point dims, Point mpos, ObjID objid = OBJ_NULL);
extern int      DPCInvFindObjSlotPos(Point dims, Point mpos, Rect *pRect, ObjID objid);
extern bool     SetInvObj(int x, int y, int obj);
extern bool     SetInvObj(int i, int obj);
EXTERN bool     DPCInvFindSpace(ObjID *arr, Point dims, ObjID obj, int *slot);
EXTERN void     DPCInvFillObjPos(ObjID o, ObjID *arr, int loc, Point dims);
EXTERN void     DPCInvResize(ObjID obj, sContainDims *olddims, sContainDims *dims);
EXTERN void     DPCInvDestroyCursorObj(void);
EXTERN void     DPCInvReset(void);
EXTERN void     DPCInvRefresh(void);

EXTERN void DPCSplitCursor(void);

extern void DPCInvDrawObjArray(const Point& dims, const Point& maxsize, ObjID *objarr, const Point& startloc);
extern void DPCInvComputeObjArray(ObjID o, ObjID *arr);

EXTERN void DPCInvListenInit();

EXTERN BOOL DPCObjGetQuantity(ObjID o, char *temp);

EXTERN ObjID g_invFocusObj;

EXTERN void DPCTickerDraw(unsigned long inDeltaTicks);
EXTERN void DPCTickerInit(int which);
EXTERN void DPCTickerTerm(void);
EXTERN bool DPCTickerHandleMouse(Point pos);
#endif  // __DPCINV_H
