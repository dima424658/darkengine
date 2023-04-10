/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/init.c $
 * $Revision: 1.4 $
 * $Author: KEVIN $
 * $Date: 1997/09/23 12:41:05 $
 *
 * This file is part of the g2 library.
 *
 */

#include <dbg.h>
#include <dev2d.h>
#include <g2d.h>
#include <setdrv.h>
#include <g2makewr.h>

void g2_set_canvas(grs_canvas *c)
{
   switch (c->bm.type) {
   case BMT_FLAT8:
      g2d_canvas_table = g2d_flat8_canvas_table; break;
   case BMT_FLAT16:
      g2d_canvas_table = g2d_flat16_canvas_table; break;
   case BMT_FLAT24:
      g2d_canvas_table = g2d_flat24_canvas_table; break;
   default:
      g2d_canvas_table = g2d_gen_canvas_table; break;
   }
}

int g2d_active=0;
static int callback_id;
int g2_init()
{
   extern void g2pt_init(void);
   int err;

   if (g2d_active!=0) {
      Warning(("g2_init(): g2 already active.\n"));
      return 0;
   }

   g2_make_writable();
   err = gd_init();
   if (err!=0) {
      Warning(("g2_init(): Error initializing dev2d!\n"));
      return err;
   }
   g2_use_default_drivers();
   g2pt_init();               // init "portal" mappers
   callback_id = gr_install_set_canvas_callback(g2_set_canvas);
   g2d_active=1;
   return 0;
}

int g2_close()
{
   if (g2d_active == 0)
      return 0;
   gr_uninstall_set_canvas_callback(callback_id);
   g2d_active = 0;
   return gd_close();
}
