/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/genbm.h $
 * $Revision: 1.2 $
 * $Author: KEVIN $
 * $Date: 1996/04/25 12:32:55 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __GENBM_H
#define __GENBM_H

#include <bmftype.h>

#ifdef __cplusplus
extern "C" {
#endif

extern gdubm_func gen_mono_trans_ubitmap;
extern gdubm_func gen_flat8_trans_ubitmap;
extern gdubm_func gen_flat16_trans_ubitmap;
extern gdubm_func gen_flat24_trans_ubitmap;
extern gdubm_func gen_bank8_trans_ubitmap;
extern gdubm_func gen_modex_trans_ubitmap;
extern gdubm_func gen_rsd8_trans_ubitmap;
extern gdubm_func gen_tluc8_trans_ubitmap;

extern gdubm_func gen_mono_opaque_ubitmap;
extern gdubm_func gen_flat8_opaque_ubitmap;
extern gdubm_func gen_flat16_opaque_ubitmap;
extern gdubm_func gen_flat24_opaque_ubitmap;
extern gdubm_func gen_bank8_opaque_ubitmap;
extern gdubm_func gen_modex_opaque_ubitmap;
extern gdubm_func gen_rsd8_opaque_ubitmap;
extern gdubm_func gen_tluc8_opaque_ubitmap;

extern gdubm_func gen_opaque_solid_ubitmap;

extern gdubm_func gen_rsd8_ubitmap;

extern gdbm_func gen_mono_bitmap;
extern gdbm_func gen_flat8_bitmap;
extern gdbm_func gen_flat16_bitmap;
extern gdbm_func gen_bank8_bitmap;
extern gdbm_func gen_modex_bitmap;
extern gdbm_func gen_rsd8_bitmap;
extern gdbm_func gen_tluc8_bitmap;

#ifdef __cplusplus
};
#endif
#endif
