;
; $Source: x:/prj/tech/libsrc/g2/RCS/fl8lo2wi.asm $
; $Revision: 1.7 $
; $Author: TOML $
; $Date: 1996/11/04 13:57:15 $
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
include tmapd.inc
include msmacros.inc

assume ds:_DATA
_DATA segment
public _g2d_o2wi_mask_u
public _g2d_o2wi_mask_v
public _g2d_o2wi_shift_v
public _g2d_o2wi_p_src

public _g2d_o2wi_last_mask_u
_g2d_o2wi_last_mask_u   dd      0deadbeefh
public _g2d_o2wi_last_mask_v
_g2d_o2wi_last_mask_v   dd      0deadbeefh
public _g2d_o2wi_last_p_src
_g2d_o2wi_last_p_src    dd      0deadbeefh
public _g2d_o2wi_last_shift_v
_g2d_o2wi_last_shift_v  db      0ffh

public _g2d_ltab_dither0
public _g2d_ltab_dither1

buffer_end      equ     _g2d_tmap_buffer[G2C_TMAP_BUFFER_SIZE]
ltab            equ     _g2d_tmap_info[G2S_TMAP_LTAB]
p_dest          equ     _g2d_tmap_info[G2S_TMAP_P_DEST]
du              equ     _g2d_tmap_info[G2S_TMAP_DUX]
dv              equ     _g2d_tmap_info[G2S_TMAP_DVX]
i_delta         equ     _g2d_tmap_info[G2S_TMAP_DIX]
i               equ     _g2d_tmap_info[G2S_TMAP_I]
drow            equ     _g2d_tmap_info[G2S_TMAP_DROW]

_DATA ends

_TEXT segment
; takes eax=-count, ebx:u, ecx:v
; NOT C callable
public flat8_flat8_opaque_p2_wrap_il_chained_
public _flat8_flat8_opaque_p2_wrap_il_chained
_flat8_flat8_opaque_p2_wrap_il_chained:
flat8_flat8_opaque_p2_wrap_il_chained_:
        mov     edx, ecx                        ;v_temp = v
        mov     esi, ebx                        ;u_temp = u
        mov     edi, dv                         ;get dv
        mov     ebp, du                         ;get du

;eax    count
;ebx    u
;ecx    v
;edx    v_temp
;esi    u_temp
;edi    dv
;ebp    du

pix_loop:
        shr     esi, 16
        and     edx, 0deadbeefh
_g2d_o2wi_mask_v:

        shr     edx, 0ffh
_g2d_o2wi_shift_v:
        and     esi, 0deadbeefh
_g2d_o2wi_mask_u:

        add     ebx, ebp
        add     ecx, edi

        mov     dl, 0deadbeefh[esi+edx]
_g2d_o2wi_p_src:
        mov     esi, ebx

        mov     buffer_end[eax], dl
        mov     edx, ecx

        inc     eax
        jnz     pix_loop

        ret

;eax = -count
;edx = u
;ebx = v
_FUNCDEF flat8_flat8_opaque_p2_wrap_il_chain_from_c, 3
        push    ecx
        push    ebp
        push    esi
        push    edi
        mov     ecx, ebx         ; ecx = v
        mov     ebx, edx         ; ebx = u
        call    flat8_flat8_opaque_p2_wrap_il_chained_
        pop     edi
        pop     esi
        pop     ebp
        pop     ecx
        ret

punt:
        pop     ebp
        pop     esi
        pop     edi
        ret

_FUNCDEF flat8_flat8_lit_opaque_p2_wrap_il, 3
        push    edi
        mov     edi, p_dest

        push    esi
        push    ebp

        lea     esi, [edi+eax]
        mov     ebp, drow

        add     edi, ebp
        sub     eax, edx

        mov     p_dest, edi
        jns     punt

        push    eax                     ;-count
        push    esi                     ;p_dest

        call    flat8_flat8_opaque_p2_wrap_il_chained_
        pop     ebp                     ;p_dest

        pop     esi                     ;-count
        mov     edi, ltab

        inc     esi
        mov     ebx, i_delta

        ror     ebx, 16
        mov     eax, i

        ror     eax, 16
        xor     edx, edx

        mov     dh, al
        test    esi, esi

        jz      one_pix
        add     eax, ebx

        mov     al, [ebp]               ;read cache line

        mov     ecx, 0                  ;don't xor! need to preserve flags
        mov     al, dh

;       eax     i  high 16 = frac; al = int
;       ebx     di high 16 = frac; bl = int
;       ecx     temp
;       edx     temp
;       esi     source
;       edi     ltab
;       ebp     dest

light_loop:
        adc     eax, ebx
        lea     ebp, [ebp+2]

        mov     ch, al
        mov     dl, buffer_end[esi]-1

        adc     eax, ebx
        mov     cl, buffer_end[esi]

        mov     dl, [edi+edx]
        inc     esi

        mov     dh, [edi+ecx]
        inc     esi

        mov     [ebp-2], dl
        mov     [ebp-1], dh

        mov     dh, al
        js      light_loop

        jnz     done
        mov     dh, al

one_pix:
        mov     dl, buffer_end-1
        mov     dl, [edi+edx]
        mov     [ebp], dl

done:
        pop     ebp
        pop     esi
        pop     edi
        ret

_FUNCDEF flat8_flat8_lit_opaque_p2_wrap_dither_il, 3
        push    edi
        mov     edi, p_dest

        push    esi
        push    ebp

        lea     esi, [edi+eax]
        mov     ebp, drow

        add     edi, ebp
        sub     eax, edx

        mov     p_dest, edi
        jns     punt

        push    eax                     ;-count
        push    esi                     ;p_dest

        call    flat8_flat8_opaque_p2_wrap_il_chained_
        pop     edi                     ;p_dest
        pop     esi                     ;-count

        xor     ebp, ebp                ;error
        mov     ebx, i_delta

        mov     eax, i
        inc     esi

        jz      last_pix_hack
;       eax     i
;       ebx     di
;       ecx     temp
;       edx     temp
;       esi     source
;       edi     p_dest
;       ebp     mask

dithered_light_loop:
        add     ebp, eax
        add     eax, ebx

        mov     edx, ebp
        and     ebp, 0ffffh

        shr     edx, 8
        add     ebp, eax

        mov     ecx, ebp
        mov     dl, buffer_end[esi]-1

        shr     ecx, 8
        add     eax, ebx

        mov     dl, 0deadbeefh[edx]
_g2d_ltab_dither0:
        mov     cl, buffer_end[esi]

        and     ebp, 0ffffh
        mov     [edi], dl

        add     esi, 2
read_ltab:
        mov     dh, 0deadbeefh[ecx]
_g2d_ltab_dither1:

        mov     [edi+1], dh
        lea     edi, [edi+2]

        js      dithered_light_loop

        jz      last_pix_hack

        pop     ebp
        pop     esi
        pop     edi
        ret

last_pix_hack:
        mov     ecx, eax
        dec     edi

        shr     ecx, 8
        mov     cl, buffer_end-1

        inc     esi                     ;set zf=0, sf=0
        jmp     read_ltab


_TEXT ends
end

