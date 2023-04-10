/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkskill.h,v 1.4 2000/01/31 09:59:12 adurant Exp $
#pragma once

#ifndef __SHKSKILL_H
#define __SHKSKILL_H

#include <rect.h>

extern "C"
{
#include <event.h>
}

EXTERN void ShockSkillsInit(int which);
EXTERN void ShockSkillsTerm(void);
EXTERN void ShockSkillsDraw(void);
EXTERN bool ShockSkillsHandleMouse(Point pt);
EXTERN void ShockSkillsStateChange(int which);
EXTERN bool ShockSkillsCheckTransp(Point pt);

#endif