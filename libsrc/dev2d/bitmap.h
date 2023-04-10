// $Header: x:/prj/tech/libsrc/dev2d/RCS/bitmap.h 1.7 1998/03/12 10:40:16 KEVIN Exp $

#ifndef __BITMAP_H
#define __BITMAP_H
#include <grs.h>

#ifdef __cplusplus
extern "C" {
#endif

/* bitmap types. */
/*
   for compatability with old data, we _must_ have
      BMT_MONO = 1, BMT_FLAT8=2, BMT_RSD8 = 4
 */
enum {
   BMT_MODEX,
   BMT_MONO,
   BMT_FLAT8,
   BMT_FLAT16,
   BMT_RSD8,
   BMT_FLAT24,
   BMT_BANK8,
   BMT_BANK16,
   BMT_BANK24,
   BMT_TLUC8,
   BMT_TYPES
};
/*
   BMT_DEVICE_VIDMEM and BMT_DEVICE_VIDMEM_TO_FLAT are hacks for gr_init_bm()
   to produce bitmaps of the appropriate type for the current mode.
*/
#define BMT_DEVICE_VIDMEM BMT_TYPES
#define BMT_DEVICE_VIDMEM_TO_FLAT BMT_TYPES+1

/* bitmap flags. */
enum {
   BMF_OPAQUE,
   BMF_TRANS,
   BMF_TYPES
};
#define BMF_TLUC8          0x2
#define BMF_TLUC           0x2
#define BMF_DEVICE_VIDMEM  0x4
#define BMF_DEVICE_SYSMEM  0x8
#define BMF_DEVICE         (BMF_DEVICE_VIDMEM | BMF_DEVICE_SYSMEM)
#define BMF_DEV_MUST_BLIT 0x10
#define BMF_POW2          0x20
#define BMF_RELOCATABLE 0x8000

/* additional rgb format "flags" for upper 8 bits of flags field */
#define BMF_RGB_INDEX    0x000
#define BMF_RGB_332      0x100
#define BMF_RGB_1555     0x200
#define BMF_RGB_555      0x300
#define BMF_RGB_565      0x400
#define BMF_RGB_888      0x500
#define BMF_RGB_8888     0x600
#define BMF_RGB_4444     0x700
#define BMF_RGB_1232     0x800

#define gr_test_bitmap_format(bm, format) (((bm)->flags&0x7f00) == (format))
#define gr_set_bitmap_format(bm, format) (bm)->flags = (ushort )(((bm)->flags&0x80ff)|(format))

/* function prototypes for bitmap routines. */
extern void gr_init_bitmap
   (grs_bitmap *bm, uchar *p, uint type, uint flags, int w, int h);
extern void gr_init_sub_bitmap
   (grs_bitmap *sbm, grs_bitmap *dbm, int x, int y, int w, int h);
extern void gr_init_sub_bitmap_unsafe
   (grs_bitmap *sbm, grs_bitmap *dbm, int x, int y, int w, int h);
extern grs_bitmap *gr_alloc_bitmap
   (uint type, uint flags, int w, int h);
extern void gr_calc_sub_bitmap
   (grs_bitmap *bm, int x, int y);

/* returns the flat memory model type for the given bitmap type */
extern uint gr_bm_type_to_flat(uint type);

/* must call this under windows when done using a bitmap that points to direct draw memory. */
extern void gr_close_bitmap(grs_bitmap *bm);

#define gr_close_bm gr_close_bitmap
#define gr_close_sub_bitmap gr_close_bitmap
#define gr_close_sub_bm gr_close_bitmap

/* compatibility defines. */
#define gr_init_bm gr_init_bitmap
#define gr_init_sub_bm gr_init_sub_bitmap
#define gr_init_sub_bm_unsafe gr_init_sub_bitmap_unsafe
#define gr_alloc_bm gr_alloc_bitmap

#ifdef __cplusplus
};
#endif
#endif /* !__BITMAP_H */
