// $Header: x:/prj/tech/libsrc/dev2d/RCS/gdpix.h 1.3 1997/02/17 14:49:48 KEVIN Exp $

#ifndef __GDPIX_H
#define __GDPIX_H

#include <context.h>
#include <pixftype.h>
#include <grd.h>
#include <icanvas.h>

#ifdef __cplusplus
extern "C" {
#endif

#define gd_upix \
   ((gdupix_func *)(gdd_canvas_table[GDC_UPIX]))
#define gd_upix8 \
   ((gdupix_func *)(gdd_canvas_table[GDC_UPIX8]))
#define gd_upix16 \
   ((gdupix_func *)(gdd_canvas_table[GDC_UPIX16]))
#define gd_upix24 \
   ((gdupix_func *)(gdd_canvas_table[GDC_UPIX24]))

#define gd_upix_expose \
   ((gdupix_expose_func *)(gdd_canvas_table[GDC_UPIX_EXPOSE]))
#define gd_upix8_expose \
   ((gdupix_expose_func *)(gdd_canvas_table[GDC_UPIX8_EXPOSE]))
#define gd_upix16_expose \
   ((gdupix_expose_func *)(gdd_canvas_table[GDC_UPIX16_EXPOSE]))
#define gd_upix24_expose \
   ((gdupix_expose_func *)(gdd_canvas_table[GDC_UPIX24_EXPOSE]))

extern void gd_upoint(int x, int y);
extern void gd_upoint_opt(int x, int y, gdupix_func *upix_func);

extern int gd_pix_opt(int c, int x, int y, gdupix_func *upix_func);
extern int gd_pix8(int c, int x, int y);
extern int gd_pix16(int c, int x, int y);
extern int gd_pix24(int c, int x, int y);
extern int gd_point(int x, int y);

#define gd_ugpix8 \
   ((gdgpix_func *)(gdd_canvas_table[GDC_UGPIX8]))
#define gd_ugpix16 \
   ((gdgpix_func *)(gdd_canvas_table[GDC_UGPIX16]))
#define gd_ugpix24 \
   ((gdgpix_func *)(gdd_canvas_table[GDC_UGPIX24]))

#define gd_ugpix8_expose \
   ((gdgpix_expose_func *)(gdd_canvas_table[GDC_UGPIX8_EXPOSE]))
#define gd_ugpix16_expose \
   ((gdgpix_expose_func *)(gdd_canvas_table[GDC_UGPIX16_EXPOSE]))
#define gd_ugpix24_expose \
   ((gdgpix_expose_func *)(gdd_canvas_table[GDC_UGPIX24_EXPOSE]))

extern int gr_get_pixel_bm(grs_bitmap *bm, int x, int y);
extern int gd_gpix_opt(int x, int y, gdgpix_func *ugpix_func);
extern int gd_gpix8(int x, int y);
extern int gd_gpix16(int x, int y);
extern int gd_gpix24(int x, int y);

#define gd_lpix \
   ((gdlpix_func *)(gdd_canvas_table[GDC_LPIX]))
#define gd_lpix8 \
   ((gdlpix_func *)(gdd_canvas_table[GDC_LPIX8]))
#define gd_lpix16 \
   ((gdlpix_func *)(gdd_canvas_table[GDC_LPIX16]))
#define gd_lpix24 \
   ((gdlpix_func *)(gdd_canvas_table[GDC_LPIX24]))

#define gd_lpix_expose \
   ((gdlpix_expose_func *)(gdd_canvas_table[GDC_LPIX_EXPOSE]))
#define gd_lpix8_expose \
   ((gdlpix_expose_func *)(gdd_canvas_table[GDC_LPIX8_EXPOSE]))
#define gd_lpix16_expose \
   ((gdlpix_expose_func *)(gdd_canvas_table[GDC_LPIX16_EXPOSE]))
#define gd_lpix24_expose \
   ((gdlpix_expose_func *)(gdd_canvas_table[GDC_LPIX24_EXPOSE]))

#ifdef __cplusplus
};
#endif
#endif

