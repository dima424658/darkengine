// $Header: x:/prj/tech/libsrc/g2/RCS/tmapd.h 1.3 1997/05/16 09:38:48 KEVIN Exp $

#ifndef __TMAPD_H
#define __TMAPD_H

#include <plyparam.h>
#include <tmaps.h>
extern g2s_tmap_info g2d_tmap_info;
extern g2s_poly_params g2d_pp;
extern g2s_poly_params *g2d_ppp; // usually = &g2d_pp

// Several 8-to-16-bit inner loops use this as two adjacent scanline work
// areas (one byte of sampled texture data and one byte of intermediate
// lighting data per destination pixel).  The original 1024-byte allocation
// therefore only safely handled a 512-pixel span; a native-width 1920-pixel
// polygon overwrote neighboring renderer state.  Keep enough space for two
// full scanlines at the same modern-resolution ceiling used by scancvt.h.
#define G2C_MAX_WIDTH 8192
#define G2C_TMAP_BUFFER_SIZE (2 * G2C_MAX_WIDTH)
extern uchar g2d_tmap_buffer[G2C_TMAP_BUFFER_SIZE];
#define tmap_buffer_end (g2d_tmap_buffer+G2C_TMAP_BUFFER_SIZE)

#define POW2(bm) \
   ((bm->row==1<<bm->wlog)&&   \
    (((bm->w-1)&(bm->w))==0)&& \
    (((bm->h-1)&(bm->h))==0))

#endif
