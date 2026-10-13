/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/_animtxt.h,v 1.3 2000/01/29 12:41:54 adurant Exp $
//
// Internals for anim_txt.c
// Published here just so resapp.h can see the structure definition...
#pragma once

#ifndef ___ANIMTXT_H
#define ___ANIMTXT_H

#include <darklimit.h>

// Frame zero is the base image; NewDark accepts numbered frames through 99.
#define MAX_FRAMES DARK_LIMIT_RUNTIME_TEXTURE_ANIMATION_SLOTS

typedef struct {
   void  *animsys;
   IRes  *frame_hnd[MAX_FRAMES];
   short flags;
   short cnt;
   int   lock_cnt;
   int   cur;
   int   txt_id;
   int   last_time;
   int   rate;
} ectsAnimInfo;

#endif // ___ANIMTXT_H
