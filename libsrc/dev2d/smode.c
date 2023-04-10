/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/smode.c $
 * $Revision: 1.7 $
 * $Author: KEVIN $
 * $Date: 1998/01/13 12:43:01 $
 *
 * generic set mode.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <grdev.h>
#include <mode.h>

#define fix_three_fourths fix_make(0, 0xc000)

void (*grd_set_mode_callback)()=NULL;

int gr_set_mode(int mode, uint flags)
{
   short *mode_ptr;

   mode_ptr = grd_info.modes;
   do {
      if (*mode_ptr == -1)
         return -1;
   } while (*mode_ptr++ != mode);

// @TBD: kevin 10/4/96
// under com, set_screen_mode may change grd_mode behind our backs,
// if we don't get what we asked for.
// com also sets grd_cap->w and h independently.
// Eventually, all this crap will only be done through IDisplayDevice
// and then life will be better, but for now, things are somewhat perilous...

   grd_mode = mode;
   grd_cap->w = grd_mode_info[mode].w;
   grd_cap->h = grd_mode_info[mode].h;
   gr_set_screen_mode(mode, flags);
   gr_init_device(&grd_info);
   grd_cap->aspect = (grd_cap->w * fix_three_fourths)/grd_cap->h;
   if (grd_set_mode_callback!=NULL)
      grd_set_mode_callback();
   return 0;
}


// Finds the first 2d mode with w,h, and bitdepth
int gr_find_mode(int w, int h, int bitDepth)
{
   return gr_find_mode_flags_list(w,h,bitDepth,0,grd_info.modes);
}

int gr_find_mode_list(int w, int h, int bitDepth, short *mode_ptr)
{
   return gr_find_mode_flags_list(w,h,bitDepth,0,mode_ptr);
}

int gr_find_mode_flags(int w, int h, int bitDepth, uchar flags)
{
   return gr_find_mode_flags_list(w, h, bitDepth, flags, grd_info.modes);
}

// More sophisticated version of above, let's you look for specific
// mode flags like flat and linear
int gr_find_mode_flags_list(int w, int h, int bitDepth, uchar flags, short *mode_ptr)
{
   grs_mode_info *info;

   while (*mode_ptr != -1) {
      info = &grd_mode_info[*mode_ptr];

      if (info->w==w && info->h==h && info->bitDepth==bitDepth && (info->flags&flags)==flags)
         break;

      mode_ptr++;
   }

   return *mode_ptr;
}

