/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/mxfl8.c $
 * $Revision: 1.3 $
 * $Author: KEVIN $
 * $Date: 1997/01/28 16:51:07 $
 *
 * Routines for drawing flat8 bitmaps onto a mode X canvas.
 *
 * This file is part of the dev2d library.
 *
 */

#include <grd.h>
#include <mxlatch.h>

#define BIG_BLIT_MINIMUM        0x4000          // 16k
#define PROC_DATA_CACHE         0x1000          // rough size of data cache

#ifdef __WATCOMC__
modex_memmove (uchar *s, uchar *d, int h, int s_row, int d_row);
#pragma aux modex_memmove =   \
"L1:"                         \
   "mov     al,[esi]"         \
   "mov     [edi],al"         \
   "add     esi,ebx"          \
   "add     edi,ecx"          \
   "dec     edx"              \
   "jnz     L1"               \
   parm [esi] [edi] [edx] [ebx] [ecx] \
   modify [eax ecx edx];
#else
__inline modex_memmove (uchar *s, uchar *d, int h, int s_row, int d_row)
{
	__asm
	{
		mov		esi, s
		mov		edi, d
		mov	   edx, h
		mov	   ebx, s_row
		mov	   ecx, d_row
	L1:
		mov		al, [esi]
		mov		[edi], al
		add		esi, ebx
		add		edi, ecx
		dec		edx
		jnz		L1
	}
}
#endif

void modex_flat8_trans_ubitmap (grs_bitmap *bm, short x, short y)
{
   int i,j,k;
   uchar *src, *dst;
   uchar *s, *d;
   uchar *p;
   short w, h;
   uchar save_wlatch;

   wlatch_start(save_wlatch);

   w = bm->w;
   h = bm->h;
   if ((h<=0)||(w<=0)) return;
   p = grd_bm.bits+grd_bm.row*y;
   x += grd_bm.align;
   for (i=0; i<4; i++) {
      src=bm->bits+i;
      dst=p+(x>>2);
      modex_force_wlatch(1<<(x&3));
      for (j=(w+3)>>2; j>0; j--) {
         s=src; d=dst;
         for (k=0; k<h; k++) {
            if (*s) *d=*s;
            s+=bm->row;
            d+=grd_bm.row;
         }
         src+=4;
         dst++;
      }
      x++;
      if (--w==0) break;
   }

   wlatch_restore(save_wlatch);
}

void modex_flat8_opaque_ubitmap (grs_bitmap *bm, short x, short y)
{
   int i,j;
   uchar *src, *dst;
   uchar *p;
   short w, h;
   uchar save_wlatch;

   wlatch_start(save_wlatch);

   w = bm->w;
   h = bm->h;
   if ((h<=0)||(w<=0)) return;
   p = grd_bm.bits+grd_bm.row*y;
   x += grd_bm.align;
   if (w * h > BIG_BLIT_MINIMUM) {
      int nh;
      // too big to fit in cache, so draw in smaller chunks that fit
      nh = PROC_DATA_CACHE / w;
      y = 0;
      while (h) {
         if (h < nh) nh = h;
         for (i=0; i < 4; i++) {
            src = bm->bits+i+bm->row*y;
            dst = p + ((x+i)>>2) + (grd_bm.row*y);
            modex_force_wlatch(1<<((x+i)&3));
            // assume we're more horizontal than vertical
            for(j=nh; j > 0; j--) {
               modex_memmove(src,dst,(w+3)>>2,4,1);
               src += bm->row;
               dst += grd_bm.row;
            }
            --w;
         }
         w += 4;
         y += nh;
         h -= nh;
      }
   } else {
      for (i=0; i<4; i++) {
         src = bm->bits+i;
         dst = p + (x>>2);
         modex_force_wlatch(1<<(x&3));
         for (j=(w+3)>>2; j>0; j--) {
            modex_memmove(src,dst,h,bm->row,grd_bm.row);
            src += 4;
            dst++;
         }
         x++;
         if (--w==0) break;
      }
   }

   wlatch_restore(save_wlatch);
}
