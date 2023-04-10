/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/hflip.c $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/05/15 15:59:56 $
 */

#include <grs.h>
#include <bitmap.h>
#include <string.h>

void gd_hflip_in_place(grs_bitmap *bm)
{
   int hw=bm->w>>1;
   int h=bm->h;
   uchar tmp;
   uchar *left=bm->bits;
   uchar *right=bm->bits+bm->w-1;

   if ((bm->type!=BMT_FLAT8)&&
       (bm->type!=BMT_TLUC8))
         return;
   while (h--) {
      int i;
      for (i=0;i<hw;i++) {
         tmp=*left;
         *left=*right;
         *right=tmp;
         left++,right--;
      }
      left+=bm->row-hw;
      right+=bm->row+hw;
   }
}



