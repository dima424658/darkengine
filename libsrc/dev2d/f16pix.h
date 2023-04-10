/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16pix.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:35:22 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#ifndef __F16PIX_H
#define __F16PIX_H
#include <pixftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdupix_func flat16_norm_upix8;
extern gdupix_func flat16_clut_upix8;
extern gdupix_func flat16_solid_upix8;
extern gdupix_func flat16_xor_upix8;
extern gdupix_func flat16_tluc_upix8;

extern gdupix_func flat16_norm_upix16;
extern gdupix_func flat16_clut_upix16;
extern gdupix_func flat16_solid_upix16;
extern gdupix_func flat16_xor_upix16;
extern gdupix_func flat16_tluc_upix16;
#endif
