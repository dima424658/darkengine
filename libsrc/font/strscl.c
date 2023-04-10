/* $Source: x:/prj/tech/libsrc/font/RCS/strscl.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1997/01/23 14:06:07 $
 *
 * Font string scale routines.
 *
 * This file is part of the font library.
 *
 */

#include <chr.h>
#include <fonts.h>
#include <g2.h>
#include <strsiz.h>

extern void font_setup(grs_font_info *fi, grs_font *f);

int gr_font_scale_string (grs_font *f, char *s, int x0, int y0, int w, int h)
{
   grs_font_info fi;
   int offset;                /* offset of current character */
   short str_w, str_h;        /* width and height of src string */
   int x, y, dy;              /* position, delta y of current character */
   fix xf, yf, dyf;           /* fixed point position, delta y */
   int next_x, next_y;        /* need to use next */
   g2s_func *scale_func;

   fix x_scale, y_scale;      /* x and y scale factors */
   uint i;                     
   uchar c;                   /* current character */

   if ((w<=0)||(h<=0))
      return CLIP_ALL;

   gr_font_string_size (f, s, &str_w, &str_h);

   font_setup (&fi, f);

   // won't work for antialised fonts...
   scale_func = gr_scale_bitmap_expose(&fi.bm, 0, 0, 0, 0);

   x_scale = fix_make(w, 0) / str_w;
   y_scale = fix_make(h, 0) / str_h;

   x = x0; y = y0;

   dyf = y_scale * f->h;
   xf = fix_make(x,0);
   yf = fix_make(y,0) + dyf;
   next_y = fix_int(yf);
   dy = next_y - y;

   while ((c = (uchar)(*s++)) != '\0') {
      if (c=='\n' || c==CHAR_SOFTCR) {
         x = x0;
         xf = fix_make(x0, 0);
         y = next_y;
         yf += dyf;
         next_y = fix_int(yf);
         dy = next_y - y;
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
         offset>>=3;
      fi.bm.bits = fi.bits + offset;

      xf += x_scale * fi.bm.w;
      next_x = fix_int(xf);

      scale_func (&fi.bm, x, y, next_x - x, dy);
      x = next_x;
   }
   return CLIP_NONE;
}

void gr_font_scale_ustring (grs_font *f, char *s, int x0, int y0, int w, int h)
{
   grs_font_info fi;
   int offset;                /* offset of current character */
   short str_w, str_h;        /* width and height of src string */
   int x, y, dy;              /* position, delta y of current character */
   fix xf, yf, dyf;           /* fixed point position, delta y */
   int next_x, next_y;        /* need to use next */

   fix x_scale, y_scale;      /* x and y scale factors */
   uint i;                     
   uchar c;                   /* current character */
   g2us_func *uscale_func;

   if ((w<=0)||(h<=0))
      return;

   gr_font_string_size (f, s, &str_w, &str_h);
   uscale_func = gr_scale_ubitmap_expose(&fi.bm, 0, 0, 0, 0);

   x_scale = fix_make(w, 0) / str_w;
   y_scale = fix_make(h, 0) / str_h;

   x = x0; y = y0;

   dyf = y_scale * f->h;
   xf = fix_make(x,0);
   yf = fix_make(y,0) + dyf;
   next_y = fix_int(yf);

   while ((c = (uchar)(*s++)) != '\0') {
      if (c=='\n' || c==CHAR_SOFTCR) {
         x = x0;
         xf = fix_make(x0, 0);
         y = next_y;
         yf += dyf;
         next_y = fix_int(yf);
         dy = next_y - y;
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
         offset>>=3;
      fi.bm.bits = fi.bits + offset;

      xf += x_scale * fi.bm.w;
      next_x = fix_int(xf);

      uscale_func (&fi.bm, x, y, next_x - x, dy);
      x = next_x;
   }
}


