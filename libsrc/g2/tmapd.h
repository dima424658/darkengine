// $Header: x:/prj/tech/libsrc/g2/RCS/tmapd.h 1.3 1997/05/16 09:38:48 KEVIN Exp $

#ifndef __TMAPD_H
#define __TMAPD_H

#include <plyparam.h>
#include <tmaps.h>
extern g2s_tmap_info g2d_tmap_info;
extern g2s_poly_params g2d_pp;
extern g2s_poly_params *g2d_ppp; // usually = &g2d_pp

#define G2C_TMAP_BUFFER_SIZE 1024
extern uchar g2d_tmap_buffer[G2C_TMAP_BUFFER_SIZE];
#define tmap_buffer_end (g2d_tmap_buffer+G2C_TMAP_BUFFER_SIZE)

#define POW2(bm) \
   ((bm->row==1<<bm->wlog)&&   \
    (((bm->w-1)&(bm->w))==0)&& \
    (((bm->h-1)&(bm->h))==0))

#endif
