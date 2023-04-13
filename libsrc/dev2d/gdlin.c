/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/gdlin.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:41:01 $
 *
 * Global stuff.
 *
 * This file is part of the dev2d library.
 */

#include <clip2d.h>
#include <gdlin.h>

int gd_hline_opt(int x, int y, int x1, gdulin_func *f)
{
   int clip=CLIP_NONE;

   if ((y<grd_clip.top)||(y>=grd_clip.bot)||
       (x>=grd_clip.right)||(x1<grd_clip.left))
      return CLIP_ALL;
   if (x<grd_clip.left)
      x = grd_clip.left, clip=CLIP_LEFT;
   if (x1>=grd_clip.right)
      x1 = grd_clip.right-1, clip=CLIP_RIGHT;
   f(x, y, x1);
   return clip;
}

int gd_vline_opt(int x, int y, int y1, gdulin_func *f)
{
   int clip=CLIP_NONE;

   if ((y1<grd_clip.top)||(y>=grd_clip.bot)||
       (x<grd_clip.left)||(x>=grd_clip.right))
      return CLIP_ALL;
   if (y<grd_clip.top)
      y = grd_clip.top, clip=CLIP_TOP;
   if (y1>=grd_clip.bot)
      y1 = grd_clip.bot-1, clip=CLIP_BOT;
   f(x, y, y1);
   return clip;
}

int gd_hline(int x, int y, int x1)
{
   return gd_hline_opt(x, y, x1, gd_uhline_expose(0,0,0));
}

int gd_vline(int x, int y, int y1)
{
   return gd_vline_opt(x, y, y1, gd_uvline_expose(0,0,0));
}
