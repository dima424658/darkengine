/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/gdgbm.c $
 * $Revision: 1.3 $
 * $Author: TOML $
 * $Date: 1996/06/26 10:34:40 $
 *
 * get bitmap.
 *
 */

#include <bitmap.h>
#include <canvas.h>
#include <grd.h>
#include <gdbm.h>

static grs_canvas get_cnv;
void gd_get_bitmap(grs_bitmap *bm, int x, int y)
{
   grs_canvas *c = grd_canvas;
   grs_canvas gc_save=get_cnv;
   short flags_save=c->bm.flags;

   gr_make_canvas(bm, &get_cnv);
   gr_set_canvas(&get_cnv);

   c->bm.flags &= ~BMF_TRANS;
   gd_bitmap(&(c->bm), -x, -y);
   c->bm.flags = flags_save;

   gr_set_canvas(c);
   gr_close_canvas(&get_cnv);

   get_cnv = gc_save;
}
