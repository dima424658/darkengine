/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/sim/bow.h,v 1.5 2000/01/29 13:40:58 adurant Exp $
#pragma once

#ifndef __BOW_H
#define __BOW_H
                        
#include <objtype.h>
#include <camera.h>

typedef enum
{
   BS_REST,
   BS_DRAWN,
   BS_FIRED,
} eBowModes;


EXTERN void BowCreate();
EXTERN void BowDestroy();
// camera through which bow is viewed
EXTERN void BowUpdate(Camera *cam);
EXTERN ulong BowStateSetMode(int mode, ulong transTime, ulong startDelta);

// debugging funcs
EXTERN void BowToggle();
EXTERN void BowGoMode(int mode);

EXTERN ObjID g_BowObjID;
#endif

