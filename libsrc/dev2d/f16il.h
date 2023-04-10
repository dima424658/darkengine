// $Header: x:/prj/tech/libsrc/dev2d/RCS/f16il.h 1.1 1997/02/17 14:49:28 KEVIN Exp $
// flat16 inner loops

#ifndef __F16IL_H
#define __F16IL_H

extern void flat16_memset(ushort c16, ushort *dst, int count);
extern void flat16_flat16_trans_il(ushort *src, ushort *dst, int n);
extern void flat16_flat16_opaque_clut_il(ushort *p_dst, ushort *p_src, int n, ushort *clut16);
extern void flat16_flat8_opaque_inner_loop(ushort *dst, uchar *src, int count);

#endif
