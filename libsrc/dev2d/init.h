/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/init.h $
 * $Revision: 1.3 $
 * $Author: KEVIN $
 * $Date: 1996/08/29 09:44:38 $
 *
 * Prototypes for detection routines.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __INIT_H
#define __INIT_H

#ifdef __cplusplus
extern "C" {
#endif

extern int gd_init();
extern void gd_use_svga();
extern int gd_svga_init();

extern void gd_use_com();

extern void gd_use_no_video();
extern int gd_no_video_init();

extern void gd_use_flat();
extern int gd_flat_init();

extern void gd_use_flat8();
extern int gd_flat8_init();

extern void gd_use_flat16();
extern int gd_flat16_init();

#define dev2d_init gd_svga_init

extern void gd_release_com();

extern int gd_close();

#ifdef __cplusplus
};
#endif
#endif
