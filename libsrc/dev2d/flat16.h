/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/flat16.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:36:21 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __FLAT16_H
#define __FLAT16_H

#ifdef __cplusplus
extern "C" {
#endif

extern void flat16_ubitmap();
extern void flat16_ubitmap_expose();

extern void flat16_ugpix8();
extern void flat16_ugpix8_expose();
extern void flat16_ugpix16();
extern void flat16_ugpix16_expose();
extern void flat16_ugpix24();
extern void flat16_ugpix24_expose();

extern void flat16_upix8();
extern void flat16_upix8_expose();
extern void flat16_upix16();
extern void flat16_upix16_expose();
extern void flat16_upix24();
extern void flat16_upix24_expose();

extern void flat16_lpix8();
extern void flat16_lpix8_expose();
extern void flat16_lpix16();
extern void flat16_lpix16_expose();
extern void flat16_lpix24();
extern void flat16_lpix24_expose();

extern void flat16_uhline();
extern void flat16_uhline_expose();
extern void flat16_uvline();
extern void flat16_uvline_expose();

#ifdef __cplusplus
};
#endif
#endif
