;                                                     
; $Source: x:/prj/tech/libsrc/g2/RCS/fl8o2ni.asm $
; $Revision: 1.2 $
; $Author: TOML $
; $Date: 1996/10/16 16:07:24 $
;
; Inner loop of opaque linear mapping primitive.
;
; This file is part of the g2 library.
;

.386
include type.inc
include cseg.inc
include dseg.inc
include grs.inc
include tmaps.inc

assume ds:_DATA
_DATA segment
extern _g2d_tmap_info:dword

temp:
        dd ?
ones    dd ?
dest    dd ?

public _g2d_tm_wlog
_DATA ends

_TEXT segment

; takes:
; eax = x_left
; edx = x_right
; ebx = u
; ecx = v

; returns:
; ecx = length
; edx = u_frac
; ebx = v_frac
; esi = p_src
; edi = p_dst
g2_flat8_tmap_setup:
        mov     esi, ecx        ; v_fix
        mov     edi, _g2d_tmap_info[G2S_TMAP_P_DEST]
                                ; get p_dest

        sal     ecx, 16         ; v_frac
        sub     edx, eax        ; get length

        sar     esi, 16         ; v_int
        add     eax, edi        ; p_dest + x_left

        sal     esi, 0ffh       ; v_int<<wlog
_g2d_tm_wlog:
        add     edi, _g2d_tmap_info[G2S_TMAP_DROW]
                                ; p_dest += drow

        mov     _g2d_tmap_info[G2S_TMAP_P_DEST], edi
                                ; save next p_dest 
        mov     edi, eax        ; edi = p_dest *

        mov     ebp, ebx        ; u_fix
        sar     ebx, 16         ; u_int

        mov     eax, _g2d_tmap_info[G2S_TMAP_BM]
                                ; bitmap pointer
        sal     ebp, 16         ; u_frac

        add     esi, ebx        ; offset = u_int + v_int<<wlog
        mov     ebx, ecx        ; ebx = v_frac *

        mov     ecx, edx        ; length *
        mov     edx, ebp        ; u_frac *

        add     esi, GRS_BITMAP_BITS[eax]
                                ; p_src = bm->bits + offset *
        ret

o_ones:
        test    ecx, ecx
        jz      o_ones_done
o_ones_loop:
        mov     bl,[esi]
        add     edx,ebp
        adc     esi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]
        add     ebx,_g2d_tmap_info[G2S_TMAP_DV_FRAC]
        sbb     eax,eax
        mov     [edi],bl
        inc     edi
        dec     ecx

        jnz     o_ones_loop
o_ones_done:
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
public flat8_flat8_opaque_p2_nowrap_il_
flat8_flat8_opaque_p2_nowrap_il_:
        push    esi
        push    edi
        push    ebp

;       takes   eax=x_left, edx=x_right, ebx=u, ecx=v
;       returns ecx = length, edi=p_dst, esi=p_src, edx=u_frac, ebx=v_frac
        call    g2_flat8_tmap_setup

        add     ebx, _g2d_tmap_info[G2S_TMAP_DV_FRAC]
        sbb     eax, eax

        cmp     ecx, 7
        jae     o_normal

        mov     ebp, _g2d_tmap_info[G2S_TMAP_DU_FRAC]
        call    o_ones
        pop     ebp
        pop     edi
        pop     esi
        ret

o_normal:
        mov     ebp, edi
        mov     dest, edi

        neg     edi
        add     ebp, ecx

        and     ebp, 3
        and     edi, 3

        mov     ones, ebp
        sub     ecx, ebp

        shr     ecx, 2
        mov     ebp, _g2d_tmap_info[G2S_TMAP_DU_FRAC]

        mov     dl, cl
        mov     ecx, edi

        mov     edi, dest
        call    o_ones

        mov     dest, edi
        mov     ecx, _g2d_tmap_info[G2S_TMAP_DV_FRAC]

;eax:v_carry
;edx:u_frac
;ebx:v_frac
;ecx:dv_frac
;esi:p_src
;edi:scratch
;ebp:du_frac

o_main_loop:
        mov     bl,[esi]                 
        add     edx,ebp                  
        mov     edi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]
        mov     b ds:[temp],bl
        adc     esi,edi
        add     ebx,ecx

        sbb     eax,eax
        mov     bl,[esi]                  
        add     edx,ebp                   
        mov     edi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]
        adc     esi,edi
        add     ebx,ecx

        sbb     eax,eax
        mov     b ds:1[temp],bl
        mov     bl,[esi]                  
        add     edx,ebp                   
        mov     b ds:2[temp],bl
        mov     edi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]

        adc     esi,edi
        add     ebx,ecx
        sbb     eax,eax
        mov     bl,[esi]                  
        add     edx,ebp                   
        mov     edi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]

        adc     esi,edi
        add     ebx,ecx

        mov     b ds:3[temp],bl

        sbb     eax,eax
        mov     edi,dest

        add     edi,4
        mov     ecx,d ds:[temp]
        mov     dest,edi
        dec     dl
        mov     [edi-4],ecx
        mov     ecx, _g2d_tmap_info[G2S_TMAP_DV_FRAC]
        jnz     o_main_loop

        mov     ecx, ones
        call    o_ones

        pop     ebp
        pop     edi
        pop     esi
        ret

_TEXT ends
end
