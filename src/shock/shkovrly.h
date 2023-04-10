/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once

#ifndef __SHKOVRLY_H
#define __SHKOVRLY_H

#include <dev2d.h>
#include <event.h>
#include <resapi.h>
#include <objtype.h>
#include <label.h>


EXTERN void ShockOverlayInit(void);
EXTERN void ShockOverlayTerm(void);
EXTERN void ShockOverlayDoFrame(void);
EXTERN void ShockOverlayChange(int which, int mode);
EXTERN void ShockOverlayChangeObj(int which, int mode, ObjID obj);
EXTERN bool ShockOverlayCheck(int which);
EXTERN void ShockOverlayAddColoredText(const char *text, ulong time, int color);
EXTERN void ShockOverlayAddText(const char *text, ulong time);
//EXTERN bool ShockOverlayHandleMouse(uiMouseEvent *mev);
EXTERN bool ShockOverlayDoubleClick(Point pos);
EXTERN bool ShockOverlayDragDrop(Point pos, BOOL start);
EXTERN bool ShockOverlayClick(Point pt);
EXTERN bool ShockOverlayHandleKey(int keycode);
EXTERN void ShockOverlaySetRect(int which, Rect r);
EXTERN Rect ShockOverlayGetRect(int which);
EXTERN bool ShockOverlayMouseOcclude(Point mpt);
EXTERN void ShockOverlaySetObj(int which, ObjID obj);
EXTERN ObjID ShockOverlayGetObj(void);
EXTERN bool ShockOverlayCheckTransp(Point pt, int overlay, IRes *art);
EXTERN void ShockOverlaySetFlags(int which, ulong flags);
EXTERN void ToggleMouseStateChange(int which);
EXTERN void ShockOverlayMouseMode(bool mode);
EXTERN void DeferOverlayClose(void *which);
EXTERN void DeferOverlayOpen(void *which);
EXTERN void DeferOverlayToggle(void *which);
EXTERN bool ShockOverlayFullTransp(Point pt);
EXTERN float ShockOverlayGetDist(int which);
EXTERN void ShockOverlaySetDist(int which, BOOL dist);
EXTERN void ShockOverlayDrawBuffers();
EXTERN void ShockOverlayComputeRestore();

EXTERN void ShockOverlayListenInit();


typedef void (*OverlayFunc)(void);
typedef void (*OverlayWhichFunc)(int);
typedef bool (*OverlayKeyFunc)(int keycode);
typedef bool (*OverlayPointFunc)(Point);
typedef bool (*OverlayPointBoolFunc)(Point,BOOL);
typedef IRes* (*OverlayBitmapFunc)(void);
typedef bool (*OverlayBoolFunc)(void);

#define OVERLAY_DIST 100.0F

typedef struct _OverlayFuncs {
   OverlayFunc m_drawfunc;
   OverlayWhichFunc m_initfunc;
   OverlayFunc m_termfunc;
   OverlayPointFunc m_mousefunc;
   OverlayPointFunc m_dclickfunc;
   OverlayPointBoolFunc m_dragdropfunc;
   OverlayKeyFunc m_keyfunc;
   OverlayBitmapFunc m_bitmapfunc;
   Label m_upschema;
   Label m_downschema;
   OverlayWhichFunc m_statefunc;
   OverlayPointFunc m_transpfunc;
   int m_distance;
   bool m_needmouse;
   int m_alpha;
   OverlayBoolFunc m_updatefunc;
   bool m_checkcontains;
} sOverlayFunc;

EXTERN void ShockOverlayAddFuncs(sOverlayFunc *pFunc, int which);

#endif // __SHKOVRLY_H
