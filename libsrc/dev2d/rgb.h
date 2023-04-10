/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/rgb.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:00:00 $
 *
 * Prototypes and macros for rgb manipulation routines.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __RGB_H
#define __RGB_H

#ifdef __cplusplus
extern "C" {
#endif

#define RGB_OK (0)
#define RGB_OUT_OF_MEMORY (-1)
#define RGB_CANT_DEALLOCATE (-2)
#define RGB_IPAL_NOT_ALLOCATED (-3)

/* convert fixed-point (r,g,b) triplet into a 15-bit ipal index. */
#define gr_index_rgb(r,g,b) \
   (((r)>>19)&0x1f) | (((g)>>14)&0x3e0) | (((b)>>9)&0x7c00)

/* convert an 8-8-8 long rgb into a 15-bit ipal index. */
#define gr_index_lrgb(t) \
   ((((t)>>3)&0x1f) | (((t)>>6)&0x3e0) | (((t)>>9)&0x7c00))

/* convert 8-bit r,g,b into grs_rgb format. */
#define gr_bind_rgb(r,g,b) (((r)<<2)|((g)<<13)|((b)<<24))

/* convert a grs_rgb value into a 15-bit inverse palette table index. */
#define gr_index_brgb(c) \
   ((((c)>>5)&0x1f)|(((c)>>11)&0x3e0)|(((c)>>17)&0x7c00))

/* split a grs_rgb into its component 8-bit r, g, and b values. */
#define gr_split_rgb(c, r, g, b) \
do {                       \
   *(r) = ((c)>>2)&0xff;   \
   *(g) = ((c)>>13)&0xff;  \
   *(b) = ((c)>>24)&0xff;  \
} while (0)

/* Generate an inverse palette for the given screen palette */
int gr_alloc_ipal(void);

/* Reinitialize the ipal for the current palette */
int gr_init_ipal(void);

/* Calculate an ipal for any given palette */
int gr_calc_ipal(grs_rgb *bpal, uchar *ipal);

/* Destroy the current inverse palette, freeing memory */
int gr_free_ipal(void);

#ifdef __cplusplus
};
#endif
#endif /* !__RGB_H */
