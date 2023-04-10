/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/portal/pt.h,v 1.5 2000/01/29 13:37:30 adurant Exp $
#pragma once

//  PORTAL TEXTURE MAPPING

#ifndef __PT_H
#define __PT_H

#include <lg.h>

#ifdef __cplusplus
extern "C"
{
#endif

// generic context

   // an N x 256 table used for table-based lighting (lt x tmap)
extern uchar *pt_light_table;

   // a 256 x 256 table used for tluc8 operations (tmap x old)
extern uchar *pt_tluc_table;

   // a 256 byte color-lookup table applied to the next polygon
   // rendered if set during the r3s_texture query callback
extern uchar *pt_clut;

   // if true, does 2x2 checkerboard dithering
extern int dither;

#ifdef __cplusplus
};
#endif

#endif
