// $Header: x:/prj/tech/libsrc/dev2d/RCS/mode.h 1.6 1997/11/21 11:56:32 KEVIN Exp $

#ifndef __MODE_H
#define __MODE_H
#include <grd.h>
#ifdef __cplusplus
extern "C" {
#endif

#define GRM_FIRST_SVGA_MODE GRM_640x350x4

// Translate 'x' to 'X' from mode list in emode.h

enum {
   GRM_320X200X8,
   GRM_320X200X8X,
   GRM_320X400X8,
   GRM_320X240X8,
   GRM_320X480X8,

   GRM_640X350X4,
   GRM_640X400X4,
   GRM_640X480X4,
   GRM_800X600X4,
   GRM_1024X768X4,
   GRM_1280X1024X4,
   GRM_1600X1200X4,

   GRM_320X200X8S,
   GRM_320X240X8S,
   GRM_320X400X8S,
   GRM_360X200X8,
   GRM_360X240X8,
   GRM_360X400X8,
   GRM_640X350X8,
   GRM_640X400X8,
   GRM_640X480X8,
   GRM_800X600X8,
   GRM_1024X768X8,
   GRM_1280X1024X8,
   GRM_1600X1200X8,

   GRM_320X200X15,
   GRM_320X240X15,
   GRM_320X400X15,
   GRM_360X200X15,
   GRM_360X240X15,
   GRM_360X400X15,
   GRM_640X350X15,
   GRM_640X400X15,
   GRM_640X480X15,
   GRM_800X600X15,
   GRM_1024X768X15,
   GRM_1280X1024X15,
   GRM_1600X1200X15,

   GRM_320X200X16,
   GRM_320X240X16,
   GRM_320X400X16,
   GRM_360X200X16,
   GRM_360X240X16,
   GRM_360X400X16,
   GRM_640X350X16,
   GRM_640X400X16,
   GRM_640X480X16,
   GRM_800X600X16,
   GRM_1024X768X16,
   GRM_1280X1024X16,
   GRM_1600X1200X16,

   GRM_320X200X24,
   GRM_320X240X24,
   GRM_320X400X24,
   GRM_360X200X24,
   GRM_360X240X24,
   GRM_360X400X24,
   GRM_640X350X24,
   GRM_640X400X24,
   GRM_640X480X24,
   GRM_800X600X24,
   GRM_1024X768X24,
   GRM_1280X1024X24,
   GRM_1600X1200X24,

   GRM_320X200X32,
   GRM_320X240X32,
   GRM_320X400X32,
   GRM_360X200X32,
   GRM_360X240X32,
   GRM_360X400X32,
   GRM_640X350X32,
   GRM_640X400X32,
   GRM_640X480X32,
   GRM_800X600X32,
   GRM_1024X768X32,
   GRM_1280X1024X32,
   GRM_1600X1200X32,

   GRM_400X300X8,
   GRM_512X384X8,
   GRM_400X300X15,
   GRM_512X384X15,
   GRM_400X300X16,
   GRM_512X384X16,
   GRM_400X300X24,
   GRM_512X384X24,
   GRM_400X300X32,
   GRM_512X384X32,

};

// Define the flags to send gr_set_mode.  Done so that TRUE (=1) and
// FALSE (=0) still work as they used to.

#define MODE_CLEAR_BIT  1
#define MODE_LINEAR_BIT 2

#define NO_CLEAR_BANKED 0
#define CLEAR_BANKED    1
#define NO_CLEAR_LINEAR 2
#define CLEAR_LINEAR    (MODE_CLEAR_BIT | MODE_LINEAR_BIT)

#ifdef _WIN32
#define MODE_WINDOWED      0x000000004
#define MODE_FULLSCREEN    0x000000008
#else
#define MODE_WINDOWED      0x000000000
#define MODE_FULLSCREEN    0x000000000
#endif

extern grs_mode_info grd_mode_info[];
extern int gr_set_mode (int mode, uint flags);

// Enumerated mode <--> Mode info conversions
#define gr_mode_info_from_mode(mode) (&grd_mode_info[(mode)])
extern int gr_mode_from_info(int w, int h, int bitDepth);
extern const char * gr_mode_name(int mode);

// Finds the first 2d mode with w,h, and bitdepth
// Note it doesn't care about flat, linear, banked, or anything else
// Hopefully the 2d is organized with flat first.
extern int gr_find_mode(int w, int h, int bitDepth);

// find mode from specified -1 terminated list
extern int gr_find_mode_list(int w, int h, int bitDepth, short *mode_list);

// Also checks for all the flag bits set
extern int gr_find_mode_flags(int w,int h,int bitDepth,uchar flags);

// find mode from specified -1 terminated list
extern int gr_find_mode_flags_list(int w,int h,int bitDepth,uchar flags,short *mode_list);

#define gr_mode_linear(mode) (grd_mode_info[(mode)].flags & GRM_IS_LINEAR)
#define gr_mode_supported(mode) (grd_mode_info[(mode)].flags & GRM_IS_SUPPORTED)
#define gr_mode_flat(mode) (gr_mode_linear(mode)||(mode==GRM_320x200x8))
#define grd_bpp (grd_mode_info[grd_mode].bitDepth)

#ifdef __cplusplus
};
#endif
#endif /* !__MODE_H */
