; 
; $Source: s:/prj/tech/libsrc/dev2d/RCS/vgabios.inc $
; $Revision: 1.2 $
; $Author: KEVIN $
; $Date: 1996/04/11 09:28:30 $
;
; Constants for vga bios functions.
;
; This file is part of the dev2d library.
;

VGABIOS_INT     equ     000000010h
VB_BUFFER       equ     000000448h
VB_BUFFER_SIZE  equ     60h
VB_GET_MODE     equ     00000000fh
VB_SET_MODE     equ     000000000h
VB_GET_PALREG   equ     000001007h
VB_GET_COLREG   equ     000001015h
VB_SET_PALREG   equ     000001000h
VB_SET_COLREG   equ     000001010h
VB_GET_COMBO    equ     000001a00h
VB_SET_8x8      equ     000001112h
COMBO_MONO      equ     1t
COMBO_VGA       equ     8t
