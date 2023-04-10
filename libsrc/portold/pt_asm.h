/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/pt_asm.h,v 1.6 1996/11/05 18:48:24 buzzard Exp $

// accessing functions for all of the assembly stuff, private

// basically we never call these directly, we just stuff
// "function pointers" to them which are only called from
// assembly in the first place.

// however, the "setup" functions ARE called from C


  // n-pixel mappers
extern void pt_unlit_n_asm(void);
extern void pt_pallit_n_asm(void);
extern void pt_palflat_n_asm(void);
extern void pt_lit_n_asm(void);
extern void pt_dest_pallit_n_asm(void);
extern void pt_unlit_arb_n_asm(void);
extern void pt_unlit_256_n_asm(void);

  // n-pixel filters
extern void pt_clut_n_asm(void);
extern void pt_transp_n_asm(void);
extern void pt_tluc_n_asm(void);

  // n-pixel self-modify setup
extern void pt_unlit_n_setup(void);
extern void pt_pallit_n_setup(void);
extern void pt_palflat_n_setup(void);
extern void pt_lit_n_setup(void);
extern void pt_dest_pallit_n_setup(void);
extern void pt_unlit_256_n_setup(void);

  // 8-pixel mappers
extern void pt_unlit_8_asm(void);
extern void pt_palflat_8_asm(void);
extern void pt_palflat_direct_8_asm(void);
extern void pt_lit_8_asm(void);
extern void pt_dest_pallit_8_asm(void);
extern void pt_unlit_arb_8_asm(void);

  // 8-pixel filters
extern void pt_pallit_8_asm(void);
extern void pt_pallit_store_8_asm(void);
extern void pt_clut_8_asm(void);
extern void pt_pallit_clut_store_8_asm(void);
extern void pt_transp_8_asm(void);
extern void pt_tluc_8_asm(void);

  // 8-pixel wrappers
extern void pt_generic_8_asm(void);
extern void pt_generic_preload_8_asm(void);

   // 8-pixel self-modify setup
extern void pt_unlit_8_setup(void);
extern void pt_pallit_8_setup(void);
extern void pt_palflat_8_setup(void);
extern void pt_lit_8_setup(void);

   // arbitrary length perspective runs
extern void pt_lit_perspective_run_asm(void);
extern void pt_unlit_perspective_run_asm(void);

   // 8*n pixel perspective core
extern void pt_lit_perspective_core_asm(void);
extern void pt_pallit_perspective_core_asm(void);
extern void pt_unlit_perspective_core_asm(void);
extern void pt_unlit_perspective_unrolled_asm(void);

