/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkscomp.h,v 1.2 2000/01/31 09:59:09 adurant Exp $
#pragma once

#ifndef __SHKSCOMP_H
#define __SHKSCOMP_H

#include <objtype.h>
extern "C"
{
#include <event.h>
}

EXTERN void ShockSecCompInit(int which);
EXTERN void ShockSecCompTerm(void);
EXTERN void ShockSecCompDraw(void);
EXTERN void ShockSecCompStateChange(int which);

#endif