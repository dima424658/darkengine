/*
 * $Source: s:/prj/tech/libsrc/font/RCS/chrsiz.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:24:58 $
 *
 * character size routines.
 *
 * This file is part of the font library.
 *
 */

#include <fonts.h>

/* returns the width in pixels of c in the specified font. */
short gr_font_char_width (grs_font *f, char c)
{
   short *off_tab;            /* character offset table */
   uint i;                    /* index into offset table */

   i = ((uchar )c) - f->min;              /* if c < f->min, i is large positive! */
   if (i > ((uint )(f->max - f->min)))    /* true if c < f->min also! */
      return 0;
   off_tab = f->off_tab;
   return off_tab[i+1]-off_tab[i];
}

/* returns the width in pixels of c in the specified font */

void gr_font_char_size (grs_font *f, char c, short *w, short *h)
{
   *h = f->h;
   *w = gr_font_char_width (f, c);
}

