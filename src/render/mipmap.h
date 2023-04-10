/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/render/mipmap.h,v 1.5 2000/01/29 13:39:14 adurant Exp $
#pragma once

#ifndef __MIPMAP_H
#define __MIPMAP_H

#include <lg.h>
#include <2d.h>
#include <r3d.h>

enum {
   MIP_MAP,
   MIP_FILTER,
   MIP_BOTH
};

EXTERN bool mipmap_enable;
EXTERN bool mipmap_dither;

  // allocate a mipmapped texture
EXTERN r3s_texture create_mipmapped_texture(grs_bitmap *src);

  // free a mipmapped texture.  if free_rect is true, it calls
  // portal_free_mem_rect for each thing.  If you're mass-clearing
  // them instead, then set free_rect to false
EXTERN void free_mipmapped_texture(r3s_texture dest, bool free_rect);

#endif
