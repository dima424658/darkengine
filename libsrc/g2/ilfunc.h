/*
 * $Source: s:/prj/tech/libsrc/g2/RCS/ilfunc.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/11 10:03:22 $
 * 
 * This file is part of the g2 library.
 *
 */

#ifndef __ILFUNC_H
#define __ILFUNC_H

extern void gen_flat8_il(int x, int xf, fix u, fix v);
extern void flat8_flat8_clut_trans_il(int x, int xf, fix u, fix v);
extern void flat8_flat8_clut_opaque_il(int x, int xf, fix u, fix v);
extern void flat8_flat8_norm_trans_il(int x, int xf, fix u, fix v);
extern void flat8_flat8_norm_opaque_il(int x, int xf, fix u, fix v);
extern void flat8_flat8_tluc_trans_il(int x, int xf, fix u, fix v);
extern void flat8_flat8_tluc_opaque_il(int x, int xf, fix u, fix v);

#endif
