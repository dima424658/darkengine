;
; $Source: x:/prj/tech/libsrc/g2/RCS/fl8nni.asm $
; $Revision: 1.3 $
; $Author: TOML $
; $Date: 1996/10/16 16:07:20 $
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
include msmacros.inc

assume ds:_DATA
_DATA segment
extern _g2d_tmap_info:dword

temp:
        dd ?
ones    dd ?
dest    dd ?

public _g2d_tnni_clut
public _g2d_onni_clut
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
setup:
        mov     ebp, ecx        ; v_fix
        mov     edi, _g2d_tmap_info[G2S_TMAP_P_DEST]
                                ; get p_dest

        sub     edx, eax        ; get length
        add     eax, edi        ; p_dest + x_left

        add     edi, _g2d_tmap_info[G2S_TMAP_DROW]
                                ; p_dest += drow

        mov     _g2d_tmap_info[G2S_TMAP_P_DEST], edi
                                ; save next p_dest
        mov     edi, eax        ; edi = p_dest *

        mov     ecx, edx        ; ecx = length *
        mov     eax, _g2d_tmap_info[G2S_TMAP_BM]

        mov     edx, ebp
        sal     ebp, 16t        ; v_frac

        mov     esi, GRS_BITMAP_BITS[eax]
        mov     ax, GRS_BITMAP_ROW[eax]

        sar     edx, 16t        ; v_int
        and     eax, 0ffffh

        imul    edx

        mov     edx, ebx
        add     esi, eax

        sar     ebx, 16t
        mov     eax, _g2d_tmap_info

        sal     edx, 16         ; u_frac *
        add     esi, ebx        ; p_src *

        mov     ebx, ebp        ; v_frac *
        ret

        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF flat8_flat8_opaque_np2_nowrap_il, 4
        push    esi
        push    edi
        push    ebp

;       takes   eax=x_left, edx=x_right, ebx=u, ecx=v
;       returns ecx = length, edi=p_dst, esi=p_src, edx=u_frac, ebx=v_frac
        call    setup

        add     ebx, _g2d_tmap_info[G2S_TMAP_DV_FRAC]
        sbb     eax, eax

        mov     ebp, _g2d_tmap_info[G2S_TMAP_DU_FRAC]
        test    ecx, ecx

        jz      o_done
o_pix_loop:
        mov     bl,[esi]
        add     edx,ebp

        adc     esi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]
        add     ebx,_g2d_tmap_info[G2S_TMAP_DV_FRAC]

        sbb     eax,eax
        mov     [edi],bl

        inc     edi
        dec     ecx

        jnz     o_pix_loop
o_done:
        pop     ebp
        pop     edi
        pop     esi
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF flat8_flat8_trans_np2_nowrap_il, 4
        push    esi
        push    edi
        push    ebp

;       takes   eax=x_left, edx=x_right, ebx=u, ecx=v
;       returns ecx = length, edi=p_dst, esi=p_src, edx=u_frac, ebx=v_frac
        call    setup

        add     ebx, _g2d_tmap_info[G2S_TMAP_DV_FRAC]
        sbb     eax, eax

        mov     ebp, _g2d_tmap_info[G2S_TMAP_DU_FRAC]
        test    ecx, ecx

        jz      t_done
t_pix_loop:
        mov     bl,[esi]
        add     edx,ebp

        adc     esi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]
        add     ebx,_g2d_tmap_info[G2S_TMAP_DV_FRAC]

        sbb     eax,eax
        test    bl, bl

        jz      t_skip_write
        mov     [edi],bl

t_skip_write:
        inc     edi
        dec     ecx

        jnz     t_pix_loop
t_done:
        pop     ebp
        pop     edi
        pop     esi
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF flat8_flat8_clut_opaque_np2_nowrap_il, 4
        push    esi
        push    edi
        push    ebp

;       takes   eax=x_left, edx=x_right, ebx=u, ecx=v
;       returns ecx = length, edi=p_dst, esi=p_src, edx=u_frac, ebx=v_frac
        call    setup

        test    ecx, ecx
        jz      co_done

        mov     ebp, ebx
        xor     ebx, ebx

        add     ebp, _g2d_tmap_info[G2S_TMAP_DV_FRAC]
        sbb     eax, eax

;eax:v_carry
;edx:u_frac
;ebx:scratch
;ecx:count
;esi:p_src
;edi:p_dest
;ebp:v_frac

co_pix_loop:
        mov     bl,[esi]
        add     edx,_g2d_tmap_info[G2S_TMAP_DU_FRAC]

        adc     esi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]
        add     ebp,_g2d_tmap_info[G2S_TMAP_DV_FRAC]

        sbb     eax,eax
        mov     bl, 0deadbeefh[ebx]
_g2d_onni_clut:

        mov     [edi],bl
        inc     edi

        dec     ecx
        jnz     co_pix_loop
co_done:
        pop     ebp
        pop     edi
        pop     esi
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF flat8_flat8_clut_trans_np2_nowrap_il, 4
        push    esi
        push    edi
        push    ebp

;       takes   eax=x_left, edx=x_right, ebx=u, ecx=v
;       returns ecx = length, edi=p_dst, esi=p_src, edx=u_frac, ebx=v_frac
        call    setup

        test    ecx, ecx
        jz      ct_done

        mov     ebp, ebx
        xor     ebx, ebx

        add     ebp, _g2d_tmap_info[G2S_TMAP_DV_FRAC]
        sbb     eax, eax

;eax:v_carry
;edx:u_frac
;ebx:scratch
;ecx:count
;esi:p_src
;edi:p_dest
;ebp:v_frac

ct_pix_loop:
        mov     bl,[esi]
        add     edx,_g2d_tmap_info[G2S_TMAP_DU_FRAC]

        adc     esi,_g2d_tmap_info[G2S_TMAP_DSRC][4*eax]
        add     ebp,_g2d_tmap_info[G2S_TMAP_DV_FRAC]

        sbb     eax,eax
        test    bl, bl

        jz      ct_skip_write
        mov     bl, 0deadbeefh[ebx]
_g2d_tnni_clut:

        mov     [edi],bl

ct_skip_write:
        inc     edi
        dec     ecx

        jnz     ct_pix_loop
ct_done:
        pop     ebp
        pop     edi
        pop     esi
        ret

_TEXT ends
end
