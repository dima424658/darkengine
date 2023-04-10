/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/bk8lin.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:42:31 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#ifndef __BK8LIN_H
#define __BK8LIN_H

#include <linftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdulin_func bank8_norm_uvline;
extern gdulin_func bank8_clut_uvline;
extern gdulin_func bank8_solid_uvline;
extern gdulin_func bank8_xor_uvline;
extern gdulin_func bank8_tluc_uvline;

extern gdulin_func bank8_norm_uhline;
extern gdulin_func bank8_clut_uhline;
extern gdulin_func bank8_solid_uhline;
extern gdulin_func bank8_xor_uhline;
extern gdulin_func bank8_tluc_uhline;

#ifdef __cplusplus
};
#endif
#endif
