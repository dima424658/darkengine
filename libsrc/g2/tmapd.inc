; $Header: x:/prj/tech/libsrc/g2/RCS/tmapd.inc 1.2 1997/02/18 14:33:30 KEVIN Exp $                                                    
; texture mapping data

ifndef tmapd_inc
tmapd_inc equ 1

extern _g2d_tmap_info:dword

; Keep this synchronized with tmapd.h.  Some 8-to-16-bit paths divide the
; buffer into two scanline-sized work areas, so it must be twice the maximum
; supported destination width.
G2C_TMAP_BUFFER_SIZE equ 16384t
extern _g2d_tmap_buffer:byte
tmap_buffer_end      equ     _g2d_tmap_buffer[G2C_TMAP_BUFFER_SIZE]

endif
