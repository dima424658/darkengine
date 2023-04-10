/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bank8.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:41:41 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __BANK8_H
#define __BANK8_H

#ifdef __cplusplus
extern "C" {
#endif

extern void bank8_ubitmap();
extern void bank8_ubitmap_expose();

extern void bank8_upix8();
extern void bank8_upix8_expose();

extern void bank8_upix16();
extern void bank8_upix16_expose();

extern void bank8_upix24();
extern void bank8_upix24_expose();

extern void bank8_uhline();
extern void bank8_uhline_expose();

extern void bank8_uvline();
extern void bank8_uvline_expose();

#ifdef __cplusplus
};
#endif
#endif
