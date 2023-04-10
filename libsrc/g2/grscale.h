/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/grscale.h $
 * $Revision: 1.4 $
 * $Author: PATMAC $
 * $Date: 1998/07/04 13:14:39 $
 *
 * Dispatch macros for rendering functions.
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __GRSCALE_H
#define __GRSCALE_H

#include <g2d.h>
#include <icanvas.h>
#include <sftype.h>

#define gr_scale_bitmap \
    ((g2s_func *)(g2d_canvas_table[G2C_SCALE]))
#define gr_scale_bitmap_expose \
    ((g2s_expose_func *)(g2d_canvas_table[G2C_SCALE_EXPOSE]))
#define gr_scale_ubitmap \
    ((g2us_func *)(g2d_canvas_table[G2C_USCALE]))
#define gr_scale_ubitmap_expose \
    ((g2us_expose_func *)(g2d_canvas_table[G2C_USCALE_EXPOSE]))

#define gr_clut_scale_bitmap(bm, x, y, w, h, clut) \
do {                                      \
   if (gr_get_fill_type() == FILL_NORM) { \
      int __fp = gr_get_fill_parm();      \
      gr_set_fill_type(FILL_CLUT);        \
      gr_set_fill_parm(clut);             \
      gr_scale_bitmap(bm, x, y, w, h);    \
      gr_set_fill_type(FILL_NORM);        \
      gr_set_fill_parm(__fp);             \
   } else {                               \
      gr_scale_bitmap(bm, x, y, w, h);    \
   }                                      \
} while (0)

#define gr_scale_hflip_ubitmap(bm, x, y, w, h) \
do {                                           \
   int __w = w;                                \
   gr_scale_ubitmap(bm, x+__w, y, -__w, h);    \
} while (0)                                    

#define gr_scale_hflip_bitmap(bm, x, y, w, h) \
do {                                          \
   int __w = w;                               \
   gr_scale_bitmap(bm, x+__w, y, -__w, h);    \
} while (0)                                    

#define gr_hflip_ubitmap(bm, x, y)   \
do {                                 \
   grs_bitmap *__bm=bm;              \
   gr_scale_hflip_ubitmap(__bm, x,   \
      y, __bm->w, __bm->h);          \
} while (0)                                    

#define gr_hflip_bitmap(bm, x, y)    \
do {                                 \
   grs_bitmap *__bm=bm;              \
   gr_scale_hflip_bitmap(__bm, x,    \
      y, __bm->w, __bm->h);          \
} while (0)                                    

EXTERN fix gr_scale_check_occlusion(grs_bitmap *bm, int x, int y, int w, int h);
EXTERN int gr_inv_tluc8_scale_bitmap(grs_bitmap *bm, int x, int y, int w, int h);

#endif /* !__GRREND_H */
