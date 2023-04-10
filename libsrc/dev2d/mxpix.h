/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxpix.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:58:20 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#ifndef __MXPIX_H
#define __MXPIX_H
#include <pixftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdupix_func modex_norm_upix8;
extern gdupix_func modex_clut_upix8;
extern gdupix_func modex_solid_upix8;
extern gdupix_func modex_xor_upix8;
extern gdupix_func modex_tluc_upix8;

extern gdupix_func modex_norm_upix16;
extern gdupix_func modex_clut_upix16;
extern gdupix_func modex_solid_upix16;
extern gdupix_func modex_xor_upix16;
extern gdupix_func modex_tluc_upix16;

#ifdef __cplusplus
};
#endif
#endif
