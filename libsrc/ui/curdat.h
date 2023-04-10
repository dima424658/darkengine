// $Header: x:/prj/tech/libsrc/ui/RCS/curdat.h 1.5 1998/06/18 13:28:17 JAEMZ Exp $

#ifndef __CURDAT_H
#define __CURDAT_H
#pragma once

#include <curtyp.h>

EXTERN int MouseLock;
EXTERN Region* CursorRegion;
EXTERN Cursor* CurrentCursor;
EXTERN Cursor* LastCursor;
EXTERN Region* LastCursorRegion;
EXTERN Point LastCursorPos;
EXTERN Rect* HideRect;
EXTERN int curhiderect;
EXTERN grs_canvas* CursorCanvas;
EXTERN grs_canvas* SecondCursorCanvas; // for stereo support
EXTERN grs_canvas DefaultCursorCanvas;
EXTERN struct _cursor_saveunder SaveUnder;

#endif
