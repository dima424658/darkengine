; $Header: x:/prj/tech/libsrc/g2/RCS/o88bil.asm 1.1 1997/05/16 09:42:05 KEVIN Exp $

.386
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc

include grs.inc
include plyrast.inc
include plyparam.inc
include tmapd.inc

assume ds:_DATA
_DATA segment
last_mask_u   dd      0deadbeefh
last_mask_v   dd      0deadbeefh
last_p_src    dd      0deadbeefh
last_shift_v  db      0ffh
_DATA ends

_TEXT segment

;takes  eax = g2s_raster *r
;       edx = g2s_poly_param *p

_FUNCDEF opaque_8to8_buffer_il, 2
        push    ebp
        push    esi

        push    edi
        push    ecx
        
        push    ebx
        mov     ebx, G2S_RASTER_U[eax]

        mov     ecx, G2S_RASTER_V[eax]
        mov     eax, G2S_RASTER_N[eax]

        mov     edi, G2S_POLY_PARAMS_DVX[edx]
        mov     ebp, G2S_POLY_PARAMS_DUX[edx]

        mov     edx, ecx
        mov     esi, ebx

        neg     eax

;eax    -count
;ebx    u
;ecx    v
;edx    v_temp
;esi    u_temp
;edi    dv
;ebp    du

pix_loop:
        shr     esi, 16
        and     edx, 0deadbeefh
mask_v:

        shr     edx, 0ffh
shift_v:
        and     esi, 0deadbeefh
mask_u:

        add     ebx, ebp
        add     ecx, edi

        mov     dl, 0deadbeefh[esi+edx]
p_src:
        mov     esi, ebx

        mov     tmap_buffer_end[eax], dl
        mov     edx, ecx

        inc     eax
        jnz     pix_loop

        pop     ebx
        pop     ecx

        pop     edi
        pop     esi

        pop     ebp
        ret

;takes eax = grs_bitmap *bm
_FUNCDEF opaque_8to8_buffer_il_init, 1
        push    esi
        push    edi

        mov     ebx, GRS_BITMAP_BITS[eax]

        mov     edx, GRS_BITMAP_W[eax]  ;width AND height
        mov     al, GRS_BITMAP_WLOG[eax]

        mov     cl, 16
        sub     edx, 010001h            ;convert to masks

        sub     cl, al                  ;v_shift = 16-bm->wlog
        mov     eax, edx

        and     edx, 0ffff0000h         ;v_mask  = fix_make(bm->h-1,0)
        and     eax, 0ffffh             ;u_mask  = bm->w -1

        mov     esi, last_mask_u
        mov     edi, last_mask_v

        cmp     esi, eax
        jz      skip_mask_u

        mov     d ds:[mask_u-4], eax
        mov     last_mask_u, eax
skip_mask_u:
        cmp     edi, edx
        jz      skip_mask_v

        mov     d ds:[mask_v-4], edx
        mov     last_mask_v, edx
skip_mask_v:

        mov     eax, last_p_src
        mov     dl, last_shift_v

        cmp     eax, ebx
        jz      skip_p_src

        mov     d ds:[p_src-4], ebx
        mov     last_p_src, ebx
skip_p_src:
        cmp     dl, cl
        jz      skip_shift_v

        mov     b ds:[shift_v-1], cl
        mov     last_shift_v, cl
skip_shift_v:
        pop     edi
        pop     esi

        ret


_TEXT ends
end
