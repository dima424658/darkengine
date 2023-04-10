// $Header: x:/prj/tech/libsrc/dev2d/RCS/ltab.c 1.2 1998/04/03 13:29:38 KEVIN Exp $

#include <dbg.h>
#include <grs.h>
#include <pal16.h>

// 4 supported bitmasks:
// r = 0x001f, g=0x03e0, b=0x7c00
// r = 0x001f, g=0x07e0, b=0xf800
// r = 0x7c00, g=0x03e0, b=0x001f
// r = 0xf800, g=0x07e0, b=0x001f


void gr_compute_ltab816(ushort *ltab, uchar *pal_data, int dark_levels, int bright_levels, grs_rgb_bitmask *bitmask)
{
   int low_bits, i, j;
   uchar bshift;
   ulong *high_bitmask;

   if (bitmask->blue==0x1f) {
      low_bits = 2;
      high_bitmask = &bitmask->red;
   } else {
      if (bitmask->red != 0x1f)
         goto bad_bitmask;
      low_bits = 0;
      high_bitmask = &bitmask->blue;
   }
   if (bitmask->green==0x3e0) {
      if (*high_bitmask != 0x7c00)
         goto bad_bitmask;
      bshift = 10;
   } else {
      if ((bitmask->green!=0x7e0)||(*high_bitmask != 0xf800))
         goto bad_bitmask;
      bshift = 11;
   }

   for (j=0; j<dark_levels; j++) {
      ushort *ltab_row = &ltab[j*256];
      for (i=0;i<256;i++) {
         int r = pal_data[3*i+low_bits];
         int g = pal_data[3*i+1];
         int b = pal_data[3*i+2-low_bits];
         r=r*j/(dark_levels-1);
         g=g*j/(dark_levels-1);
         b=b*j/(dark_levels-1);
         r=r>>3, g=g>>(13-bshift), b=b>>3;
         ltab_row[i] = r+(g<<5)+(b<<bshift);
      }
   }

   for (j=0; j<bright_levels; j++) {
      int white = (0xff * (j+1)) / bright_levels;
      int color = (bright_levels - 1) - j;
      ushort *ltab_row = &ltab[(dark_levels + j)*256];

      for (i=0;i<256;i++) {
         int r = pal_data[3*i+low_bits];
         int g = pal_data[3*i+1];
         int b = pal_data[3*i+2-low_bits];
         r=r*color/bright_levels + white;
         g=g*color/bright_levels + white;
         b=b*color/bright_levels + white;
         r=r>>3, g=g>>(13-bshift), b=b>>3;
         ltab_row[i] = r+(g<<5)+(b<<bshift);
      }
   }

   return;

bad_bitmask:
   Warning(("gr_compute_ltab816(): invalid bitmask\n"));
}
