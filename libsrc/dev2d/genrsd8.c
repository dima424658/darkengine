/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/genrsd8.c $
 * $Revision: 1.3 $
 * $Author: KEVIN $
 * $Date: 1996/06/28 11:43:15 $
 *
 */

#include <bitmap.h>
#include <clip.h>
#include <gdbm.h>
#include <rsd.h>
#include <string.h>

// Hack for flat8 canvases only!
void gri_flat8_rsd8_bitmap (grs_bitmap *bm, int x_left, int y_top, int code)
{
   int x,y;                            /* current destination position */
   int x_right,y_bot;                  /* opposite edges of bitmap */
   int x_off,y_off;                    /* x,y offset for clip */
   ulong start_byte;                   /* byte to start drawing */
   ulong cur_byte;                     /* current position within rsd */
   uchar *p_dst;
   uchar *rsd_src;                     /* rsd source buffer */
   int rsd_code;                       /* last rsd opcode */
   int rsd_count;                      /* count for last opcode */
   int op_count;                       /* operational count */

   rsd_src = bm->bits;
   x = x_left; y = y_top;
   x_off = y_off = cur_byte = rsd_count = 0;
   x_right = x_left+bm->w;
   y_bot = y_top +bm->h;

   if (code&CLIP_LEFT) {
      /* clipped off left edge. */
      x_off = grd_clip.left-x_left;
      x = grd_clip.left;
   }
   if (code&CLIP_TOP) {
      /* clipped off top edge. */
      y_off = grd_clip.top-y_top;
      y = grd_clip.top;
   }
   if (code&CLIP_RIGHT) {
      /* clipped off right edge. */
      x_right = grd_clip.right;
   }
   if (code&CLIP_BOT) {
      /* clipped off bottom edge. */
      y_bot = grd_clip.bot;
   }

   if (y_off>0 || x_off>0) {
      /* been clipped of left and/or top, so we need to skip from beginning
         of rsd buffer to be at x_off,y_off within rsd bitmap. */
      start_byte = y_off*bm->row + x_off;
      while (cur_byte < start_byte) {
         if (rsd_count == 0)
            /* no pending opcodes, get a new one. */
            RSD_GET_TOKEN ();
         if (cur_byte+rsd_count <= start_byte) {
            /* current code doesn't hit start_byte yet, so skip all of it. */
            switch (rsd_code) {
            case RSD_RUN:
               /* advance past 1 byte of run color. */
               rsd_src++;
               break;
            case RSD_SKIP:
               break;
            default: /* RSD_DUMP */
               /* advance past rsd_count bytes of dump pixel data. */
               rsd_src += rsd_count;
               break;
            }
            cur_byte += rsd_count;
            rsd_count = 0;
         }
         else {
            /* current code goes past start_byte, so skip only enough to get
               to start_byte. */
            op_count = start_byte-cur_byte;
            switch (rsd_code) {
            case RSD_RUN:
               break;
            case RSD_SKIP:
               break;
            default: /* RSD_DUMP */
               rsd_src += op_count;
               break;
            }
            cur_byte += op_count;
            rsd_count -= op_count;
         }
      }
   }

   p_dst = grd_bm.bits + y*grd_bm.row + x;

   /* process each scanline in two chunks. the first is the clipped section
      from the right edge, wrapping around to to the left. the second is the
      unclipped area in the middle. */
   while (y < y_bot) {
      /* clipped section. */
      while (x < x_left+x_off) {
         if (rsd_count == 0)
            RSD_GET_TOKEN ();
         if (x+rsd_count <= x_left+x_off) {
            switch (rsd_code) {
            case RSD_RUN:
               rsd_src++;
               break;
            case RSD_SKIP:
               break;
            default: /* RSD_DUMP */
               rsd_src += rsd_count;
               break;
            }
            x += rsd_count;
            rsd_count = 0;
         }
         else {
            op_count = x_left+x_off-x;
            switch (rsd_code) {
            case RSD_RUN:
               break;
            case RSD_SKIP:
               break;
            default: /* RSD_DUMP */
               rsd_src += op_count;
               break;
            }
            rsd_count -= op_count;
            x += op_count;
         }
      }

      /* section to draw. */
      while (x < x_right) {
         if (rsd_count == 0)
            RSD_GET_TOKEN ();
         if (x+rsd_count <= x_right) {
            switch (rsd_code) {
            case RSD_RUN:
               memset (p_dst, *rsd_src, rsd_count);
               rsd_src++;
               break;
            case RSD_SKIP:
               break;
            default: /* RSD_DUMP */
               memcpy (p_dst, rsd_src, rsd_count);
               rsd_src += rsd_count;
               break;
            }
            x += rsd_count;
            p_dst += rsd_count;
            rsd_count = 0;
         }
         else {
            op_count = x_right-x;
            switch (rsd_code) {
            case RSD_RUN:
               memset (p_dst, *rsd_src, op_count);
               break;
            case RSD_SKIP:
               break;
            default: /* RSD_DUMP */
               memcpy (p_dst, rsd_src, op_count);
               rsd_src += op_count;
               break;
            }
            x += op_count;
            p_dst += op_count;
            rsd_count -= op_count;
         }
      }

      /* reset x to be beginning of line and set y to next scanline. */
      x -= bm->w;
      p_dst += grd_bm.row-(x_right-x_left)+x_off;
      y++;
   }
rsd_done:;
}

/* convert and blit */
static grs_bitmap dbm;
void gen_rsd8_ubitmap (grs_bitmap *bm, int x, int y)
{
   grs_bitmap dbm_save=dbm;
   gd_rsd8_convert(bm, &dbm);
   gd_bitmap(&dbm, x, y);
   dbm = dbm_save;
}

#pragma off(unreferenced)
void gen_rsd8_bitmap (grs_bitmap *bm, int x, int y, int code, gdubm_func *ubm_func)
{
   if (grd_bm.type==BMT_FLAT8)
      gri_flat8_rsd8_bitmap(bm, x, y, code);
   else      
      gen_rsd8_ubitmap(bm, x, y);
}
#pragma on(unreferenced)
