/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkutils.h,v 1.20 2000/01/31 09:59:32 adurant Exp $
#pragma once

#ifndef __SHKUTILS_H
#define __SHKUTILS_H

// these should be fixed to use EXTERN
extern "C" {
   #include <event.h>
}
#include <resapi.h>
#include <rect.h>

#define SHK_INTERFACE_PATH "iface\\"

typedef enum eShockLoadFlags  
{
   ShockLoadNormal = 0,
   ShockLoadNoPalette = 0x1,

   ShockLoadPad = 0xffffffff,
};

EXTERN void ShockUtilsInit(void);
EXTERN void ShockUtilInitColor();
EXTERN void ShockUtilsTerm(void);

EXTERN uint gShockTextColor;

IRes *LoadPCX(const char *str, char *path = SHK_INTERFACE_PATH, eShockLoadFlags flags = ShockLoadNormal);
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

extern BOOL ShockStringFetch(char *temp,int bufsize, const char *name, const char *table,int offset = -1);

EXTERN void Pal16Callback(uchar *pal, void *bitmap);

EXTERN IRes *gCursorHnd;

struct grs_font;
EXTERN grs_font *gShockFont;
EXTERN grs_font *gShockFontMono;
EXTERN grs_font *gShockFontAA;

#endif
