// $Header: x:/prj/tech/libsrc/dev2d/RCS/blnfcn.h 1.2 1996/12/06 15:29:46 KEVIN Exp $
// prototypes for blending routines.

#ifndef __BLNFCN_H
#define __BLNFCN_H
#include <alpha.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint gr_blend_color(uint c0, uint c1, alpha_real alpha, int format);
extern uint gr_blend_color_vector(uint c0, uint c1, alpha_vector alpha, int format);

extern void gr_blend_pal16(ushort *dst, ushort *src, uint color, alpha_real alpha, int format);
extern void gr_blend_pal16_vector(ushort *dst, ushort *src, uint color, alpha_vector alpha, int format);

#ifdef __cplusplus
};
#endif
#endif /* !__BLNFCN */
