/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktechs.h,v 1.2 2000/01/31 09:59:23 adurant Exp $
#pragma once

#ifndef __SHKTECHS_H
#define __SHKTECHS_H

#include <rect.h>

EXTERN void ShockTechSkillInit(int which);
EXTERN void ShockTechSkillTerm(void);
EXTERN void ShockTechSkillDraw(void);
EXTERN bool ShockTechSkillHandleMouse(Point pt);
EXTERN void ShockTechSkillStateChange(int which);
EXTERN bool ShockTechSkillCheckTransp(Point pt);

#endif