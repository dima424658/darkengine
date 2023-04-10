// $Header: x:/prj/tech/libsrc/g2/RCS/pt.h 1.1 1997/09/23 13:49:31 KEVIN Exp $

//  PORTAL TEXTURE MAPPING

#ifndef __PT_H
#define __PT_H

// generic context

   // an N x 256 table used for table-based lighting (lt x tmap)
extern uchar *g2pt_light_table;

   // a 256 x 256 table used for tluc8 operations (tmap x old)
extern uchar *g2pt_tluc_table;

   // a 256 byte color-lookup table applied to the next polygon
   // rendered if set during the r3s_texture query callback
extern uchar *g2pt_clut;

   // if true, does 2x2 checkerboard dithering
extern int g2pt_dither;

#endif
