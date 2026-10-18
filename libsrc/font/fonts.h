// $Header: x:/prj/tech/libsrc/font/RCS/fonts.h 1.2 1997/01/23 14:06:10 KEVIN Exp $

#ifndef __FONTS_H
#define __FONTS_H

#include <stddef.h>
#include <types.h>
#include <dev2d.h>

#pragma pack(push, 1)
typedef struct grs_font {
   ushort   id;
   uchar    flags;           // special extended flags, EXF_XXX
   uchar    pal_id;          // palette id for 8 bit fonts on 16+ bit canvas
   short    spacing;         // fractional spacing
   int32    kernOffset;      // offset to kerning table
   short    vertAdjust;      // Vertical spacing adjustment, added to h
   char     dummy1[24];      // all 0 of 0x0A in case of anti-alias
   short    min;
   short    max;
   char     dummy2[32];
   int32    cotptr;
   int32    buf;
   short    w;
   short    h;
   short    off_tab[1];
} grs_font;

// Font resources contain this header verbatim.  C long is 64-bit on LP64
// hosts, but the file format and the original Windows ABI are always 32-bit.
typedef char grs_font_disk_header_must_be_84_bytes[
   offsetof(grs_font, off_tab) == 84 ? 1 : -1];
#pragma pack(pop)

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
