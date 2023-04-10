/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/f16lin.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:35:07 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#ifndef __F16LIN_H
#define __F16LIN_H
#include <linftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdulin_func flat16_norm_uvline;
extern gdulin_func flat16_clut_uvline;
extern gdulin_func flat16_solid_uvline;
extern gdulin_func flat16_xor_uvline;
extern gdulin_func flat16_tluc_uvline;

extern gdulin_func flat16_norm_uhline;
extern gdulin_func flat16_clut_uhline;
extern gdulin_func flat16_solid_uhline;
extern gdulin_func flat16_xor_uhline;
extern gdulin_func flat16_tluc_uhline;
#endif
