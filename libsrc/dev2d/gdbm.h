/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/gdbm.h $
 * $Revision: 1.4 $
 * $Author: TOML $
 * $Date: 1996/10/16 16:06:50 $
 *
 * Bitmap blitting prototypes, constants and macros.
 *
 * This file is part of the dev2d library.
 */

#ifndef __GDBM_H
#define __GDBM_H

#include <bmftype.h>
#include <grd.h>
#include <icanvas.h>

#ifdef __cplusplus
extern "C" {
#endif

#define gd_ubitmap \
   ((gdubm_func *)(gdd_canvas_table[GDC_UBITMAP]))

#define gd_ubitmap_expose \
   ((gdubm_expose_func *)(gdd_canvas_table[GDC_UBITMAP_EXPOSE]))

extern int gd_bitmap(grs_bitmap *bm, int x, int y);
extern int gd_bitmap_opt(grs_bitmap *bm, int x, int y, gdubm_func *ubm_func);

extern void gd_get_bitmap(grs_bitmap *bm, int x, int y);

extern void gd_hflip_in_place(grs_bitmap *bm);

extern int gd_rsd8_convert(grs_bitmap *sbm, grs_bitmap *dbm);

extern uchar *gd_rsd8_unpack(uchar* src, uchar *dst);

enum {
   GR_UNPACK_RSD8_OK,
   GR_UNPACK_RSD8_NOBUF,
   GR_UNPACK_RSD8_NOTRSD,
};


#ifdef __cplusplus
};
#endif
#endif
