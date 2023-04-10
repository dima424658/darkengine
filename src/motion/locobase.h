/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

///////////////////////////////////////////////////////////////////////////////
// $Source: r:/t2repos/thief2/src/motion/locobase.h,v $
// $Author: adurant $
// $Date: 2000/01/29 13:22:11 $
// $Revision: 1.4 $
//
// Locomotion types
#pragma once

#ifndef __LOCOBASE_H
#define __LOCOBASE_H

#if 0
/*
  Motion flags. These must be set in the editor on the frames
  on which footfalls occur.
*/
#define MP_LEFT_FOOTFALL      0x02
#define MP_RIGHT_FOOTFALL     0x04
#define MP_LEFT_FOOTUP        0x08
#define MP_RIGHT_FOOTUP       0x10
#define MP_FEET_FLAGS (MP_LEFT_FOOTFALL|MP_RIGHT_FOOTFALL|MP_LEFT_FOOTUP|MP_RIGHT_FOOTUP)
#define MP_STANDING           0x20
#define MP_TRIGGER1           0x40

#endif

// flags for footfalls
#define NEITHER_FOOT 0
#define LEFT_FOOT 1
#define RIGHT_FOOT 2

#endif // __LOCOBASE_H
