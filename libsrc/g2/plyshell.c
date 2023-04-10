// $Header: x:/prj/tech/libsrc/g2/RCS/plyshell.c 1.3 1997/05/01 15:37:00 KEVIN Exp $
//
// polygon rendering shell.

#include <plyparam.h>
#include <plyrast.h>
#include <buffer.h>


bool g2d_skip_il = FALSE;

void g2_poly_shell(g2s_poly_params *p, int num_scanlines)
{
   g2s_raster_list r = p->raster_list;

   goto loop_entry;
   do {
      r.bytes += p->raster_row;
loop_entry:
      if ((r.raster->n>0)&&(!g2d_skip_il))
         p->il(r.raster, p);
   } while (--num_scanlines);
   gr_free_temp(p->raster_list.bytes);
}

