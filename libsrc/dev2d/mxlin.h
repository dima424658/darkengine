/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/mxlin.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:58:09 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#ifndef __MXLIN_H
#define __MXLIN_H
#include <linftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdulin_func modex_norm_uvline;
extern gdulin_func modex_clut_uvline;
extern gdulin_func modex_solid_uvline;
extern gdulin_func modex_xor_uvline;
extern gdulin_func modex_tluc_uvline;

extern gdulin_func modex_norm_uhline;
extern gdulin_func modex_clut_uhline;
extern gdulin_func modex_solid_uhline;
extern gdulin_func modex_xor_uhline;
extern gdulin_func modex_tluc_uhline;

#ifdef __cplusplus
};
#endif
#endif
