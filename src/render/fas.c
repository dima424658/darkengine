/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/fas.c,v 1.3 2000/02/19 12:35:19 toml Exp $

#include <lg.h>
#include <mprintf.h>
#include <fas.h>
#include <fastype.h>
#include <simtime.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

#define TICKS_PER_SECOND   1000.0
#define MAX_TIME           0x7fffffff

// compute and return the current of the animation
int FrameAnimationGetFrame(FrameAnimationState *state,
     FrameAnimationConfig *cfg, int num_frames)
{
   ulong cur_time = GetSimTime();

   if (!cfg)
   {
      if (state)
         return state->cur_frame;
      else
         return 0;
   } else if (!state)
      return 0;
   
   if (cur_time >= state->next_time)
   {
      ulong delta_time = cur_time - state->start_time;
      float abstract_frame = delta_time/TICKS_PER_SECOND * cfg->fps;
      int frame_count;
      ulong frame;

      if (cfg->bounce && num_frames > 1)
         frame_count = (num_frames * 2) - 2; // doesn't work for count=1
      else
         frame_count = num_frames;

      // now, the integer part of abstract frame is the actual frame #;
      // the fractional remainder tells us how much time is left

      frame = (ulong) abstract_frame;
      if (cfg->fps)
         state->next_time = cur_time +
                TICKS_PER_SECOND*(frame+1-abstract_frame)/cfg->fps;
      else
         state->next_time = MAX_TIME;

      if (frame_count <= 0) return 0; // avoid mod by 0 or negative

      if (cfg->clamp && frame >= frame_count)
      {
         frame = frame_count-1;
         state->next_time = MAX_TIME;  // we'll never update again
      } else
         frame %= frame_count;

      if (cfg->frame_limit && frame > state->cur_frame + 1)
         frame = state->cur_frame+1;

      state->cur_frame = frame;
   }

   // if we're bouncing, our frame number might exceed the real one
   if (state->cur_frame >= num_frames)
      return num_frames - 2 - (state->cur_frame-num_frames);
   else
      return state->cur_frame;
}

// @TODO: make this actually be smart, ie. set a variable
BOOL FrameAnimationIsDone(FrameAnimationState *state)
{
   return state->next_time==MAX_TIME;
}

void FrameAnimationStateStart(FrameAnimationState *fas)
{
   fas->start_time = GetSimTime();
   fas->next_time  = fas->start_time+1; // recompute after this frame
   fas->cur_frame  = 0;
   fas->reserved   = 0;
}

void FrameAnimationStateBackdateSetFrame(FrameAnimationState *fas,
                FrameAnimationConfig *cfg, int frame, ulong tm)
{
   // force it to be on frame tm at time tm

   fas->start_time = tm - cfg->fps * frame * TICKS_PER_SECOND;
   if (fas->start_time > tm) // WRAPPED
      fas->start_time = 0;
   fas->next_time = cfg->fps ? 0 : MAX_TIME;
   fas->cur_frame = frame;
   fas->reserved = 0;
}

void FrameAnimationStateBackdateStart(FrameAnimationState *fas,
                FrameAnimationConfig *cfg, ulong tm)
{
   FrameAnimationStateBackdateSetFrame(fas, cfg, 0, tm);
}

void FrameAnimationStateSetFrame(FrameAnimationState *fas,
                             FrameAnimationConfig *cfg, int cur_frame)
{
   FrameAnimationStateBackdateSetFrame(fas, cfg, cur_frame, GetSimTime());
}

// if you want to change the animation without restarting it, call this:
void FrameAnimationChange(FrameAnimationState  *state,
                          FrameAnimationConfig *old_cfg, int old_frames,
                          FrameAnimationConfig *new_cfg, int new_frames)
{
   ulong cur_time = GetSimTime();

   // compute current frame number, suppressing bounce
   bool prev_bounce = old_cfg->bounce;
   if (prev_bounce && old_frames > 1)
      old_frames = 2*old_frames-2;

   // cache the current frame number & next_time into state
   old_cfg->bounce = FALSE;
   FrameAnimationGetFrame(state, old_cfg, old_frames);   
   old_cfg->bounce = prev_bounce;

   if (new_cfg->bounce && new_frames > 1)
      new_frames = 2*new_frames-2;

   // clamp if necessary (i.e. if clamping, or if just too big 'cause
   // they have different number of frames)
   if (state->cur_frame >= new_frames)
      state->cur_frame = new_frames-1;

   // finally, we want to force this frame slightly back in time,
   // depending on how much time has passed on this one so far...
   // the time on this one can be gotten from next time

   if (state->next_time != 0 && state->next_time != MAX_TIME) {
      ulong time_left = state->next_time - cur_time;

      // now convert the time to fractions of a frame
      float frame_left = time_left * old_cfg->fps / TICKS_PER_SECOND;

      // now use 1-frame_left as "frame_passed", and move it back in time
      if (new_cfg->fps)
         cur_time -= TICKS_PER_SECOND*(1-frame_left)/new_cfg->fps;
   }
   FrameAnimationStateBackdateSetFrame(state, new_cfg,
                                      state->cur_frame, cur_time);
}
