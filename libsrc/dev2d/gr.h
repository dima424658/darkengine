#ifndef __GR_H
#define __GR_H

#include <bitmap.h>
#include <canvas.h>
#include <screen.h>
#include <mode.h>
#include <fill.h>
#include <grd.h>
#include <gdbm.h>
#include <gdlin.h>
#include <gdrect.h>

#ifdef __cplusplus
extern "C" {
#endif

#define gr_bitmap gd_bitmap
#define gr_hflip_in_place gd_hflip_in_place
#define gr_mono_bitmap gd_bitmap
#define gr_flat8_bitmap gd_bitmap
#define gr_rsd8_bitmap gd_bitmap
#define gr_mask_bitmap(bm, s, x, y) \
        gd_bitmap(bm, x, y)
#define gr_ubitmap gd_ubitmap
#define gr_mono_ubitmap gd_ubitmap
#define gr_flat8_ubitmap gd_ubitmap
#define gr_rsd8_ubitmap gd_ubitmap
#define gr_mask_ubitmap(bm, s, x, y) \
        gd_ubitmap(bm, x, y)
#define gr_get_bitmap gd_get_bitmap
#define gr_rsd8_convert gd_rsd8_convert
#define gr_rsd8_unpack gd_rsd8_unpack

#define gr_clut_bitmap(bm, x, y, clut) \
do {                                       \
   if (gr_get_fill_type() == FILL_NORM) {  \
      ulong fill_save=gr_get_fill_parm();  \
      gr_set_fill_type(FILL_CLUT);         \
      gr_set_fill_parm(clut);              \
      gd_bitmap(bm, x, y);                 \
      gr_set_fill_type(FILL_NORM);         \
      gr_set_fill_parm(fill_save);         \
   } else {                                \
      gd_bitmap(bm, x, y);                 \
   }                                       \
} while(0)
#define gr_clut_ubitmap(bm, x, y, clut) \
do {                                       \
   if (gr_get_fill_type() == FILL_NORM) {  \
      ulong fill_save=gr_get_fill_parm();  \
      gr_set_fill_type(FILL_CLUT);         \
      gr_set_fill_parm(clut);              \
      gd_ubitmap(bm, x, y);                \
      gr_set_fill_type(FILL_NORM);         \
      gr_set_fill_parm(fill_save);         \
   } else {                                \
      gd_ubitmap(bm, x, y);                \
   }                                       \
} while(0)

#define gr_hline gd_hline
#define gr_vline gd_vline
#define gr_uhline gd_uhline
#define gr_uvline gd_uvline

#define gr_box gd_box
#define gr_ubox gd_ubox
#define gr_rect gd_rect
#define gr_urect gd_urect
#define gr_clear gd_clear
#define dr_scr_canv grd_screen_canvas
#define grd_scr_canv grd_screen_canvas

#define gr_fill_pixel gd_pix8
#define gr_set_pixel gd_pix8
#define gr_fill_upixel gd_upix8
#define gr_set_upixel gd_upix8
#define gr_get_pixel gd_gpix8
#define gr_get_upixel gd_ugpix8
#define gr_point gd_point
#define gr_upoint gd_upoint

#define gr_double_h_ubitmap gr_ubitmap
#define gr_double_hv_ubitmap gr_ubitmap

#ifdef __cplusplus
};
#endif
#endif
