;
; $Source: x:/prj/tech/libsrc/g2/RCS/fl8o2wi.asm $
; $Revision: 1.4 $
; $Author: TOML $
; $Date: 1996/10/16 16:07:14 $
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

temp    dd ?
p_dest  dd ?
ones    db ?
eights  db ?
du      equ     _g2d_tmap_info[G2S_TMAP_DUX]
dv      equ     _g2d_tmap_info[G2S_TMAP_DVX]

_DATA ends

_TEXT segment

SD_COUNT        equ 0
SD_P_DST        equ 4
SD_DATA_SIZE    equ 8

subdivide:
;we need to split into multiple lines
; have edi = v
;      ecx = u
;      eax = p_dest
;      edx = count
        sub     esp, SD_DATA_SIZE
        mov     ebp, esp
loop_128:
        mov     SD_COUNT[ebp], edx       ;save count
        mov     SD_P_DST[ebp], eax      ;save p_dst

        mov     esi, ecx                ;save u
        mov     ebx, ecx                ;setup u

        mov     ecx, edi                ;setup v
        mov     edx, 128                ;setup count

        call    internal_loop

        mov     eax, du
        mov     edx, dv

        sal     eax, 7
        sal     edx, 7

        mov     ecx, esi
        add     edi, edx

        add     ecx, eax
        mov     eax, SD_P_DST[ebp]

        mov     edx, SD_COUNT[ebp]
        add     eax, 128

        sub     edx, 128
        cmp     edx, 0ffh

        jg      loop_128
        add     esp, SD_DATA_SIZE
        jmp     spoof

abort:
        pop     ebp
        pop     edi
        pop     esi
        ret

; takes eax=x_left, edx=x_right, ebx:u, ecx:v
_FUNCDEF flat8_flat8_opaque_p2_wrap_il, 4
        push    esi
        push    edi
        push    ebp

        mov     esi, _g2d_tmap_info[G2S_TMAP_P_DEST]
        sub     edx, eax

        add     eax, esi
        mov     ebp, _g2d_tmap_info[G2S_TMAP_DROW]

        add     esi, ebp
        mov     edi, ecx

        mov     _g2d_tmap_info[G2S_TMAP_P_DEST], esi
        mov     ecx, ebx

        cmp     edx,0
        jle     abort

        cmp     edx,0ffh
        jle     spoof

        jmp     subdivide

;eax:    p_dst
;edx:    mask (high 16=v_mask, dh=count, dl=u_mask)
;ebx:    offset into texture
;ecx:    u
;esi:    p_src
;edi:    v
;ebp:    scratch

pix_loop_standalone:
        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi,ebp         ;v += dv
        mov     cl,[esi + ebx]  ;get pixel

        mov     [eax], cl
        inc     eax

        dec     dh
        jnz     pix_loop_standalone

        pop     ebp
        pop     edi
        pop     esi
        ret

; takes eax=p_dest, edx=count, ebx:u, ecx:v (note we require 0<count<256)
internal_loop:
        push    esi
        push    edi
        push    ebp

        mov     edi, ecx         ;edi=v
        mov     ecx, ebx         ;ecx=u

spoof:
; here's some silliness that should be once per poly only
        mov     esi, du
        and     esi, 0ffffff00h
        mov     du,esi
; end silliness
        mov     ebx, edx
        cmp     edx, 10

        mov     esi,_g2d_tmap_info[G2S_TMAP_P_SRC]
        mov     edx,_g2d_tmap_info[G2S_TMAP_MASK]

        mov     dh, bl
        mov     ebx, eax

        jbe     pix_loop_standalone
        neg     ebx

        and     ebx, 3                  ;bl = #pix_start

        sub     dh, bl                  ;dh = remainder

        mov     ones, dh                ;ones&3 = #pix_end
        shr     dh, 3                   ;dh = #pix8

        test    bl, bl
        jz      pix8_loop_init

        mov     eights, dh
        mov     dh, bl

;eax:    p_dest
;edx:    mask (high 16=v_mask, dh=count, dl=u_mask)
;ebx:    offset into texture
;ecx:    u
;esi:    p_src
;edi:    v
;ebp:    scratch

pix_loop_start:
        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi,ebp         ;v += dv
        mov     cl,[esi + ebx]  ;get pixel

        mov     [eax], cl
        inc     eax

        dec     dh
        jnz     pix_loop_start

        mov     dh, eights
pix8_loop_init:
        mov     p_dest, eax

;eax:    pix_accum
;edx:    mask (high 16=v_mask, dh=count, dl=u_mask)
;ebx:    offset into texture
;ecx:    u
;esi:    p_src
;edi:    v
;ebp:    scratch

pix8_loop:
        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi, ebp        ;v += dv
        mov     al, [esi + ebx] ;get pixel
;pix1:                                                   ;6 (6)

        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi, ebp        ;v += dv
        mov     ah, [esi + ebx] ;get pixel
;pix2:                                                   ;6 (12)

        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        shl     eax, 16
        add     edi, ebp        ;v += dv

        mov     al, [esi + ebx] ;get pixel
        mov     ebx, ecx        ;ebx = u_fix
;pix3:                                                   ;7 (19)

        shr     ebx, 23         ;u_int
        mov     ebp, edi        ;ebp = v_fix

        and     ebp, edx        ;mask v
        and     ebx, edx        ;mask u

        shr     ebp, 15         ;v "int"
        nop

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi, ebp        ;v += dv
        mov     ah, [esi + ebx] ;get pixel

        ror     eax,16
        mov     ebp, edi        ;ebp = v_fix
;pix4:                                                   ;7 (26)

        mov     [temp],eax
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi, ebp        ;v += dv
        mov     al, [esi + ebx] ;get pixel
;pix5:                                                   ;6 (32)

        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi, ebp        ;v += dv
        mov     ah, [esi + ebx] ;get pixel
;pix6:                                                   ;6 (38)

        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        shl     eax, 16
        add     edi, ebp        ;v += dv

        mov     al, [esi + ebx] ;get pixel
        mov     ebx, ecx        ;ebx = u_fix

;pix7:                                                   ;7 (45)
        shr     ebx, 23         ;u_int
        mov     ebp, edi        ;ebp = v_fix

        and     ebp, edx        ;mask v
        and     ebx, edx        ;mask u

        shr     ebp, 15         ;v "int"
        nop

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi, ebp        ;v += dv
        mov     ah, [esi + ebx] ;get pixel

        mov     ebx,p_dest
        mov     ebp,[temp]

        ror     eax,16
        dec     dh

        mov     [ebx+4], eax
        mov     [ebx], ebp

        lea     ebx, [ebx+8]
        nop

        mov     p_dest, ebx
        jnz     pix8_loop
;pix8:                                                   ;11 (56)

        mov     dh, ones
        mov     eax, ebx

        and     dh, 7
        jz      done

pix_loop_end:
        mov     ebp, edi        ;ebp = v_fix
        mov     ebx, ecx        ;ebx = u_fix

        shr     ebx, 23         ;u_int
        and     ebp, edx        ;mask v

        shr     ebp, 15         ;v "int"
        and     ebx, edx        ;mask u

        or      ebx, ebp        ;get offset into texture
        mov     ebp, du         ;ebp = delta

        add     ecx, ebp        ;u += du
        mov     ebp, dv         ;ebp = dv

        add     edi,ebp         ;v += dv
        mov     cl,[esi + ebx]  ;get pixel

        mov     [eax], cl
        inc     eax

        dec     dh
        jnz     pix_loop_end

done:
        pop     ebp
        pop     edi
        pop     esi
        ret

_TEXT ends
end

