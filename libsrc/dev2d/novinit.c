/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/novinit.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/08/29 09:44:41 $
 *
 * Prototypes for detection routines.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <devtab.h>
#include <dftctab.h>

/* don't support _any_ hardware.             */
/* returns non-zero if an error is detected. */

int no_video_detect(grs_sys_info *info)
{
   info->id_maj = 0;
   info->id_min = 0;
   info->memory = 0;
   info->modes[0] = -1;

   grd_device_table = no_video_device_table;
   return 0;
}

void gd_use_no_video()
{
   gdd_flat8_canvas_table  = gdd_default_flat8_canvas_table;
   gdd_flat16_canvas_table = gdd_default_flat16_canvas_table;
//   gdd_flat24_canvas_table = gdd_default_flat24_canvas_table;
   gdd_flat24_canvas_table = gdd_null_canvas_table;

   gdd_bank8_canvas_table = gdd_null_canvas_table;
   gdd_bank16_canvas_table = gdd_null_canvas_table;
   gdd_bank24_canvas_table = gdd_null_canvas_table;
   gdd_modex_canvas_table  = gdd_null_canvas_table;

   gdd_detect_func = no_video_detect;
}

extern int gd_init();
int gd_no_video_init()
{
   gd_use_no_video();
   return gd_init();
}
