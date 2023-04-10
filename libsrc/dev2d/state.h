/*
 * $Source: s:/prj/tech/libsrc/dev2d/RCS/state.h $
 * $Revision: 1.1 $
 * $Author: KEVIN $
 * $Date: 1996/04/10 17:01:01 $
 *
 * Declarations for video state push/pop.
 *
 * This file is part of the dev2d library.
 *
 */

#ifndef STATE_H
#define STATE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _VideoState {
   ushort type;
   ushort mode;                  // has linear bit set if necessary
   struct {
      uchar red;                 // these values are either
      uchar green;               // stored as 6 bit or 8 bit
      uchar blue;                // depending on whether is text
   } pal[256];                   // or graphics and if have 8 bit dac
   union {
      struct {
         uchar modeFlags;        // several bits describing text mode
         uchar palRegs[17];      // the 17 palette registers for this mode
      } textInfo; // text mode info (18 bytes)
      struct {
         uchar is2dMode;         // whether we call 2d to set mode (and other stuff)

      // If level -1 then must use vga bios to set the mode, if level 0
      // then can use vesa to set mode, but can not set dacWidth,
      // pixelsPerScanLine and need to use registers for setting the focus.
      // If 1 then can use vesa for for setting dacWidth, bytesPerScanLine,
      // and focus, but not for palette, and for level 2 can do all these
      // plus set the palette.  Note: can't really set dacWidth with 1.1,
      // but shouldn't matter as we set it only if isn't 6 and function
      // should just fail if we try to read the depth.

         char vesaLevel;         // -1 for none, 0 for 1.0, 1 for 1.1, or 1.2, 2 for 2.0 or better
         
         int pixelsPerScanLine;  // indicate how many pixels across each scan line is
      // NOTE: xFocus and yFocus are not currently set!!!!
         int xFocus;             // first pixel in scan line to display
         int yFocus;             // first scan line to display
         uchar dacWidth;         // whether values are 6 or 8 bit
      } graphInfo; // graphics mode info
   };
} VideoState;                    // 790 bytes total

// The modeFlags are defined as follows:

#define TEXT_FONT_IS_DOUBLED     1        // bit 0 is whether fonts are doubled
#define TEXT_FONT_NUM_COLOR_SETS 2        // whether 4 or 16 color sets available
#define TEXT_FONT_HAVE_16_SETS   2        // check against this for 16 sets
#define TEXT_FONT_HAVE_4_SETS    0        // and this for 4 sets
#define TEXT_FONT_ACTIVE_SET     0xF0     // top 4 bits indicate which set
#define TEXT_FONT_ACTIVE_SHIFT   4        // shift by 4 to get the active set index

#define GRD_STATE_TEXT           0        // means text mode
#define GRD_STATE_GRAPHICS       1        // means graphics mode
#define MODE_IS_NOT_2D           0        // have non-2d mode
#define MODE_IS_2D               1        // have 2d mode

extern int gr_push_video_state();
extern int gr_pop_video_state();

#ifdef __cplusplus
};
#endif
#endif

