#ifndef _FDESC_H
#define _FDESC_H
#include <2d.h>
#include <rect.h>

// A Ref in a resource gets you a Frame Descriptor:

typedef struct FrameDesc {
   grs_bitmap bm;       // embedded bitmap, bm.bits set to NULL
   union {
      Rect updateArea;  // update area (for anims)
      Rect anchorArea;  // area to anchor sub-bitmap
      Point anchorPt;   // point to anchor from
      };
   long pallOff;        // offset to pallette
                        // bitmap's bits follow immediately
} FrameDesc;

#endif
