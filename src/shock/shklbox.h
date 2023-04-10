/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shklbox.h,v 1.3 2000/01/31 09:57:47 adurant Exp $
#pragma once

#ifndef __SHKLBOX_H
#define __SHKLBOX_H

extern "C" 
{
#include <event.h>
}

EXTERN void ShockLetterboxDraw(void);
EXTERN void ShockLetterboxInit(int which);
EXTERN void ShockLetterboxTerm(void);

EXTERN void ShockLetterboxSet(BOOL camera);
#endif