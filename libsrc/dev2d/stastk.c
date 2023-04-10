/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/stastk.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:08:23 $
 *
 * Public vesa vbe routines
 *
 * This file is part of the dev2d library.
 *
 */

#include <grdev.h>
#include <state.h>

#define STATE_DEPTH 5

static VideoState videoStateStack[STATE_DEPTH];
static int curTop = 0;

//
// gr_push_video_state:
//
// Routine to push the current video state onto the stack of
// saved video states, returning 0 if all ok, !0 if there was
// an error.
//

int gr_push_video_state()
{
   if (curTop == STATE_DEPTH) return 1;
// if (!SaveState(videoStateStack + curTop)) return 1;
   if (!gr_save_state(videoStateStack + curTop)) return 1;
   curTop++;
   return 0;
} // gr_push_video_state

//
// gr_pop_video_state:
//
// Routine to pop the top video state on the stack and make
// the new state equal to that one.  Returns 0 on success,
// !0 on failure.
//

int gr_pop_video_state()
{
   if (curTop == 0) return 0;
   curTop--;
// return !RestoreState(videoStateStack + curTop);
   return !gr_restore_state(videoStateStack + curTop);
} // gr_pop_video_state

