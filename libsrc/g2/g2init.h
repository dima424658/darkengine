/*
 * $Source: x:/prj/tech/libsrc/g2/RCS/init.h $
 * $Revision: 1.4 $
 * $Author: KEVIN $
 * $Date: 1996/08/29 11:09:09 $
 *
 * This file is part of the g2 library.
 *
 */

#ifndef __G2INIT_H
#define __G2INIT_H
#include <dev2d.h>

extern int g2_init();
extern int g2_svga_init();

#ifdef __DOS4G
#define gr_init g2_svga_init
#else
#define gr_init g2_init
#endif

extern int g2_close();
#define gr_close g2_close

// obsolete
#define g2_com_init g2_init
#define gr_com_init g2_init
#define gr_tnova_init g2_svga_init
#define gr_flight_init g2_svga_init
#define gr_flat8_init g2_svga_init
#define gr_vga_init g2_svga_init

#endif
