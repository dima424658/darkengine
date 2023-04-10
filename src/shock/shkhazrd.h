/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkhazrd.h,v 1.3 2000/01/31 09:56:36 adurant Exp $
#pragma once

#ifndef __SHKHAZRD_H
#define __SHKHAZRD_H

extern "C" 
{
#include <event.h>
}

EXTERN void ShockRadDraw(void);
EXTERN void ShockRadInit(int which);
EXTERN void ShockRadTerm(void);

EXTERN void ShockPoisonDraw(void);
EXTERN void ShockPoisonInit(int which);
EXTERN void ShockPoisonTerm(void);
#endif