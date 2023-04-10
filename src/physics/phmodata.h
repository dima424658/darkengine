/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/physics/phmodata.h,v 1.3 2000/01/29 13:32:52 adurant Exp $
//
// Physics motion data header
//
#pragma once

#ifndef __PHMODATA_H
#define __PHMODATA_H

#include <matrixs.h>

#include <phmoapi.h>

typedef struct 
{
   short       submod;
   mxs_vector  vec;
} sOffset;

typedef struct
{
   ePlayerMotion  motion;
   mxs_real       duration;
   mxs_real       hold_len;

   short          numOffsets;
   sOffset        offset[3];
} sPlayerMotionData;

EXTERN sPlayerMotionData PlayerMotionTable[];

#endif // __PHMODATA_H
