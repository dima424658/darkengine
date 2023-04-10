;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/vgaasm.asm $
; $Revision: 1.2 $
; $Author: TOML $
; $Date: 1996/10/16 16:07:00 $
;
; VGA specific code.
;
; This file is part of the 2d library.
;

.386

include cseg.inc
include grs.inc
include grd.inc
include vgaregs.inc

_TEXT segment

; The most crashproof piece of code I've ever seen:
public vga_get_focus_
vga_get_focus_:
        ret

; takes (x,y) position of new focus in eax, edx.
public vga_set_focus_
vga_set_focus_:
        push    ebx
        push    ecx
        push    edx
        push    esi
        ; save new (x,y) position of screen.
        mov     ebx,_grd_screen
        mov     [ebx+GRS_SCREEN_X],ax
        mov     [ebx+GRS_SCREEN_Y],dx
        mov     esi,eax
        mov     ebx,_grd_screen_canvas
        ; calculate start address = bits-base + y*row + x/4.
        mov     ecx,[ebx+GRS_CANVAS_BM+GRS_BITMAP_BITS]
        mov     eax,[ebx+GRS_CANVAS_BM+GRS_BITMAP_ROW]
        and     eax,0ffffh
        mul     edx
        add     ecx,eax
        shr     esi,2
        add     ecx,esi
        ;update visible_canvas
        mov     edx,_grd_visible_canvas
        mov     [edx+GRS_CANVAS_BM+GRS_BITMAP_BITS],ecx

        sub     ecx,VGA_BASE

        ; set VGA start address.
        mov     dx,CRX_ADR
        mov     ah,ch
        mov     al,CR_SAH
        out     dx,ax
        mov     ah,cl
        inc     al                      ;mov al,CR_SAL
        out     dx,ax

        pop     esi
        pop     edx
        pop     ecx
        pop     ebx
        ret

; takes start in eax, count in edx, pal_data ebx
public vga_get_pal_
vga_get_pal_:
        ; don't bomb if count is <= 0.
        or      edx,edx
        jle     vgp_punt
        push    ebx
        push    ecx
        push    edx
        lea     ecx,[edx+2*edx]         ; ecx = 3*count
        ; send start to notify pel of upcoming read.
        mov     dx,PEL_RADR
        out     dx,al
        ; read r,g,b values in order from pel data register.
        mov     dx,PEL_DATA
vgp_read_pal:
        in      al,dx
        shl     al,2                    ;fix up 6-bit values for 2d
        mov     [ebx],al
        inc     ebx
        dec     ecx
        jnz     vgp_read_pal
        pop     edx
        pop     ecx
        pop     ebx
vgp_punt:
        ret

; takes start in eax, count in edx, pal_data ebx
public vga_set_pal_
vga_set_pal_:
        ; don't bomb if count is <= 0.
        or      edx,edx
        jle     vsp_punt
        push    ebx
        push    ecx
        push    edx
        lea     ecx,[edx+2*edx]         ; ecx = 3*count
        ; send start to notify pel of upcoming write.
        mov     dx,PEL_WADR
        out     dx,al
        ; send r,g,b values in order to pel data register.
        mov     dx,PEL_DATA
vsp_write_pal:
        mov     al,[ebx]
        shr     al,2                    ;fix up 8-bit values for vga
        out     dx,al
        inc     ebx
        dec     ecx
        jnz     vsp_write_pal
        pop     edx
        pop     ecx
        pop     ebx
vsp_punt:
        ret

; returns 1 if horizontal retrace is in progress.
public vga_stat_htrace_
vga_stat_htrace_:
        push    edx
        xor     eax,eax
        mov     dx,GEN_STAT1
        in      al,dx
        and     al,MR_HSYNC
        pop     edx
        ret

; returns 1 if horizontal retrace is in progress.
public vga_stat_vtrace_
vga_stat_vtrace_:
        push    edx
        xor     eax,eax
        mov     dx,GEN_STAT1
        in      al,dx
        and     al,MR_VSYNC
        shr     al,3
        pop     edx
        ret

; takes width in pixels in eax.
public vga_set_width_
vga_set_width_:
        push    edx
        mov     dx,CRX_ADR
        shl     ax,5                    ;ax=w/8*256
        mov     al,CR_OFFSET
        out     dx,ax
        pop     edx
        ret

_TEXT ends
end

