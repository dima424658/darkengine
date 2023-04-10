/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#pragma once
#ifndef __DPCUTILS_H
#define __DPCUTILS_H

// these should be fixed to use EXTERN
extern "C" {
   #include <event.h>
}

#ifndef __RESAPI_H
#include <resapi.h>
#endif // !__RESAPI_H

#ifndef RECT_H
#include <rect.h>
#endif // !RECT_H

#define DPC_INTERFACE_PATH "iface\\"

typedef enum eDPCLoadFlags
{
   DPCLoadNormal = 0,
   DPCLoadNoPalette = 0x1,

   DPCLoadPad = 0xffffffff,
};

EXTERN void DPCUtilsInit(void);
EXTERN void DPCUtilInitColor();
EXTERN void DPCUtilsTerm(void);

EXTERN uint gDPCTextColor;

IRes *LoadPCX(const char *str, char *path = DPC_INTERFACE_PATH, eDPCLoadFlags flags = DPCLoadNormal);
EXTERN BOOL DrawByHandle(IRes *drawhand, Point pt);
EXTERN BOOL DrawByHandleCenter(IRes *drawhand, Point pt);
EXTERN BOOL DrawByHandleCenterRotate(IRes *drawhand, Point pt, fixang theta);
EXTERN BOOL DrawCursorByHandle(IRes *drawhand, Point pt);
EXTERN BOOL DrawCursorByHandleCenter(IRes *drawhand, Point pt);
EXTERN void DrawVerticalString(char *text, int x, int y, int dy);
EXTERN void SafeFreeHnd(IRes **hndPtr);
EXTERN DWORD HandleGetPix(IRes *handle, Point loc);
EXTERN bool SetCursorByHandle(IRes *hnd); // , Cursor *cursorp);
EXTERN void ClearCursor(void);
EXTERN int FindColor(int *color);

extern BOOL DPCStringFetch(char *temp,int bufsize, const char *name, const char *table,int offset = -1);

EXTERN void Pal16Callback(uchar *pal, void *bitmap);

EXTERN IRes *gCursorHnd;

struct grs_font;
EXTERN grs_font *gDPCFont;
EXTERN grs_font *gDPCFontMono;
EXTERN grs_font *gDPCFontAA;

#endif  // __DPCUTILS_H
