// $Header: x:/prj/tech/libsrc/dev2d/RCS/grd.h 1.9 1998/03/26 10:40:26 KEVIN Exp $
// Global data.

#ifndef __GRD_H
#define __GRD_H
#include <grs.h>
#include <rgb.h>

#ifdef __cplusplus
extern "C" {
#endif
extern int gdd_active;

extern grs_sys_info grd_info;
extern int grd_mode;
extern grs_drvcap grd_mode_cap;
extern grs_drvcap *grd_cap;

extern int grd_valloc_mode;
extern int grd_screen_row;
extern int grd_screen_bmt;
extern int grd_screen_max_height;

extern grs_screen *grd_screen;
extern grs_canvas *grd_screen_canvas;
extern grs_canvas *grd_visible_canvas;
extern grs_canvas *grd_back_canvas;

extern uchar **tluc8tab;

extern grs_canvas *grd_canvas;

extern void (**grd_device_table)();

extern void (**gdd_flat8_canvas_table)();
extern void (**gdd_flat16_canvas_table)();
extern void (**gdd_flat24_canvas_table)();
extern void (**gdd_bank8_canvas_table)();
extern void (**gdd_bank16_canvas_table)();
extern void (**gdd_bank24_canvas_table)();
extern void (**gdd_modex_canvas_table)();
extern void (**gdd_dispdev_canvas_table)();

extern void (**gdd_canvas_table)();

extern grs_context grd_defgc;

#define grd_bm (grd_canvas->bm)
#define grd_gc (grd_canvas->gc)
#define grd_ytab (grd_canvas->ytab)
#define grd_int_clip (grd_gc.clip.i)
#define grd_fix_clip (grd_gc.clip.f)
#define grd_clip (grd_int_clip)

extern uchar *grd_pal;
extern uchar *grd_ipal;
extern grs_rgb *grd_bpal;

extern ushort **grd_ltab816_list;
extern ushort *grd_ltab816;
extern uchar *grd_light_table;
extern int grd_light_table_size;
extern uchar *grd_unpack_buf;
extern uint grd_unpack_buf_size;
extern void *pixpal;
extern uint grd_chroma_key;
extern int (*gdd_detect_func)(grs_sys_info *info);

#ifdef __cplusplus
};
#endif
#endif /* !__GRD_H */
