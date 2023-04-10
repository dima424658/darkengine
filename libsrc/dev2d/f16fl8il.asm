; $Header: x:/prj/tech/libsrc/dev2d/RCS/f16fl8il.asm 1.3 1996/10/16 16:06:32 TOML Exp $
;
; Inner loops for blitting from 8 bit source to 16 bit destinsation.
;

.386

include cseg.inc
include dseg.inc
include type.inc
include msmacros.inc

assume ds:_DATA
_DATA segment
externdef _pixpal:ptr
_DATA ends

_TEXT segment

f16fl8o_bye:
        ret

;takes  eax=dest
;       edx=src
;       ebx=count

_FUNCDEF flat16_flat8_opaque_inner_loop, 3

        test    ebx,ebx
        jz      f16fl8o_bye

        push    esi
        push    edi

        push    ecx
        mov     ecx, _pixpal

        test    eax, 2                  ;are we destination aligned?
        jz      skip_dst_fixup

        mov     esi, ecx
        xor     ecx, ecx

        mov     cl, [edx]
        inc     edx

        mov     cx, [esi + 2*ecx]

        mov     [eax], cx

        add     eax, 2
        dec     ebx

        jz      f16fl8o_done
        mov     ecx, esi

skip_dst_fixup:
        lea     esi, [edx + ebx]
        lea     edi, [eax + 2*ebx]

        xor     eax, eax
        dec     ebx

        jz      f16fl8o_one
        neg     ebx

        mov     al, [esi + ebx]
        jmp     f16fl8o_loop_entry

f16fl8o_loop:
        mov     dh, [ecx + 2*eax + 1]
        mov     al, [esi + ebx]

        mov     [edi + 2*ebx - 6], edx
f16fl8o_loop_entry:
        mov     dl, [ecx + 2*eax]               ;note AGI

        mov     dh, [ecx + 2*eax + 1]
        mov     al, [esi + ebx - 1]

        ror     edx, 16
        add     ebx, 2

        mov     dl, [ecx + 2*eax]
        jnc     f16fl8o_loop

        mov     dh, [ecx + 2*eax + 1]

        mov     [edi + 2*ebx - 6], edx

        jnz     f16fl8o_done

f16fl8o_one:
        mov     al, [esi - 1]
        mov     ax, [ecx + 2*eax]
        mov     [edi - 2], ax

f16fl8o_done:
        pop     ecx
        pop     edi
        pop     esi

        ret

_TEXT ends
end
