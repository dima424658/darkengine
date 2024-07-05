// $Header: x:/prj/tech/libsrc/dev2d/RCS/grs.h 1.7 1998/04/28 13:47:25 KEVIN Exp $

#ifndef __GRS_H
#define __GRS_H

#include <fix.h>
#include <emode.h>

#ifdef __cplusplus
extern "C" {
#endif

/* system information structure. */
typedef struct grs_sys_info {
   uchar id_maj;     /* major id---type of graphics system */
   uchar id_min;     /* minor id---vendor */
   short memory;     /* memory in kilobytes */
   short modes[GRD_MODES+1]; /* array of modes, ends with -1 */
} grs_sys_info;

/* mode information descriptor structure.  The global structure
   contains the information which we think should hold.  All of
   it is checked, and if any does not match the data obtained
   from the vesa bios call, it is flaged as unsupported.  We also
   fill in the granularity and the linear framebuffer flag on
   init. */

typedef struct grs_mode_info {
   short mode_2d;    /* internal 2d mode value */
   short mode_vesa;  /* vesa mode number or -1 if non-vesa */
   short w;          /* screen width in mode */
   short h;          /* screen height in mode */
   uchar bitDepth;   /* number of bits per pixel */
   uchar flags;      /* internal bits defined below */
   short bankShift;  /* we save the amount to shift the bank by */
} grs_mode_info;     /* 12 bytes long */

/* Defines for the flags field in grs_mode_info */
#define GRM_IS_SUPPORTED   1     /* whether given graphics mode is supported       */
#define GRM_IS_LINEAR      2     /* whether the mode can use a linear frame buffer */
#define GRI_USE_WINA       4     /* whether winA is both read/writable             */
#define GRM_IS_MODEX       8     /* whether the mode is a mode x mode              */
#define GRM_IS_NATIVE      16    /* whether the mode is native or "emulated"       */
#define GRM_CAN_FULLSCREEN 32    /* whether the mode can be full screen            */
#define GRM_CAN_WINDOW     64    /* whether the mode can be in a window            */

/* amazing rgb type. */
typedef ulong grs_rgb;

/* hey, it's an rgb bitmask */
typedef struct grs_rgb_bitmask {
   ulong red, green, blue, alpha;
} grs_rgb_bitmask;

/* structure for bitmaps to be drawn from and to.  if a bitmap is contained
   within a larger bitmap, the row field tells how wide the containing bitmap
   is. */
typedef struct grs_bitmap {
   uchar *bits;      /* ptr to data */
   uchar type;       /* type of data in bitmap, 1-bit, 8-bit, etc */
   uchar align;      /* where data really starts */
   ushort flags;     /* whether compressed, transparent, etc */
   short w;          /* width in pixels */
   short h;          /* height */
   ushort row;       /* bytes in row of containing bitmap */
   uchar wlog;       /* log2 of w */
   uchar hlog;       /* log2 of h */
} grs_bitmap;

/* stencil element for non-rectangular clipping. */
typedef struct grs_sten_elem {
   short l;                   /* left edge of stencil */
   short r;                   /* right */
   struct grs_sten_elem *n;   /* pointer to next span in this scanline */
} grs_sten_elem;

/* stencil header for non-rectangular clipping. */
typedef struct grs_stencil {
   grs_sten_elem *elem;    /* pointer to first stencil element */
   long flags;             /* specific stencil data. */
} grs_stencil;

/* structure for clipping regions.  a clipping region can either be a simple
   rectangle (given by left,top,right,bot) or a grs_stencil, pointed to by
   sten. */
typedef union {
   struct grs_fix_clip {
      grs_stencil *sten;  /* pointer to stencil for nonrect clip region */
      fix left;            /* current clipping rectangle */
      fix top;             /* fixed-point coordinates */
      fix right;
      fix bot;
   } f;
   struct grs_int_clip {
      grs_stencil *sten;  /* pointer to stencil for nonrect clip region */
      short pad0;
      short left;          /* current clipping rectangle */
      short pad1;
      short top;           /* integral coordinates */
      short pad2;
      short right;
      short pad3;
      short bot;
   } i;
} grs_clip;

/* structure for drawing context.  the context contains data about which
   color, font attributes, filling attributes, and an embedded clipping
   region structure. */
typedef struct grs_context {
   long fcolor;      /* current drawing color */
   long bcolor;      /* background color */
   void *font;       /* font pointer */
   long text_attr;   /* attributes for text */
   long fill_type;   /* how to fill primitives */
   long fill_parm;   /* parameter for fill */
   grs_clip clip;    /* clipping region */
   grs_clip safe_clip;  /* safe clipping region */ 
} grs_context;

/* a canvas is a bitmap   drawing context. */
typedef struct grs_canvas {
   grs_bitmap  bm;   /* bitmap to draw into/read out of */
   grs_context gc;   /* graphic context */
   int *ytab;        /* pointer to an optional y table */
} grs_canvas;

/* a screen is a descriptor for a visible region of video memory. */
typedef struct grs_screen {
   grs_bitmap bm;    /* where we actually draw */
   grs_bitmap bm2;   /* second video buffer for WIN32 */
   grs_canvas *c;    /* pointer to 2 system canvases */
   short x;          /* upper left coordinates of visible */
   short y;          /* region of virtual buffer */
} grs_screen;

/* driver capability/info structure. */
typedef struct grs_drvcap {
   fix aspect;       /* fixed point aspect ratio w/h */
   short w;          /* screen width */
   short h;          /* screen height */
   uchar *vbase;     /* base video address */
   uchar *vbase2;    /* base video address */
   ulong rowBytes;   /* bytes per scanline */
} grs_drvcap;

/* 3d point structure for perspective mapper. */
typedef struct grs_point3d {
   fix x,y,z;    /* 3's */
} grs_point3d;

#ifdef __cplusplus
};
#endif
#endif /* !__GRS_H */




