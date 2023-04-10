/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/fastype.h,v 1.2 2000/01/29 13:38:48 adurant Exp $
#pragma once

#ifndef __FASTYPE_H
#define __FASTYPE_H

#include <fas.h>   // get the typedefs

struct FrameAnimationConfig
{
   float fps;        // 0 means doesn't animate
   bool clamp;       // don't wrap, stop after one play
   bool bounce;      // play back-n-forth; if both set, back-n-forth once
   bool frame_limit; // don't allow it to advance by more than 1 per call
   char client_data; // clients can use this for whatever they want

   // future expansion may occur here
};

// all this data is private and you should never try to set it
// directly--use one of the set functions in fas.h
struct FrameAnimationState
{
   ulong start_time;   // times are in ms
   ulong next_time;
   ulong cur_frame;
   ulong reserved;

   // future expansion may occur here
};

#endif
