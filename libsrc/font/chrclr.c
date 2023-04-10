/*
 * $Source: s:/prj/tech/libsrc/font/RCS/chrclr.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:24:48 $
 *
 * Clipped character clear routine.
 *
 * This file is part of the font library.
 *
 */

#include <fonts.h>
#include <chrsiz.h>
#include <dev2d.h>

/* clear the rectangle occupied by the character c at (x,y) on the
   current canvas.  clips to the current clipping rectangle. */

int gr_font_char_clear (grs_font *f, char c, int x, int y)
{
   long color;                /* klugey save of current color */
   short w, h;                /* width and height of character */
   int r;                     /* return value */

   /* get width and height of character */
   gr_font_char_size(f, c, &w, &h);

   /* draw rectangle of size width of c x height of font. */
   color = gr_get_fcolor ();
   gr_set_fcolor (gr_get_bcolor ());
   r = gr_rect (x, y, x+w, y+h);
   gr_set_fcolor (color);
   return r;
}

void gr_font_char_uclear (grs_font *f, char c, int x, int y)
{
   long color;                /* klugey save of current color */
   short w, h;                /* width and height of character */

   /* get width and height of character */
   gr_font_char_size(f, c, &w, &h);

   /* draw rectangle of size width of c x height of font. */
   color = gr_get_fcolor ();
   gr_set_fcolor (gr_get_bcolor ());
   gr_urect (x, y, x+w, y+h);
   gr_set_fcolor (color);
}


