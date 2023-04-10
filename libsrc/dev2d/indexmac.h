/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/indexmac.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 16:55:36 $
 *
 * Constants for bitmap flags & type fields; prototypes for bitmap
 * functions.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef __INDEXMAC_H
#define __INDEXMAC_H
#include <bitmap.h>
#include <fill.h>
#include <grd.h>

#ifdef __cplusplus
extern "C" {
#endif

#define make_index_bmt_fill_bmf(bm) \
   ((bm->type*FILL_TYPES + grd_gc.fill_type)*BMF_TYPES + (bm->flags&1))

#define make_index_bmt_bmf(bm) \
   bm->type*BMF_TYPES + (bm->flags&1)

#define make_index_bmt(bm) \
   bm->type

#ifdef __cplusplus
};
#endif
#endif
