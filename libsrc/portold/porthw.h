/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/libsrc/portold/porthw.h,v 1.3 1998/02/20 17:09:18 KEVIN Exp $
#ifndef __PORTHW_H
#define __PORTHW_H

#include <r3ds.h>
#include <wrdb.h>

typedef struct hw_render_info {
   r3s_texture tex, lm;          // pointers to base texture and
                                 // lightmap texture (possibly an aggregation of many lightmaps)
   float lm_u0, lm_v0;           // offset within lm to start of lightmap
                                 // always at least 0.5 to facilitate bilinear
                                 // filtering
   int flags;
   int mip_level;
} hw_render_info;

#define HWRIF_NOT_LIGHT 1
#define HWRIF_SKY       2
#define HWRIF_WATER     4

// True when lightmap aggregation enabled
extern BOOL pt_aggregate_lightmaps;

// invalidate a lightmap (e.g., when we want to animate)
extern void porthw_invalidate_cached_lightmap(int n);

// sets hw->lm, hw->lm_u0, and hw->lm_v0;
// allocates and frees lightmaps as necessary
extern void porthw_get_cached_lightmap(hw_render_info *hw,
      PortalPolygonRenderInfo *render,
      PortalLightMap *lt);


extern ushort *pt_alpha_pal;
#endif
