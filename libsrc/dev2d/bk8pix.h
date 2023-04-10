/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8pix.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:42:39 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#ifndef __FL8PIX_H
#define __FL8PIX_H
#include <pixftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdupix_func bank8_norm_upix8;
extern gdupix_func bank8_clut_upix8;
extern gdupix_func bank8_solid_upix8;
extern gdupix_func bank8_xor_upix8;
extern gdupix_func bank8_tluc_upix8;

extern gdupix_func bank8_norm_upix16;
extern gdupix_func bank8_clut_upix16;
extern gdupix_func bank8_solid_upix16;
extern gdupix_func bank8_xor_upix16;
extern gdupix_func bank8_tluc_upix16;

#ifdef __cplusplus
};
#endif
#endif
