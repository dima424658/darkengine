/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/fl8bm.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:35:38 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __FL8BM_H
#define __FL8BM_H

#include <bmftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdubm_func flat8_mono_trans_ubitmap;
extern gdubm_func flat8_mono_trans_clut_ubitmap;
extern gdubm_func flat8_mono_trans_xor_ubitmap;
extern gdubm_func flat8_mono_trans_blend_ubitmap;
extern gdubm_func flat8_mono_trans_solid_ubitmap;

extern gdubm_func flat8_mono_opaque_ubitmap;
extern gdubm_func flat8_mono_opaque_clut_ubitmap;
extern gdubm_func flat8_mono_opaque_xor_ubitmap;
extern gdubm_func flat8_mono_opaque_blend_ubitmap;
extern gdubm_func flat8_mono_opaque_solid_ubitmap;

extern gdubm_func flat8_flat8_trans_ubitmap;
extern gdubm_func flat8_flat8_trans_clut_ubitmap;
extern gdubm_func flat8_flat8_trans_xor_ubitmap;
extern gdubm_func flat8_flat8_trans_blend_ubitmap;
extern gdubm_func flat8_flat8_trans_solid_ubitmap;

extern gdubm_func flat8_flat8_opaque_ubitmap;
extern gdubm_func flat8_flat8_opaque_clut_ubitmap;
extern gdubm_func flat8_flat8_opaque_xor_ubitmap;
extern gdubm_func flat8_flat8_opaque_blend_ubitmap;
extern gdubm_func flat8_flat8_opaque_solid_ubitmap;

extern gdubm_func flat8_bank8_trans_ubitmap;
extern gdubm_func flat8_bank8_trans_clut_ubitmap;
extern gdubm_func flat8_bank8_trans_xor_ubitmap;
extern gdubm_func flat8_bank8_trans_blend_ubitmap;
extern gdubm_func flat8_bank8_trans_solid_ubitmap;

extern gdubm_func flat8_bank8_opaque_ubitmap;
extern gdubm_func flat8_bank8_opaque_clut_ubitmap;
extern gdubm_func flat8_bank8_opaque_xor_ubitmap;
extern gdubm_func flat8_bank8_opaque_blend_ubitmap;
extern gdubm_func flat8_bank8_opaque_solid_ubitmap;

extern gdubm_func flat8_modex_trans_ubitmap;
extern gdubm_func flat8_modex_trans_clut_ubitmap;
extern gdubm_func flat8_modex_trans_xor_ubitmap;
extern gdubm_func flat8_modex_trans_blend_ubitmap;
extern gdubm_func flat8_modex_trans_solid_ubitmap;

extern gdubm_func flat8_modex_opaque_ubitmap;
extern gdubm_func flat8_modex_opaque_clut_ubitmap;
extern gdubm_func flat8_modex_opaque_xor_ubitmap;
extern gdubm_func flat8_modex_opaque_blend_ubitmap;
extern gdubm_func flat8_modex_opaque_solid_ubitmap;

extern gdubm_func flat8_rsd8_ubitmap;
extern gdubm_func flat8_rsd8_clut_ubitmap;
extern gdubm_func flat8_rsd8_xor_ubitmap;
extern gdubm_func flat8_rsd8_blend_ubitmap;
extern gdubm_func flat8_rsd8_solid_ubitmap;

extern gdubm_func flat8_tluc8_trans_ubitmap;
extern gdubm_func flat8_tluc8_trans_clut_ubitmap;
extern gdubm_func flat8_tluc8_trans_xor_ubitmap;
extern gdubm_func flat8_tluc8_trans_blend_ubitmap;
extern gdubm_func flat8_tluc8_trans_solid_ubitmap;

extern gdubm_func flat8_tluc8_opaque_ubitmap;
extern gdubm_func flat8_tluc8_opaque_clut_ubitmap;
extern gdubm_func flat8_tluc8_opaque_xor_ubitmap;
extern gdubm_func flat8_tluc8_opaque_blend_ubitmap;
extern gdubm_func flat8_tluc8_opaque_solid_ubitmap;

#ifdef __cplusplus
};
#endif
#endif
