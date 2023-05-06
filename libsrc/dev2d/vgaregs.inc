;
; $Source: s:/prj/tech/libsrc/dev2d/RCS/vgaregs.inc $
; $Revision: 1.2 $
; $Author: KEVIN $
; $Date: 1996/04/11 09:28:30 $
; 
; Constants for VGA registers.
;
; This file is part of the dev2d library.
;

VGA_BASE        equ 0000a0000h
GEN_STAT0       equ 0000003c2h
GEN_STAT1       equ 0000003dah
GEN_MSCOUT_R    equ 0000003cch
GEN_MSCOUT_W    equ 0000003c2h
GEN_FEAT_R      equ 0000003cah
GEN_FEAT_W      equ 0000003dah
MR_VSYNC        equ 8t
MR_HSYNC        equ 1t
SRX_ADR         equ 0000003c4h
SRX_DATA        equ 0000003c5h
SR_MAP          equ 2t
SR_MEMORY       equ 4t
SR4_C4          equ 8t
SR4_OE          equ 4t
CRX_ADR         equ 0000003d4h
CRX_DATA        equ 0000003d5h
CR_MAX          equ 000000009h
CR_SAH          equ 00000000ch
CR_SAL          equ 00000000dh
CR_OFFSET       equ 000000013h
CR_ULINE        equ 000000014h
CR14_DWORD      equ 64t
CR_MODE         equ 000000017h
CR17_BYTE       equ 64t
GRX_ADR         equ 0000003ceh
GRX_DATA        equ 0000003cfh
GR_MAP          equ 4t
GR_MODE         equ 5t
GR5_OE          equ 16t
GR_MISC         equ 6t
GR6_CHAIN       equ 2t
GR6_MODE        equ 1t
ARX_ADR         equ 0000003c0h
ARX_WRITE       equ 0000003c0h
ARX_READ        equ 0000003c1h
AR_HPAN         equ 000000013h
AR_PAS          equ 020h
PEL_RADR        equ 0000003c7h
PEL_WADR        equ 0000003c8h
PEL_DATA        equ 0000003c9h
