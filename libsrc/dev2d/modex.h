/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/modex.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:56:38 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __MODEX_H
#define __MODEX_H

#ifdef __cplusplus
extern "C" {
#endif

extern void modex_ubitmap();
extern void modex_ubitmap_expose();

extern void modex_upix8();
extern void modex_upix8_expose();

extern void modex_upix16();
extern void modex_upix16_expose();

extern void modex_upix24();
extern void modex_upix24_expose();

extern void modex_uhline();
extern void modex_uhline_expose();

extern void modex_uvline();
extern void modex_uvline_expose();

#ifdef __cplusplus
};
#endif
#endif
