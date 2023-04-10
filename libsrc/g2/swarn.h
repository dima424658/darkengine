/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/swarn.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:00:26 $
 * 
 * This file is part of the g2 library.
 *
 */

#ifndef __SWARN_H
#define __SWARN_H
#include <dev2d.h>

#ifdef gr_warn
#undef gr_warn
#endif

extern int gr_scale_warn(grs_bitmap *bm, int x, int y, int w, int h);
#define gr_warn gr_scale_warn

#endif
