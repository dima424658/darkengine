// $Header: x:/prj/tech/libsrc/font/RCS/fonts.h 1.2 1997/01/23 14:06:10 KEVIN Exp $

#ifndef __FONTS_H
#define __FONTS_H

#include <types.h>
#include <dev2d.h>

typedef struct grs_font {
   ushort   id;
   uchar    flags;           // special extended flags, EXF_XXX
   uchar    pal_id;          // palette id for 8 bit fonts on 16+ bit canvas
   short    spacing;         // fractional spacing
   long     kernOffset;      // offset to kerning table
   short    vertAdjust;      // Vertical spacing adjustment, added to h
   char     dummy1[24];      // all 0 of 0x0A in case of anti-alias
   short    min;
   short    max;
   char     dummy2[32];
   long     cotptr;
   long     buf;
   short    w;
   short    h;
   short    off_tab[1];
} grs_font;

typedef struct grs_font_info {
   uchar       align_mask;
   uchar       align_base;
   ushort      pad;
   gdubm_func *blitter;
   uchar      *bits;
   short      *off_tab;
   uint        range;
   grs_bitmap  bm;
} grs_font_info;

// font flags
#define FONTF_SPACING      0x02  // extra (fractional) spacing set
#define FONTF_KERNTABLE    0x04  // kerning table appended
#define FONTF_VERTADJUST   0x08  // Vertical spacing adjust

// font types
#define FONTT_MONO         0x0000
#define FONTT_ANTIALIAS16  0x0001
#define FONTT_FLAT8        0xcccc

#endif
