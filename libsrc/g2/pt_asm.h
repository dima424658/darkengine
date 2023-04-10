// $Header: x:/prj/tech/libsrc/g2/RCS/pt_asm.h 1.5 1998/04/06 15:42:13 KEVIN Exp $

// accessing functions for all of the assembly stuff, private

// basically we never call these directly, we just stuff
// "function pointers" to them which are only called from
// assembly in the first place.

// however, the "setup" functions ARE called from C

#ifndef __PT_ASM_H
#define __PT_ASM_H

#include <dev2d.h>

// 16 bit mappers
extern void g2pt16_unlit_n_asm(void);
extern void g2pt16_unlit_n_setup(grs_bitmap *bm);
extern void g2pt16_unlit_8_asm(void);
extern void g2pt16_unlit_8_setup(grs_bitmap *bm);
extern void g2pt816_transp_n_asm(void);
extern void g2pt816_transp_8_asm(void);
extern void g2pt816_unlit_n_asm(void);
extern void g2pt816_unlit_n_setup(grs_bitmap *bm);
extern void g2pt_pal816_8_asm(void);
extern void g2pt16_lit_n_asm(void);
extern void g2pt16_lit_n_setup(grs_bitmap *bm);
extern void g2pt16_lit_8_asm(void);
extern void g2pt16_lit_8_setup(grs_bitmap *bm);
extern void g2pt16_generic_8_asm(void);

  // n-pixel mappers
extern void g2pt_unlit_n_asm(void);
extern void g2pt_pallit_n_asm(void);
extern void g2pt_palflat_n_asm(void);
extern void g2pt_lit_n_asm(void);
extern void g2pt_dest_pallit_n_asm(void);
extern void g2pt_unlit_arb_n_asm(void);
extern void g2pt_unlit_256_n_asm(void);

  // n-pixel filters
extern void g2pt_clut_n_asm(void);
extern void g2pt_transp_n_asm(void);
extern void g2pt_tluc_n_asm(void);

  // n-pixel self-modify setup
extern void g2pt_unlit_n_setup(grs_bitmap *bm);
extern void g2pt_pallit_n_setup(void);
extern void g2pt_palflat_n_setup(void);
extern void g2pt_lit_n_setup(grs_bitmap *bm);
extern void g2pt_dest_pallit_n_setup(void);
extern void g2pt_unlit_256_n_setup(void);

  // 8-pixel mappers
extern void g2pt_unlit_8_asm(void);
extern void g2pt_palflat_8_asm(void);
extern void g2pt_palflat_direct_8_asm(void);
extern void g2pt_lit_8_asm(void);
extern void g2pt_dest_pallit_8_asm(void);
extern void g2pt_unlit_arb_8_asm(void);

  // 8-pixel filters
extern void g2pt_pallit_8_asm(void);
extern void g2pt_pallit_store_8_asm(void);
extern void g2pt_clut_8_asm(void);
extern void g2pt_pallit_clut_store_8_asm(void);
extern void g2pt_transp_8_asm(void);
extern void g2pt_tluc_8_asm(void);

  // 8-pixel wrappers
extern void g2pt_generic_8_asm(void);
extern void g2pt_generic_preload_8_asm(void);

   // 8-pixel self-modify setup
extern void g2pt_unlit_8_setup(grs_bitmap *bm);
extern void g2pt_pallit_8_setup(void);
extern void g2pt_palflat_8_setup(void);
extern void g2pt_lit_8_setup(grs_bitmap *bm);

   // arbitrary length perspective runs
extern void g2pt_lit_perspective_run_asm(void);
extern void g2pt_unlit_perspective_run_asm(void);

   // 8*n pixel perspective core
extern void g2pt_lit_perspective_core_asm(void);
extern void g2pt_pallit_perspective_core_asm(void);
extern void g2pt_unlit_perspective_core_asm(void);
extern void g2pt_unlit_perspective_unrolled_asm(void);

#endif
