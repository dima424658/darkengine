/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/lpix.c $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1998/03/26 10:40:22 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <pixftype.h>

int flat8_lpix8(fix i, int c)
{
   return ((uchar *)grd_light_table)[(fix_int(i)<<8)+c];
}

int flat16_lpix16(fix i, int c)
{
   ushort *clut = ((ushort *)grd_light_table)+(fix_int(i)<<9);
   return (clut[c&0xff]+clut[256+(c>>8)]);
}

static int flat16_lpix8_pal(fix i, int c)
{
   return flat16_lpix16(i, ((ushort *)pixpal)[c]);
}

static int flat16_lpix8_direct(fix i, int c)
{
   return grd_ltab816[(fix_int(i)<<8)+c];
}

int flat16_lpix8(fix i, int c)
{
   return (grd_ltab816==NULL) ?
      flat16_lpix8_pal(i, c):
      flat16_lpix8_direct(i, c);
}

#pragma off(unreferenced)
gdlpix_func *flat8_lpix8_expose(fix i, int c)
{

   return flat8_lpix8;
}

gdlpix_func *flat16_lpix8_expose(fix i, int c)
{
   return (grd_ltab816==NULL) ?
      flat16_lpix8_pal:flat16_lpix8_direct;
}

gdlpix_func *flat16_lpix16_expose(fix i, int c)
{
   return flat16_lpix16;
}
#pragma on(unreferenced)
