/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCOVRLY_H
#define __DPCOVRLY_H

#ifndef __DEV2D_H
#include <dev2d.h>
#endif // !__DEV2D_H

#ifndef _EVENT_H
#include <event.h>
#endif // !_EVENT_H

#ifndef __RESAPI_H
#include <resapi.h>
#endif // !__RESAPI_H

#ifndef _OBJTYPE_H
#include <objtype.h>
#endif // ! _OBJTYPE_H

#ifndef __LABEL_H
#include <label.h>
#endif // !__LABEL_H

EXTERN void  DPCOverlayInit(void);
EXTERN void  DPCOverlayTerm(void);
EXTERN void  DPCOverlayDoFrame(unsigned long inDeltaTicks);
EXTERN void  DPCOverlayChange(int which, int mode);
EXTERN void  DPCOverlayChangeObj(int which, int mode, ObjID obj);
EXTERN bool  DPCOverlayCheck(int which);
EXTERN void  DPCOverlayAddColoredText(const char *text, ulong time, int color);
EXTERN void  DPCOverlayAddText(const char *text, ulong time);
EXTERN bool  DPCOverlayDoubleClick(Point pos);
EXTERN bool  DPCOverlayDragDrop(Point pos, BOOL start);
EXTERN bool  DPCOverlayClick(Point pt);
EXTERN bool  DPCOverlayHandleKey(int keycode);
EXTERN void  DPCOverlaySetRect(int which, Rect r);
EXTERN Rect  DPCOverlayGetRect(int which);
EXTERN bool  DPCOverlayMouseOcclude(Point mpt);
EXTERN void  DPCOverlaySetObj(int which, ObjID obj);
EXTERN ObjID DPCOverlayGetObj(void);
EXTERN bool  DPCOverlayCheckTransp(Point pt, int overlay, IRes *art);
EXTERN void  DPCOverlaySetFlags(int which, ulong flags);
EXTERN void  ToggleMouseStateChange(int which);
EXTERN void  DPCOverlayMouseMode(bool mode);
EXTERN void  DeferOverlayClose(void *which);
EXTERN void  DeferOverlayOpen(void *which);
EXTERN void  DeferOverlayToggle(void *which);
EXTERN bool  DPCOverlayFullTransp(Point pt);
EXTERN float DPCOverlayGetDist(int which);
EXTERN void  DPCOverlaySetDist(int which, BOOL dist);
EXTERN void  DPCOverlayDrawBuffers();
EXTERN void  DPCOverlayComputeRestore();

EXTERN void DPCOverlayListenInit();

typedef void  (*OverlayFunc)(unsigned long inDeltaTicks);
typedef void  (*OverlayWhichFunc)(int);
typedef void  (*OverlayTermFunc)(void);
typedef bool  (*OverlayKeyFunc)(int keycode);
typedef bool  (*OverlayPointFunc)(Point);
typedef bool  (*OverlayPointBoolFunc)(Point,BOOL);
typedef IRes* (*OverlayBitmapFunc)(void);
typedef bool  (*OverlayBoolFunc)(void);

#define OVERLAY_DIST 100.0F

typedef struct _OverlayFuncs 
{
   OverlayFunc          m_drawfunc;
   OverlayWhichFunc     m_initfunc;
   OverlayTermFunc      m_termfunc;
   OverlayPointFunc     m_mousefunc;
   OverlayPointFunc     m_dclickfunc;
   OverlayPointBoolFunc m_dragdropfunc;
   OverlayKeyFunc       m_keyfunc;
   OverlayBitmapFunc    m_bitmapfunc;
   Label                m_upschema;
   Label                m_downschema;
   OverlayWhichFunc     m_statefunc;
   OverlayPointFunc     m_transpfunc;
   int                  m_distance;
   bool                 m_needmouse;
   int                  m_alpha;
   OverlayBoolFunc      m_updatefunc;
   bool                 m_checkcontains;
} sOverlayFunc;

EXTERN void DPCOverlayAddFuncs(sOverlayFunc *pFunc, int which);

#endif // __DPCOVRLY_H
