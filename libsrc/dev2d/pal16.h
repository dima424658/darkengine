/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/pal16.h $
 * $Revision: 1.4 $
 * $Author: KEVIN $
 * $Date: 1998/04/03 13:29:56 $
 *
 * Prototypes and macros for 16 bit palette manipulation routines.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __PAL16_H
#define __PAL16_H
#include <grs.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void gr_make_pal555_from_pal888(ushort *pal16, uchar *pal_data);
extern ushort *gr_alloc_pal555_from_pal888(uchar *pal_data);
extern void gr_make_pal1555_from_pal888(ushort *pal16, uchar *pal_data);
extern ushort *gr_alloc_pal1555_from_pal888(uchar *pal_data);
extern void gr_make_pal565_from_pal888(ushort *pal16, uchar *pal_data);
extern ushort *gr_alloc_pal565_from_pal888(uchar *pal_data);
extern ushort **grd_pal16_list;
extern ushort *grd_pal16;
extern uchar *grd_ipal16;
extern void gr_make_pal16(int start, int n, ushort *pal16, uchar *pal_data, grs_rgb_bitmask *bitmask);
extern void gr_set_pal16(ushort *pal, int n);
extern void gr_make_ipal1555(uchar *ipal);
extern uchar *gr_alloc_ipal1555();
extern void gr_make_ipal555(uchar *ipal);
extern uchar *gr_alloc_ipal555();
extern void gr_make_ipal565(uchar *ipal);
extern uchar *gr_alloc_ipal565();

extern void gr_compute_ltab816(ushort *ltab, uchar *pal_data, int dark_levels, int bright_levels, grs_rgb_bitmask *bitmask);

#define gr_set_default_pal16(__pal) grd_pal16 = (__pal)
#define gr_set_pal16_list(__pal_list) grd_pal16_list = (__pal_list)
#define gr_set_ipal16(__pal) grd_ipal16=(__pal)
#define GRC_OPTIMIZED_PAL16 255

#ifdef __cplusplus
};
#endif
#endif /* __PAL16_H */
