; $Header: x:/prj/tech/libsrc/g2/RCS/tmapd.inc 1.2 1997/02/18 14:33:30 KEVIN Exp $                                                    
; texture mapping data

ifndef tmapd_inc
tmapd_inc equ 1

extern _g2d_tmap_info:dword

G2C_TMAP_BUFFER_SIZE equ 1024t
extern _g2d_tmap_buffer:byte
tmap_buffer_end      equ     _g2d_tmap_buffer[G2C_TMAP_BUFFER_SIZE]

endif
