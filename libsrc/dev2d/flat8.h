/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/flat8.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:36:39 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __FLAT8_H
#define __FLAT8_H

#ifdef __cplusplus
extern "C" {
#endif

extern void flat8_ubitmap();
extern void flat8_ubitmap_expose();

extern void flat8_ugpix8();
extern void flat8_ugpix8_expose();
extern void flat8_ugpix16();
extern void flat8_ugpix16_expose();
extern void flat8_ugpix24();
extern void flat8_ugpix24_expose();

extern void flat8_upix8();
extern void flat8_upix8_expose();
extern void flat8_upix16();
extern void flat8_upix16_expose();
extern void flat8_upix24();
extern void flat8_upix24_expose();

extern void flat8_lpix8();
extern void flat8_lpix8_expose();
extern void flat8_lpix16();
extern void flat8_lpix16_expose();
extern void flat8_lpix24();
extern void flat8_lpix24_expose();

extern void flat8_uhline();
extern void flat8_uhline_expose();
extern void flat8_uvline();
extern void flat8_uvline_expose();

#ifdef __cplusplus
};
#endif
#endif
