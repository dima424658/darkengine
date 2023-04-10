/*
 * $Source: x:/prj/tech/libsrc/dev2d/RCS/tluctab.h $
 * $Revision: 1.4 $
 * $Author: KEVIN $
 * $Date: 1997/01/22 12:54:25 $
 *
 * Declarations and macros for translucency
 * table generation.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef _TLUCTAB
#define _TLUCTAB

#include <grd.h>
#include <grmalloc.h>
#include <alpha.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void gr_init_clut16_vector(ushort *clut16, int color, alpha_vector *opacity, int format);
extern void gr_init_clut16(ushort *clut16, int color, alpha_real opacity, int format);
extern void gr_init_clut565(ushort *clut16, int color, float opacity);
extern void gr_init_clut555(ushort *clut16, int color, float opacity);

extern uchar *gr_init_simple_translucency_table(uchar *p, fix opacity, grs_rgb color, uchar *pal);
extern uchar *gr_init_translucency_table(uchar *p, fix opacity, fix purity, grs_rgb color);
extern uchar *gr_init_lit_translucency_table(uchar *p, fix opacity, fix purity, grs_rgb color, grs_rgb light);
extern uchar *gr_init_lit_translucency_tables(uchar *p, fix opacity, fix purity, grs_rgb color, int n);

extern int gr_dump_tluc8_table(uchar *buf, int nlit);
extern void gr_read_tluc8_table(uchar *buf);

#define gr_alloc_translucency_table(n) \
   ((uchar *)gr_malloc(n*256))
#define gr_free_translucency_table(tab) (gr_free(tab))

#define gr_make_translucency_table(op, pu, co) \
  (gr_init_translucency_table(gr_alloc_translucency_table(1), op, pu, co))
#define gr_make_lit_translucency_table(op, pu, co, li) \
  (gr_init_translucency_table(gr_alloc_translucency_table(1), op, pu, co, li))
#define gr_make_lit_translucency_tables(op, pu, co, lnum) \
  (gr_init_lit_translucency_tables(gr_alloc_translucency_table(lnum), op, pu, co, lnum))

#define gr_make_tluc8_table(num, op, pu, co) \
   (tluc8tab[num]=gr_make_translucency_table(op, pu, co))
#define gr_make_lit_tluc8_table(num, op, pu, co, li) \
   (tluc8ltab[num]=gr_make_lit_translucency_tables(op, pu, co, li), \
    gr_make_tluc8_table(num, op, pu, co))
#define gr_alloc_tluc8_spoly_table(num) \
   (tluc8nstab=num, tluc8stab=gr_alloc_translucency_table(num))
#define gr_init_tluc8_spoly_table(num, op, pu, co, li) \
   (gr_init_lit_translucency_table(tluc8stab+(256*num), op, pu, co, li))
#define gr_init_tluc8_spoly_tables(num, op, pu, co, li) \
   (gr_init_lit_translucency_tables(tluc8stab+(256*num), op, pu, co, li))
#define gr_bind_tluc8_table(num, p) (tluc8tab[num]=p)
#define gr_bind_lit_tluc8_table(num, p) (tluc8ltab[num]=p)
#define gr_bind_tluc8_spoly_table(p) (tluc8stab=p)

#ifdef __cplusplus
};
#endif
#endif
