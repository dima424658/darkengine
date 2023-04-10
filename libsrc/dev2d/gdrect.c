/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/gdrect.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:41:34 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <clip.h>
#include <gdlin.h>

void gdi_box(int x, int y, int x1, int y1,
      gdulin_func *hlin_func, gdulin_func *vlin_func)
{
   if (y1<y)
      return;
   if (x<x1) {
      hlin_func(x, y, x1);
      if (y<y1) {
         hlin_func(x, y1, x1);
         if (y+1<y1) {
            vlin_func(x, y+1, y1-1);
            vlin_func(x1, y+1, y1-1);
         }
      }
      return;
   }
   if (x==x1)
      vlin_func(x, y, y1);
}

void gd_ubox(int x, int y, int x1, int y1)
{
   gdi_box(x, y, x1-1, y1-1,
      gd_uhline_expose(x, y, x1),
      gd_uvline_expose(x, y, y1));
}

int gd_box(int x, int y, int x1, int y1)
{
   if ((x>=grd_clip.right)||(y>=grd_clip.bot)||
       (x1<=grd_clip.left)||(y1<=grd_clip.top))
      return CLIP_ALL;
   gdi_box(x, y, x1-1, y1-1,
      (gdulin_func *)gd_hline,
      (gdulin_func *)gd_vline);
   return CLIP_NONE;
}



void gd_urect(int x, int y, int x1, int y1)
{
   gdulin_func *uhlin_func;
   uhlin_func = gd_uhline_expose(x, y, x1);
   x1--;
   for (;y<y1; y++)
      uhlin_func(x, y, x1);
}

#define clip_rect(x, y, x1, y1) \
do {                          \
   if (x<grd_clip.left)       \
      x = grd_clip.left;      \
   if (y<grd_clip.top)        \
      y = grd_clip.top;       \
   if (x1>grd_clip.right)     \
      x1 = grd_clip.right;    \
   if (y1>grd_clip.bot)       \
      y1 = grd_clip.bot;      \
} while (0)

int gd_rect(int x, int y, int x1, int y1)
{
   if ((x>=grd_clip.right)||(y>=grd_clip.bot)||
       (x1<=grd_clip.left)||(y1<=grd_clip.top))
      return CLIP_ALL;
   clip_rect(x, y, x1, y1);
   gd_urect(x, y, x1, y1);
   return CLIP_NONE;
}

#include <context.h>
void gd_clear(int c)
{
   int fc_save =            
      gr_get_fcolor();   
   gr_set_fcolor(c);     
   gd_urect(             
      gr_get_clip_l(),   
      gr_get_clip_t(),   
      gr_get_clip_r(),   
      gr_get_clip_b());  
   gr_set_fcolor(fc_save);  
}
