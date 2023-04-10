/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8lin.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:35:54 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */


#ifndef __FL8LIN_H
#define __FL8LIN_H
#include <linftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdulin_func flat8_norm_uvline;
extern gdulin_func flat8_clut_uvline;
extern gdulin_func flat8_solid_uvline;
extern gdulin_func flat8_xor_uvline;
extern gdulin_func flat8_tluc_uvline;

extern gdulin_func flat8_norm_uhline;
extern gdulin_func flat8_clut_uhline;
extern gdulin_func flat8_solid_uhline;
extern gdulin_func flat8_xor_uhline;
extern gdulin_func flat8_tluc_uhline;

#ifdef __cplusplus
};
#endif
#endif
