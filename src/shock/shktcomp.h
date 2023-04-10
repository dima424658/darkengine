/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shktcomp.h,v 1.2 2000/01/31 09:59:21 adurant Exp $
#pragma once

#ifndef __SHKTCOMP_H
#define __SHKTCOMP_H

EXTERN void ShockTurretInit(int which);
EXTERN void ShockTurretTerm(void);
EXTERN void ShockTurretDraw(void);
EXTERN void ShockTurretStateChange(int which);

#endif