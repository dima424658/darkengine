/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkrsrch.h,v 1.6 2000/01/31 09:59:07 adurant Exp $
#pragma once

#ifndef __SHKRSRCH_H
#define __SHKRSRCH_H

extern "C"
{
#include <event.h>
}
#include <objtype.h>

EXTERN void ShockResearchInit(int which);
EXTERN void ShockResearchTerm(void);
EXTERN void ShockResearchDraw(void);
EXTERN bool ShockResearchHandleMouse(Point pt);
EXTERN void ShockResearchStateChange(int which);
//EXTERN bool ShockResearchCheckTransp(Point pt);
EXTERN void ShockResearch(ObjID o);
EXTERN void ShockResearchLoop();
EXTERN void ShockResearchCheckObj(ObjID o);

EXTERN int GetResearchQB(ObjID o, char *suffix);
EXTERN int GetResearchQBArch(ObjID arch, char *suffix);

// attempt to eat chemical obj for use in research
EXTERN BOOL ShockResearchConsume(ObjID obj);

// Network setup, which must happen at app init
EXTERN void ShockResearchNetInit(void);
EXTERN void ShockResearchNetTerm(void);
#endif
