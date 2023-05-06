;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/grs.inc $
; $Revision: 1.2 $
; $Author: KEVIN $
; $Date: 1998/04/28 13:47:48 $
;
; Public 2D system data structures.
;
; This file is part of the 2d library.
;

ifndef grs_inc
grs_inc equ 1

GRS_SYS_INFO_ID_MAJ     equ     0       ;major id---type of graphics system
GRS_SYS_INFO_ID_MIN     equ     1       ;minor id---vendor
GRS_SYS_INFO_MEMORY     equ     2       ;memory in kilobytes
GRS_SYS_INFO_MODES      equ     4       ;array of modes, ends with -1
MODE_LIST_LENGTH        equ     64      ;length of array
GRS_SYS_INFO_SIZE       equ     132     ;128 for modes, 4 for other
   
; gr mode info descriptor structure.
GRS_MODE_INFO_2D_MODE   equ     0       ;internal 2d mode number
GRS_MODE_INFO_VESA_MODE equ     2       ;vesa defined mode
GRS_MODE_INFO_W         equ     4       ;mode resolution width
GRS_MODE_INFO_H         equ     6       ;mode resolution height
GRS_MODE_INFO_B         equ     8       ;bits per pixel
GRS_MODE_INFO_FLAGS     equ     9       ;flags for mode
GRS_MODE_INFO_SHIFT     equ    10       ;amount to shift bank by
GRS_MODE_INFO_SIZE      equ    12       ;have 12 bytes in structure

GRM_IS_SUPPORTED        equ     1       ;bit 1 of flags
GRM_IS_LINEAR           equ     2       ;bit 2 is for linear frame buffer
GRI_USE_WINA            equ     4       ;bit 3 indicates if winA is read/writable

; structure for bitmaps to be drawn from and to.  if a bitmap is contained
; within a larger bitmap, the row field tells how wide the containing bitmap
; is.
GRS_BITMAP_BITS         equ     0       ;ptr to data
GRS_BITMAP_TYPE         equ     4       ;type of data in bitmap
GRS_BITMAP_ALIGN        equ     5       ;where data really starts
GRS_BITMAP_FLAGS        equ     6       ;whether compressed, transparent, etc
GRS_BITMAP_W            equ     8       ;width in pixels
GRS_BITMAP_H            equ     10      ;height
GRS_BITMAP_ROW          equ     12      ;bytes in row of containing bitmap
GRS_BITMAP_WLOG         equ     14      ;log2 of w
GRS_BITMAP_HLOG         equ     15      ;log2 of h
GRS_BITMAP_SIZE         equ     16

; lrpp list for scanline pixel-based drawing.
GRS_SPAN_L              equ     0       ;left edge of span
GRS_SPAN_R              equ     2       ;right
GRS_SPAN_PP             equ     4       ;pointer to data
GRS_SPAN_N              equ     8       ;next span on this scanline
GRS_SPAN_SIZE           equ     12

; stencil element for non-rectangular clipping. 
GRS_STEN_ELEM_L         equ     0       ;left edge of stencil
GRS_STEN_ELEM_R         equ     2       ;right
GRS_STEN_ELEM_N         equ     4       ;next span in this scanline
GRS_STEN_ELEM_SIZE      equ     8

;  stencil header for non-rectangular clipping. 
GRS_STENCIL_ELEM        equ     0       ;pointer to first stencil element
GRS_STENCIL_FLAGS       equ     4       ;specific stencil data.
GRS_STENCIL_SIZE        equ     8

; structure for clipping regions.  a clipping region can either be a simple
; rectangle (given by left,top,right,bot) or a grs_stencil, pointed to by
; sten. 
GRS_CLIP_STEN           equ     0       ;stencil for nonrect clip region
GRS_CLIP_LEFT           equ     6       ;current clipping rectangle
GRS_CLIP_TOP            equ     10      ;integral coordinates
GRS_CLIP_RIGHT          equ     14
GRS_CLIP_BOT            equ     18
GRS_FCLIP_LEFT          equ     4       ;current clipping rectangle 
GRS_FCLIP_TOP           equ     8       ;fixed-point coordinates 
GRS_FCLIP_RIGHT         equ     12
GRS_FCLIP_BOT           equ     16
GRS_CLIP_SIZE           equ     20

; structure for drawing context.  the context contains data about which
; color, font attributes, filling attributes, and an embedded clipping
; region structure. 
GRS_CONTEXT_FCOLOR      equ     0       ;current drawing color 
GRS_CONTEXT_BCOLOR      equ     4       ;background color 
GRS_CONTEXT_FONT        equ     8       ;font id 
GRS_CONTEXT_TEXT_ATTR   equ     12      ;attributes for text 
GRS_CONTEXT_FILL_TYPE   equ     16      ;how to fill primitives 
GRS_CONTEXT_FILL_PARM   equ     20      ;parameter for fill 
GRS_CONTEXT_CLIP        equ     24      ;clipping region 
GRS_CONTEXT_SAFE_CLIP   equ     44      ;clipping region 
GRS_CONTEXT_SIZE        equ     64

; a canvas is a bitmap that may be drawn to or from, with an attached
; drawing context. 
GRS_CANVAS_BM           equ     0       ;bitmap to draw into/read out of 
GRS_CANVAS_GC           equ     16      ;graphic context 
GRS_CANVAS_YTAB         equ     80      ;y table
GRS_CANVAS_SIZE         equ     84

; a screen is a descriptor for a visible region of video memory. 
; where we actually draw 
GRS_SCREEN_BM           equ     0       ;where we actually draw
GRS_SCREEN_BM2          equ     16      ;where we actually draw
GRS_SCREEN_C            equ     32      ;pointer to 2 system canvases
GRS_SCREEN_PAL          equ     36      ;rgb palette
GRS_SCREEN_BPAL         equ     40      ;bound palette
GRS_SCREEN_IPAL         equ     44      ;inverse palette
GRS_SCREEN_LTAB         equ     48      ;lighting table
GRS_SCREEN_TTAB         equ     52      ;translucency table
GRS_SCREEN_CLUT         equ     56      ;color lookup table
GRS_SCREEN_X            equ     60      ;upper left coordinates of visible
GRS_SCREEN_Y            equ     62      ;region of virtual buffer
GRS_SCREEN_SIZE         equ     64

; driver capability/info structure. 
GRS_DRVCAP_ASPECT       equ     0       ;fixed point aspect ratio w/h
GRS_DRVCAP_W            equ     4       ;screen width 
GRS_DRVCAP_H            equ     6       ;screen height 
GRS_DRVCAP_VBASE        equ     8       ;base video address 
GRS_DRVCAP_VBASE2       equ     12      ;base video address 
GRS_DRVCAP_ROWBYTES     equ     16
GRS_DRVCAP_SIZE         equ     20

; GEM font header structure.
GRS_FONT_ID             equ     0
GRS_FONT_DUMMY1         equ     2
GRS_FONT_MIN            equ     36
GRS_FONT_MAX            equ     38
GRS_FONT_DUMMY2         equ     40
GRS_FONT_COTPTR         equ     72
GRS_FONT_BUF            equ     76
GRS_FONT_W              equ     80
GRS_FONT_H              equ     82
GRS_FONT_OFF_TAB        equ     84

endif   ;ndef grs_inc
