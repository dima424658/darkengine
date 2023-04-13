/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/gdbm.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/05/01 13:53:02 $
 *
 */

#include <clip2d.h>
#include <gdbm.h>
extern void (*gen_bitmap_func[])();

static int gdi_bitmap(grs_bitmap *bm, int x, int y, gdubm_func *ubm_func)
{
   int code;

   code = CLIP_NONE;
   if (x < grd_clip.left)              /* off left edge */
      code |= CLIP_LEFT;
   if (x+bm->w > grd_clip.right)       /* off right edge */
      code |= CLIP_RIGHT;
   if (y < grd_clip.top)               /* off top */
      code |= CLIP_TOP;
   if (y+bm->h > grd_clip.bot)         /* off bottom */
      code |= CLIP_BOT;

   if (code != CLIP_NONE)
      ((gdbm_func *)gen_bitmap_func[bm->type])(bm, x, y, code, ubm_func);
   else
      ubm_func(bm, x, y);

   return code;
}

#define all_clipped(bm, x, y) \
   (x+bm->w<=grd_clip.left || x>=grd_clip.right || \
    y+bm->h<=grd_clip.top  || y>=grd_clip.bot)

int gd_bitmap_opt(grs_bitmap *bm, int x, int y, gdubm_func *ubm_func)
{
   if (all_clipped(bm, x, y))  
      return CLIP_ALL;
   return gdi_bitmap(bm, x, y, ubm_func);
}

int gd_bitmap(grs_bitmap *bm, int x, int y)
{
   gdubm_func_p ubm_func;

   if (all_clipped(bm, x, y))  
      return CLIP_ALL;
   ubm_func = gd_ubitmap_expose(bm, x, y);
   return gdi_bitmap(bm, x, y, ubm_func);
}
