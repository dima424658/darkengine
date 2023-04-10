; $Header: x:/prj/tech/libsrc/g2/RCS/n88il.asm 1.1 1997/05/16 09:41:43 KEVIN Exp $
;

.386
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc
include grs.inc
include tmaps.inc
include plyrast.inc
include plyparam.inc

assume ds:_DATA
_DATA segment
extern          _g2d_tmap_info:dword

trans_clut      dd      0deadbeefh
opaque_clut     dd      0deadbeefh
_DATA ends

_TEXT segment

; takes:
; eax = g2s_raster
; edx = g2s_poly_params

; returns:
; ecx = length
; edx = u_frac
; ebx = v_frac
; esi = p_src
; edi = p_dst
; ebp = poly_params
setup:
        mov     ecx, eax
        mov     edi, G2S_POLY_PARAMS_BM[edx]

        mov     ebp, edx
        mov     edx, G2S_RASTER_V[eax]

        xor     eax, eax
        mov     ebx, edx

        mov     ax, GRS_BITMAP_ROW[edi]

        shr     edx, 16
        mov     esi, G2S_RASTER_U[ecx]

        imul    edx             ;eax = v_int x row

        shl     ebx, 16         ;v_frac*
        mov     edx, esi        ;copy u_fix

        shr     esi, 16         ;u_int
        mov     edi, GRS_BITMAP_BITS[edi]

        shl     edx, 16         ;u_frac*
        add     esi, eax        ;u_int + v_int x row

        add     esi, edi        ;p_src*
        mov     edi, G2S_RASTER_P[ecx]

        mov     eax, G2S_RASTER_X[ecx]
        mov     ecx, G2S_RASTER_N[ecx]
                                ;count*
        add     edi, eax        ;p_dst*
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF opaque_nowrap_8to8_il, 2
        push    ebp
        push    edi
        push    esi
        push    ecx
        push    ebx

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
        pop     ebx
        pop     ecx
        pop     esi
        pop     edi
        pop     ebp
        ret

; takes eax=raster edx=params
_FUNCDEF trans_nowrap_8to8_il, 2
        push    ebp
        push    edi
        push    esi
        push    ecx
        push    ebx

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
        pop     ebx
        pop     ecx
        pop     esi
        pop     edi
        pop     ebp
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF opaque_clut_nowrap_8to8_il, 2
        push    ebp
        push    edi
        push    esi
        push    ecx
        push    ebx

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
co_clut:

        mov     [edi],bl
        inc     edi

        dec     ecx
        jnz     co_pix_loop
co_done:
        pop     ebx
        pop     ecx
        pop     esi
        pop     edi
        pop     ebp
coi_done:
        ret

_FUNCDEF opaque_clut_nowrap_8to8_il_init, 1
        cmp     eax, opaque_clut
        jz      coi_done
        mov     opaque_clut, eax
        mov     d ds:[co_clut-4],eax
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF trans_clut_nowrap_8to8_il, 2
        push    ebp
        push    edi
        push    esi
        push    ecx
        push    ebx

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
ct_clut:

        mov     [edi],bl

ct_skip_write:
        inc     edi
        dec     ecx

        jnz     ct_pix_loop
ct_done:
        pop     ebx
        pop     ecx
        pop     esi
        pop     edi
        pop     ebp
cti_done:
        ret

_FUNCDEF trans_clut_nowrap_8to8_il_init, 1
        cmp     eax, trans_clut
        jz      cti_done
        mov     trans_clut, eax
        mov     d ds:[ct_clut-4],eax
        ret

_TEXT ends
end
