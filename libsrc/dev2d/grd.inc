;
; $Source: s:/prj/tech/libsrc/dev2d/RCS/grd.inc $
; $Revision: 1.2 $
; $Author: KEVIN $
; $Date: 1996/04/11 09:28:28 $
;
; External declarations for globals.
;
; This file is part of the dev2d library.
;

ifndef grd_inc
grd_inc equ 1

externdef _grd_screen:ptr
externdef _grd_screen_row:dword
externdef _grd_screen_bmt:dword
externdef _grd_screen_canvas:ptr
externdef _grd_visible_canvas:ptr
externdef _grd_canvas:ptr
externdef _grd_info:ptr
externdef _grd_bank:word
externdef _grd_grains:word
externdef _grd_cap:ptr
externdef _grd_mode_cap:ptr
externdef _grd_device_table:ptr
externdef _grd_canvas_table:ptr
externdef _grd_pixel_table:ptr
externdef _grd_driver_list:ptr
externdef _grd_mode:dword

endif
