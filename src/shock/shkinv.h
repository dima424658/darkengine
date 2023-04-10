/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header $
#pragma once

#ifndef __SHKINV_H
#define __SHKINV_H

extern "C" 
{
#include <event.h>
}   
#include <objsys.h>
#include <2d.h>

struct sContainDims;

EXTERN bool ShockInvAddObj(ObjID container, ObjID obj);
EXTERN void ShockInvDraw(void);
extern void ShockInvObjDraw(ObjID o, int dx, int dy);
EXTERN void ShockInvInit(int which);
EXTERN void ShockInvTerm(void);
EXTERN void ShockInvStateChange(int which);
EXTERN bool ShockInvCheckTransp(Point pt);
EXTERN bool ShockInvHandleMouse(Point pos); // uiMouseEvent *mev);
EXTERN bool ShockInvDoubleClick(Point pos);
EXTERN bool ShockInvDragDrop(Point pos, BOOL start);
EXTERN void ShockInvClear(void);
EXTERN bool ShockInvPayNanites(int quan);
EXTERN int ShockInvNaniteTotal(void);
extern ObjID ShockInvFindObject(Point mpos);
extern ObjID ShockInvFindObject(int slot);
EXTERN bool ShockInvLoadCursor(int o);
extern int ShockInvFindObjSlot(Point dims, Point mpos, ObjID objid = OBJ_NULL);
extern int ShockInvFindObjSlotPos(Point dims, Point mpos, Rect *pRect, ObjID objid);
extern bool SetInvObj(int x, int y, int obj);
extern bool SetInvObj(int i, int obj);
EXTERN bool ShockInvFindSpace(ObjID *arr, Point dims, ObjID obj, int *slot);
EXTERN void ShockInvFillObjPos(ObjID o, ObjID *arr, int loc, Point dims);
EXTERN void ShockInvResize(ObjID obj, sContainDims *olddims, sContainDims *dims);
EXTERN void ShockInvDestroyCursorObj(void);
EXTERN void ShockInvReset(void);
EXTERN void ShockInvRefresh(void);

//EXTERN int ShockInvQuickbind(void);

EXTERN void ShockSplitCursor(void);

extern void ShockInvDrawObjArray(Point dims, Point maxsize, ObjID *objarr, Point startloc);
extern void ShockInvComputeObjArray(ObjID o, ObjID *arr);

EXTERN void ShockInvListenInit();

EXTERN BOOL ShockObjGetQuantity(ObjID o, char *temp);

EXTERN ObjID g_invFocusObj;

EXTERN void ShockTickerDraw(void);
EXTERN void ShockTickerInit(int which);
EXTERN void ShockTickerTerm(void);
EXTERN bool ShockTickerHandleMouse(Point pos);
#endif