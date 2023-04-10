// $Header: x:/prj/tech/libsrc/font/RCS/str.c 1.2 1997/01/23 14:05:48 KEVIN Exp $
// String drawing functions

#include <chr.h>
#include <fonts.h>

extern void font_setup(grs_font_info *fi, grs_font *f);

int gr_font_string (grs_font *f, char *s, int x0, int y0)
{
   grs_font_info fi;
   int offset;                /* offset of current character */
   int x, y;                  /* position of current character */
   uint i;                    /* index into offset_tab */

   if (x0 > grd_clip.right || y0 > grd_clip.bot)
     return CLIP_ALL;

   font_setup(&fi, f);
   x = x0; y = y0;

   while (y + f->h < grd_clip.top) {
      uchar c = (uchar )*s++;
      if (c == '\0') return CLIP_ALL;
      if ((c != CHAR_SOFTCR) && (c != '\n'))
         continue;
      y += f->h;
   }

   while (y < grd_clip.bot) {
      do {
         uchar c = (uchar )*s++;
         if (c == '\0')
            return CLIP_NONE;
         if (c == CHAR_SOFTCR || c== '\n')
            break;
         if (c == CHAR_SOFTSP)
            continue;
         i = c - f->min;
         if (i > fi.range)
            continue;
         offset = fi.off_tab[i];
         fi.bm.w = fi.off_tab[i+1]-offset;
         fi.bm.align = offset&fi.align_mask + fi.align_base;
         if (fi.align_mask != 0)
            offset >>= 3;
	      fi.bm.bits = fi.bits + offset;
         gd_bitmap_opt(&fi.bm, x, y, fi.blitter);
         x += fi.bm.w;
         if (x >= grd_clip.right) {
            do {
               c = (uchar )*s++;
               if (c == '\0') return CLIP_NONE;
            } while (c != CHAR_SOFTCR && c!= '\n');
            break;
         }
      } while (1);
      y += f->h;
      x = x0;
   }
   return CLIP_NONE;
}
     
void gr_font_ustring (grs_font *f, char *s, int x0, int y0)
{
   grs_font_info fi;
   int offset;                /* offset of current character */
   int x, y;                  /* position of current character */
   uchar c;                   /* current character */
   uint i;                    /* index into offset_tab */

   font_setup(&fi, f);
   x = x0; y = y0;
   while ((c=(uchar) (*s++)) != '\0') {
      if (c=='\n' || c==CHAR_SOFTCR) {
         x = x0;
         y += f->h;
         continue;
      }
      if (c==CHAR_SOFTSP)
         continue;
      i = c - f->min;
      if (i > fi.range)
         continue;

      offset = fi.off_tab[i];
      fi.bm.w = fi.off_tab[i+1]-offset;
      fi.bm.align = offset&fi.align_mask + fi.align_base;
      if (fi.align_mask != 0)
         offset >>= 3;
	   fi.bm.bits = fi.bits + offset;
      fi.blitter(&fi.bm, x, y);
      x += fi.bm.w;
   }
}
