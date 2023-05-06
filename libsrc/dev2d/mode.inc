;
; $Source: s:/prj/tech/libsrc/dev2d/RCS/mode.inc $
; $Revision: 1.1 $
; $Author: KEVIN $
; $Date: 1996/04/10 16:25:41 $
;
; Symbolic constants for graphics modes.
;
; This file is part of the 2d library.
;

ifndef mode_inc
mode_inc equ 1

;  definitions for all supported device-independent graphics modes. 
GRM_320x200x8           equ     0t
GRM_320x200x8X          equ     1t
GRM_320x400x8           equ     2t
GRM_320x240x8           equ     3t
GRM_320x480x8           equ     4t

GRM_FIRST_SVGA_MODE     equ     5t

; All vesa modes have mode numbers defined by the vendor

GRM_640x350x4           equ     5t
GRM_640x400x4           equ     6t
GRM_640x480x4           equ     7t
GRM_800x600x4           equ     8t
GRM_1024x768x4          equ     9t
GRM_1280x1024x4         equ    10t
GRM_1600x1200x4         equ    11t

GRM_320x200x8S          equ    12t
GRM_320x240x8S          equ    13t
GRM_320x400x8S          equ    14t
GRM_360x200x8           equ    15t
GRM_360x240x8           equ    16t
GRM_360x400x8           equ    17t
GRM_640x350x8           equ    18t
GRM_640x400x8           equ    19t
GRM_640x480x8           equ    20t
GRM_800x600x8           equ    21t
GRM_1024x768x8          equ    22t
GRM_1280x1024x8         equ    23t
GRM_1600x1200x8         equ    24t

GRM_320x200x15          equ    25t
GRM_320x240x15          equ    26t
GRM_320x400x15          equ    27t
GRM_360x200x15          equ    28t
GRM_360x240x15          equ    29t
GRM_360x400x15          equ    30t
GRM_640x350x15          equ    31t
GRM_640x400x15          equ    32t
GRM_640x480x15          equ    33t
GRM_800x600x15          equ    34t
GRM_1024x768x15         equ    35t
GRM_1280x1024x15        equ    36t
GRM_1600x1200x15        equ    37t

GRM_320x200x16          equ    38t
GRM_320x240x16          equ    39t
GRM_320x400x16          equ    40t
GRM_360x200x16          equ    41t
GRM_360x240x16          equ    42t
GRM_360x400x16          equ    43t
GRM_640x350x16          equ    44t
GRM_640x400x16          equ    45t
GRM_640x480x16          equ    46t
GRM_800x600x16          equ    47t
GRM_1024x768x16         equ    48t
GRM_1280x1024x16        equ    49t
GRM_1600x1200x16        equ    50t

GRM_320x200x24          equ    51t
GRM_320x240x24          equ    52t
GRM_320x400x24          equ    53t
GRM_360x200x24          equ    54t
GRM_360x240x24          equ    55t
GRM_360x400x24          equ    56t
GRM_640x350x24          equ    57t
GRM_640x400x24          equ    58t
GRM_640x480x24          equ    59t
GRM_800x600x24          equ    60t
GRM_1024x768x24         equ    61t
GRM_1280x1024x24        equ    62t
GRM_1600x1200x24        equ    63t

GRM_320x200x32          equ    64t
GRM_320x240x32          equ    65t
GRM_320x400x32          equ    66t
GRM_360x200x32          equ    67t
GRM_360x240x32          equ    68t
GRM_360x400x32          equ    69t
GRM_640x350x32          equ    70t
GRM_640x400x32          equ    71t
GRM_640x480x32          equ    72t
GRM_800x600x32          equ    73t
GRM_1024x768x32         equ    74t
GRM_1280x1024x32        equ    75t
GRM_1600x1200x32        equ    76t

GRD_MODES               equ    77t

externdef gr_set_mode_:near
externdef _grd_mode_info:ptr
externdef _grd_mode:dword
endif   ;mode_inc


