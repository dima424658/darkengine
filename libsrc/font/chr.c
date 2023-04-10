// $Header: x:/prj/tech/libsrc/font/RCS/chr.c 1.2 1997/01/23 14:05:03 KEVIN Exp $
// character drawing functions.

#include <fonts.h>

extern void font_setup(grs_font_info *fi, grs_font *f);

static int char_guts(grs_font_info *fi, grs_font *f, char c, int x)
{
   int offset;                /* current character offset */
   uint i;                    /* index into offset table */

   /* range check char, get font table pointers, offset. */
   i = ((uchar )c) - f->min;              /* if c < f->min, i is large positive! */
   font_setup(fi, f);
   if (i > fi->range)         /* true if c < f->min also! */
      return CLIP_ALL;

   offset = fi->off_tab[i];
   fi->bm.w = fi->off_tab[i+1]-offset;
   fi->bm.align = (offset&fi->align_mask) + fi->align_base;
   if (fi->align_mask != 0)
      offset >>= 3;
      
   fi->bm.bits = fi->bits + offset;

   return CLIP_NONE;
}

int gr_font_char (grs_font *f, char c, int x, int y)
{
   grs_font_info fi;

   if (char_guts(&fi, f, c, x)==CLIP_ALL)
      return CLIP_ALL;
   return gd_bitmap_opt(&fi.bm, x, y, fi.blitter);
}

void gr_font_uchar (grs_font *f, char c, int x, int y)
{
   grs_font_info fi;

   if (char_guts(&fi, f, c, x)==CLIP_ALL)
      return;
   fi.blitter(&fi.bm, x, y);
}
