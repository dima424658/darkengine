/*
 * $Source: s:/prj/tech/libsrc/font/RCS/strclr.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:25:29 $
 *
 * Clipped string clear routine.
 *
 * This file is part of the font library.
 *
 */

#include <strsiz.h>
#include <dev2d.h>
 
/* clear a string to the background color, with clipping. */

int gr_font_string_clear (grs_font *f, char *s, short x, short y)
{
   short w, h;                /* extent of string */
   long c;                    /* saved foreground color */
   int r;                     /* return clip code */

   c = gr_get_fcolor();
   gr_set_fcolor (gr_get_bcolor ());
   gr_font_string_size (f, s, &w, &h);
   r = gr_rect (x, y, x+w, y+h);
   gr_set_fcolor (c);
   return r;
}

/* clear a string to the background color, unclipped. */

void gr_font_string_uclear (grs_font *f, char *s, short x, short y)
{
   short w, h;                /* extent of string */
   long c;                    /* saved foreground color */

   c = gr_get_fcolor();
   gr_set_fcolor (gr_get_bcolor());
   gr_font_string_size (f, s, &w, &h);
   gr_urect (x, y, x+w, y+h);
   gr_set_fcolor (c);
}
