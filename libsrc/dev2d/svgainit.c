/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/svgainit.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:09:20 $
 *
 * Prototypes for detection routines.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <devtab.h>
#include <dftctab.h>
#include <mode.h>
#include <vesa.h>

/* Only checks for vesa support.             */
/* returns non-zero if an error is detected. */

int svga_detect(grs_sys_info *info)
{
   int err = 0;

// Try to init the vesa bios extension.  If doesn't work
// indicate only vga modes available.

   if (!VBEInit(info)) {
      /* No vesa driver; support vga only. urk. */
      info->id_maj = 0;
      info->id_min = 0;
      info->memory = 256;
      info->modes[0] = GRM_320x200x8;
      info->modes[1] = GRM_320x200x8X;
      info->modes[2] = GRM_320x240x8;
      info->modes[3] = GRM_320x400x8;
      info->modes[4] = GRM_320x480x8;
      info->modes[5] = -1;
      grd_device_table = vga_device_table;
   } else {
   // Indicate we shall use the vesa device tables from now on
      grd_device_table = vesa_device_table;
   }
   return err;
}

void gd_use_svga()
{
   gdd_flat8_canvas_table  = gdd_default_flat8_canvas_table;
   gdd_flat16_canvas_table = gdd_default_flat16_canvas_table;
//   gdd_flat24_canvas_table = gdd_default_flat24_canvas_table;
   gdd_bank8_canvas_table  = gdd_default_bank8_canvas_table;
//   gdd_bank16_canvas_table = gdd_default_bank16_canvas_table;
//   gdd_bank24_canvas_table = gdd_default_bank24_canvas_table;
   gdd_modex_canvas_table  = gdd_default_modex_canvas_table;

   gdd_flat24_canvas_table = gdd_null_canvas_table;
   gdd_bank16_canvas_table = gdd_null_canvas_table;
   gdd_bank24_canvas_table = gdd_null_canvas_table;

   gdd_detect_func = svga_detect;
}

extern int gd_init();
int gd_svga_init()
{
   gd_use_svga();
   return gd_init();
}
