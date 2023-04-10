/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/genbm.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 15:37:36 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <context.h>
#include <gdrect.h>

void gen_opaque_solid_ubitmap(grs_bitmap *bm, int x, int y)
{
   int fc_save=gr_get_fcolor();
   gr_set_fcolor(gr_get_fill_parm());
   gd_urect(x, y, x+bm->w, y+bm->h);
   gr_set_fcolor(fc_save);
}
