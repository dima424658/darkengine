// $Header: x:/prj/tech/libsrc/dev2d/RCS/rsdcvt.c 1.5 1998/03/12 10:42:56 KEVIN Exp $

// Routine for unpacking an rsd bitmap to a flat8 bitmap.
// Uses memory provided externally.

#include <string.h>

#include <memall.h>
#include <dbg.h>
#include <lgassert.h>

#include <bitmap.h>
#include <gdbm.h>
#include <rsd.h>
#include <rsdcfg.h>

static int unpack_buf_cap = 0;
int unpack_flags = FALSE;

static void alloc_buffer(int size)
{
   grd_unpack_buf = Malloc(size);
   grd_unpack_buf_size = size;
   if (unpack_buf_cap < grd_unpack_buf_size)
      Warning(("gd_rsd8_convert(): unpack_buf_cap %i exceeded. unpack_buf_size %i\n",
         unpack_buf_cap, grd_unpack_buf_size));
}

void gd_set_rsd_config(gds_rsd_config *cfg)
{
   unpack_flags = cfg->flags;
   grd_unpack_buf = cfg->buf;
   grd_unpack_buf_size = cfg->size;
   unpack_buf_cap = cfg->cap;
}

void gd_get_rsd_config(gds_rsd_config *cfg)
{
   cfg->flags = unpack_flags;
   cfg->buf = grd_unpack_buf;
   cfg->size = grd_unpack_buf_size;
   cfg->cap = unpack_buf_cap;
}

/*************************************************/
/* Puts 0's in place of skips and pads with 0's. */
/* i.e., grd_unpack_buf is entirely overwritten. */
/*************************************************/
int gd_rsd8_convert(grs_bitmap *sbm, grs_bitmap *dbm)
{
   short x_right,y_bot;                /* opposite edges of bitmap */
   short x,y;                          /* current position */
   int over_run;                       /* unwritten right edge */
   uchar *p_dst;
   uchar *rsd_src;                     /* rsd source buffer */
   short rsd_code;                     /* last rsd opcode */
   short rsd_count;                    /* count for last opcode */
   short op_count;                     /* operational count */
   int size = sbm->row * sbm->h;

   if (sbm->type != BMT_RSD8)
      return GR_UNPACK_RSD8_NOTRSD;

   if (grd_unpack_buf==NULL) {
      if (!(unpack_flags&RCF_ALLOCATE)) {
         CriticalMsg("gd_rsd8_convert(): grd_unpack_buf is NULL!\n");
         return GR_UNPACK_RSD8_NOBUF;
      }
      alloc_buffer(size);
   }
   else if (grd_unpack_buf_size<size) {
      if (!(unpack_flags&RCF_ALLOCATE)) {
         CriticalMsg4("%sgrd_unpack_buf_size:%i, sbm->row: %i, sbm->h:%i\n",
            "gd_rsd8_convert(): grd_unpack_buf too small\n!",
            grd_unpack_buf_size, sbm->row, sbm->h);
         return GR_UNPACK_RSD8_NOBUF;
      }
      Free(grd_unpack_buf);
      alloc_buffer(size);
   }
   memcpy (dbm, sbm, sizeof (*sbm));
   if (sbm->flags&BMF_TLUC8)
      dbm->type = BMT_TLUC8;
   else
      dbm->type = BMT_FLAT8;
   dbm->bits = grd_unpack_buf;
   if (dbm->w==dbm->row) p_dst=gd_rsd8_unpack(sbm->bits, dbm->bits);
   else {
      rsd_src = sbm->bits;
      x = y = rsd_count = 0;
      x_right = sbm->w;
      y_bot = sbm->h;
      p_dst = dbm->bits;
      over_run=dbm->row-sbm->w;
     
      /* process each scanline, keeping track of x and splitting opcodes that
         span across more than one line. */
      while (y < y_bot) {
         /* do enough opcodes to get to the end of the current scanline. */
         while (x < x_right) {
            if (rsd_count == 0)
               RSD_GET_TOKEN ();
            if (x+rsd_count <= x_right) {
               /* current code is all on this scanline. */
               switch (rsd_code) {
               case RSD_RUN:
                  memset (p_dst, *rsd_src, rsd_count);
                  rsd_src++;
                  break;
               case RSD_SKIP:
                  memset (p_dst, 0, rsd_count);
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
               /* code goes over to next scanline, do the amount that will fit
                  on this scanline, put off rest till next line. */
               op_count = x_right-x;
               switch (rsd_code) {
               case RSD_RUN:
                  memset (p_dst, *rsd_src, op_count);
                  break;
               case RSD_SKIP:
                  memset (p_dst, 0, op_count);
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
         x -= sbm->w;
         if (over_run) {
            memset (p_dst, 0, over_run);
            p_dst += over_run;
         }
         y++;
      }
   }
rsd_done:
   if ((over_run=(dbm->bits+dbm->row*dbm->h)-p_dst)!=0)
      memset (p_dst, 0, over_run);
   return GR_UNPACK_RSD8_OK;
}
   
