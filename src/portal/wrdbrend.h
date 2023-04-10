/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/wrdbrend.h,v 1.5 2000/01/29 13:37:42 adurant Exp $
#pragma once

#ifndef __WRDBREND_H
#define __WRDBREND_H

#ifdef RGB_LIGHTING

#ifdef RGB_888
typedef struct LightmapEntry {
   union {
      uint I;
      uchar A[4];
   };
} LightmapEntry;
#else
typedef ushort LightmapEntry;
#endif

#else
typedef uchar LightmapEntry;
#endif

struct st_PortalPolygonRenderInfo
{
   mxs_vector tex_u, tex_v;
   ushort u_base,v_base;    // SFIX u&v values at the anchor vertex
   uchar texture_id;        // which texture to paint
   uchar texture_anchor;    // which vertex to anchor the texture to
   short cached_surface;

   mxs_real texture_mag;    // used in finding MIP level
   mxs_vector center;       // also used in finding MIP level
};  // 48 bytes

struct st_PortalLightMap {
   short base_u, base_v; // these can be negative, they indicate top
                         // left of rectangle
   short pixel_row;      // bitmapesque data, hey, this is in pixels not bytes!
   uchar h, w;

// i changed these names to make sure I found all places that referenced
// them when doing RGB support
   LightmapEntry *data;  // the actual data--the static lightmap comes first,
                         // and then the bits for all the animated lightmaps,
                         // in the order in which they appear in the cell's
                         // palette
   LightmapEntry *dynamic_light;
                         // if it's dynamically lit, this has the new data

   uint anim_light_bitmask; // Which animated lights reaching this cell
                         // affect this polygon?  The bits match the
                         // anim light palette of the cell.
}; // 20 bytes

#endif
