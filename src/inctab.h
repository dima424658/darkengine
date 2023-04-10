/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/inctab.h,v 1.2 2000/01/29 12:41:31 adurant Exp $
#pragma once

#ifndef INCTAB__H
#define INCTAB__H

#define INC_TAB_INT 1
#define INC_TAB_FLOAT 2

EXTERN void IncTabsInit();
EXTERN void IncTabsShutdown();
EXTERN void IncTabAddInt(char *define, int val);
EXTERN void IncTabAddFloat(char *define, float val);
EXTERN BOOL IncTabLookupInt(char *define, int *val);
EXTERN BOOL IncTabLookupFloat(char *define, float *val);
EXTERN int IncTabLookup(char *define, void *val);

#endif
