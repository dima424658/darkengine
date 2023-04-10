// $Header: x:/prj/tech/libsrc/dev2d/RCS/nulldfcn.c 1.2 1998/03/12 10:42:22 KEVIN Exp $

#include <grs.h>
#include <mode.h>
#include <state.h>

#pragma off(unreferenced)

int null_device(grs_sys_info *info)
{
   return TRUE;
}

int null_set_mode(int a, int b)
{
   return TRUE;
}

int null_get_mode()
{
   return 0;
}

int null_state(VideoState *vs)
{
   return FALSE;
}

int null_trace()
{
   return TRUE;
}

int null_width()
{
   return 0;
}

void null_get_focus(int *x, int *y)
{
   *x = 0;
   *y = 0;
}

void null_get_rgb_bitmask(grs_rgb_bitmask *bitmask)
{
   switch (grd_bpp) {
   case 15:
      bitmask->red = 0x7c00;
      bitmask->green = 0x3e0;
      bitmask->blue = 0x1f;
      break;
   case 16:
      bitmask->red = 0xf800;
      bitmask->green = 0x7e0;
      bitmask->blue = 0x1f;
      break;
   default:
      bitmask->red = 0;
      bitmask->green = 0;
      bitmask->blue = 0;
   }
}

#pragma on(unreferenced)

