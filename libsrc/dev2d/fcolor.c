// $Header: x:/prj/tech/libsrc/dev2d/RCS/fcolor.c 1.3 1998/04/06 16:11:30 KEVIN Exp $

#include <dbg.h>

#include <grs.h>
#include <grd.h>
#include <mode.h>
#include <grdev.h>
#include <pal16.h>
#include <rgb.h>
#include <fcolor.h>

// make an fcolor for the current screen mode from an 888 lrgb (red bits low).
int gr_make_screen_fcolor(int lrgb)
{
   if (grd_mode < 0) {
      Warning(("gr_make_screen_fcolor(): No mode set!\n"));
      return 0;
   }

   switch (grd_bpp)
   {
      case 8:
      {
         if (grd_ipal==NULL) {
            Warning(("gr_make_screen_fcolor(): No ipal set...\n"));
            return 0;
         }
         return grd_ipal[gr_index_lrgb(lrgb)];
      }
      case 15:
      case 16:
      {
         grs_rgb_bitmask bmask;
         ushort retval;
         uchar pal_entry[3];
         pal_entry[0] = lrgb&0xff;
         pal_entry[1] = (lrgb>>8)&0xff;
         pal_entry[2] = (lrgb>>16)&0xff;

         gr_get_screen_rgb_bitmask(&bmask);
         gr_make_pal16(0, 1, &retval, pal_entry, &bmask);
         return retval;
      }
   }
   Warning(("gr_make_screen_fcolor(): Unsupported bitdepth\n"));
   return 0;
}
