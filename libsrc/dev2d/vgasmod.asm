;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/vgasmod.asm $
; $Revision: 1.2 $
; $Author: TOML $
; $Date: 1996/10/16 16:06:57 $
;
; Standard VGA mode set routine
;
; This file is part of the 2d library.
;

.386

include type.inc
include dseg.inc
include cseg.inc
include grs.inc
include grd.inc
include bitmap.inc
include dpmi.inc
include mode.inc
include valloc.inc
include vgabios.inc
include vgaregs.inc

_DATA segment
vsm_crtc_reg_data label word
        dw 02011h       ;turn off write protect
        dw 03e07h       ;overflow
        dw 0e312h       ;vertical display end 1e3
        dw 0ea15h       ;vertical blanking start 1ea
        dw 0f010h       ;vertical retrace start 1f0
        dw 02211h       ;vertical retrace end 1f2
        dw 00d16h       ;vertical blanking end 20d
        dw 01306h       ;vertical total 213
VSM_CRTC_REGS equ ($-vsm_crtc_reg_data)/2
externdef  _dpmi_reg_data:ptr
_DATA ends

_TEXT segment
; takes mode in eax, clear flag in edx
; returns error code in eax; 0 is no error.
public vga_set_mode_
vga_set_mode_:
        ; make sure mode is a regular vga mode.
        cmp     eax,GRM_FIRST_SVGA_MODE ; see if call is ok
        jae     vsm_invalid_mode        ; if above or equal, then leave
        xor     dl,1
        push    ebx                     ;[esp+0ch]
        push    ecx                     ;[esp+8]
        push    edx                     ;[esp+4]
        push    edi                     ;[esp]
        ; set base video address & valloc mode.
        mov     _grd_mode_cap+GRS_DRVCAP_VBASE,VGA_BASE
        mov     _grd_valloc_mode,0
        ; set mode 13 to start with.
        mov     ebx,eax
        shl     edx,7
        mov     ax,13h
        push    edi
        mov     edi,p _dpmi_reg_data
        or      eax,edx
        mov     [edi+DPMIS_REG_EAX],ax
        push    ebx
        dpmi_real_interrupt VGABIOS_INT,0,es,edi
        pop     ebx
        pop     edi
        ; if we don't need mode X, we're done.
        cmp     bl,GRM_320x200x8
        je      vsm_set_flat8
        ; frob vga registers for mode X initialization.
        ; turn off sequencer chain4 and odd/even addressing.
        mov     dx,SRX_ADR              ;select seq memory register
        mov     ax,SR_MEMORY or (6 shl 8)
        out     dx,ax
        ; turn on crtc byte mode
        mov     dx,CRX_ADR              ;select crtc mode register
        mov     al,CR_MODE              ;read/modify/store
        out     dx,al
        inc     dx
        in      al,dx
        or      al,040h                 ;set bit 6, byte mode
        out     dx,al
        dec     dx
;        mov     ax,CR_MODE or (0e3h shl 8)
;        out     dx,ax
        ; clear the rest of video memory if necessary.
        mov     eax,[esp+4]             ;get clear flag off stack
        test    al,1
        jnz     vsm_no_clear
        mov     ecx,10000h/4
        xor     eax,eax
        mov     edi,VGA_BASE
        rep     stosd
vsm_no_clear:
        ; turn off crtc dword mode---this actually enables mode X.
        mov     ax,CR_ULINE
        out     dx,ax
        ; bl still has mode number in it.
        cmp     bl,GRM_320x200x8X
        je      vsm_set_modex
        cmp     bl,GRM_320x400x8
        je      vsm_set_scanlines
        ; set 240 scanline mode
        mov     dx,GEN_MSCOUT_W
        mov     al,0e3h
        out     dx,al
        mov     dx,CRX_ADR
        mov     ecx,VSM_CRTC_REGS
        mov     edi,p vsm_crtc_reg_data
vsm_set_crtc_regs:
        mov     ax,[edi]
        out     dx,ax
        add     edi,2
        dec     ecx
        jnz     vsm_set_crtc_regs
        ; if we don't need 480 scanlines, we're done.
        cmp     bl,GRM_320x240x8
        je      vsm_set_modex

vsm_set_scanlines:
        ; The following out doubles the vertical resolution
        mov     ax,CR_MAX or (40h shl 8)
        out     dx,ax
vsm_set_modex:
        mov     _grd_screen_row, 80t
        mov     _grd_screen_bmt, BMT_MODEX

vsm_leave:
        xor     eax,eax
        pop     edi
        pop     edx
        pop     ecx
        pop     ebx
        ret

vsm_invalid_mode:
        xor     eax,eax
        mov     al,1
        ret
vsm_set_flat8:
        mov     _grd_screen_row, 320t
        mov     _grd_screen_bmt, BMT_FLAT8
        jmp     vsm_leave

_TEXT ends
end
