/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: x:/prj/tech/libsrc/gadget/RCS/util2d.h 1.9 1998/08/04 14:09:51 JAEMZ Exp $

#ifndef __UTIL2D_H
#define __UTIL2D_h
#pragma once

#include <res.h>
#include <2d.h>
#include <utilcomp.h>
#include <rect.h>
#include <datasrc.h>

#ifdef __cplusplus
extern "C" {
#endif

extern grs_bitmap *UtilLockBitmapRef(Ref bmap_id);
extern grs_bitmap *UtilGetCompressedPartial(Ref bmap_id, uchar *uncomp_buf, int cycles,
                                            UtilCompressState *state, UtilCompressCallback cb, grs_bitmap *bmp);
extern grs_bitmap *UtilLockBitmap(Id bmap_id);
extern int UtilDrawBitmapRef(Ref bmap_id, short x, short y);

extern grs_canvas *UtilScreen(short push);
extern void UtilClearCliprect();
extern void UtilSetCliprect(short x1, short y1, short x2, short y2);
extern int UtilSetColor(short r, short g, short b);
extern long UtilMakeColor(short r, short g, short b);

#define UTIL_NONE    0
#define UTIL_PUSH    1
#define UTIL_POP     2

extern ushort *UtilGetPalConv(Ref palref);
extern void UtilSetPalConv(Ref palref);
extern void UtilSetPalConvList(Ref palref, int n);
extern void UtilClearPalConv(Ref palref);
extern void UtilSetPal(Ref palref);

extern int UtilStringWrap16(char *s, short x, short y, char pal, short color, IDataSource *pFont, short width);

#define UtilString16(s,x,y,pal,color,font) (UtilStringWrap16(s,x,y,pal,color,font,0))

extern int UtilCountRefs(Id id);

// Some macros for making 16 bit colors (5,6,5)
#define RED_SHIFT_565 11
#define GREEN_SHIFT_565 5
#define BLUE_SHIFT_565 0
#define UtilBindRGB565(r,g,b) ((min((r),0x1F)<<RED_SHIFT_565)|(min((g),0x3F)<<GREEN_SHIFT_565)|(min((b),0x1F)<<BLUE_SHIFT_565))
#define RGB_WHITE 0xffff
#define RGB_BLACK 0
#define RGB_RED_565 UtilBindRGB565(0x1F, 0, 0)
#define RGB_GREEN_565 UtilBindRGB565(0, 0x3F, 0)
#define RGB_BLUE_565 UtilBindRGB565(0, 0, 0x1F)

#define UtilRBGExtractRed(c) (((c) & (0x1F << RED_SHIFT_565)) >> RED_SHIFT_565)
#define UtilRBGExtractGreen(c) (((c) & (0x3F << GREEN_SHIFT_565)) >> GREEN_SHIFT_565)
#define UtilRBGExtractBlue(c) (((c) & (0x1F << BLUE_SHIFT_565)) >> BLUE_SHIFT_565)

#ifdef __cplusplus
}
#endif
#endif