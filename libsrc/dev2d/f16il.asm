;
; $Source: x:/prj/tech/libsrc/dev2d/RCS/f16il.asm $
; $Revision: 1.6 $
; $Author: TOML $
; $Date: 1996/10/16 16:06:40 $
;
; This file is part of the dev2d library.
;

.386
include type.inc
include cseg.inc
include dseg.inc
include msmacros.inc

assume ds:_DATA
_DATA segment
extern _grd_chroma_key:dword
_DATA ends


; takes eax = source, edx = dest, ebx = count
_TEXT segment

_FUNCDEF flat16_flat16_trans_il, 3
        test    ebx, ebx
        jz      bye

        push    ecx
        test    edx, 2

        jz      skip_dst_fixup
        xor     ecx,ecx

        mov     cx, [eax]

        cmp     ecx, _grd_chroma_key
        je      skip_write

        mov     [edx], cx

skip_write:
        dec     ebx
        jz      popbye

        add     eax, 2
        add     edx, 2

skip_dst_fixup:
        lea     ecx, [2*ebx - 2]

        xor     ebx, ebx
        push    ebp

        push    esi
        push    edi

        lea     esi, [eax + ecx]
        lea     edi, [edx + ecx]

        mov     edx, _grd_chroma_key

        mov     ebp, edx
        neg     ecx

        jz      fixup
        shl     edx, 16

        or      edx, ebp
        jmp     next

opaque:
        mov     [edi + ecx - 4], eax
next:
        mov     eax, [esi + ecx]

        add     ecx, 4
        jc      done2

        cmp     edx, eax
        je      next

        mov     ebx, eax
        and     eax, 0ffffh

        cmp     eax, ebp
        mov     eax, ebx

        jz      trans1
        shr     ebx, 16

        cmp     ebx, ebp
        jnz     opaque

        mov     [edi + ecx - 4], ax
        jmp     next

trans1:
        shr     ebx, 16
        mov     [edi + ecx - 2], bx
        jmp     next

done2:
        cmp     edx, eax
        jz      done0

        mov     ebx, eax
        and     eax, 0ffffh

        cmp     eax, ebp
        mov     eax, ebx

        je      done2_skip
        shr     ebx, 16

        cmp     ebx, ebp
        jnz     opaque_last

        mov     [edi + ecx - 4], ax
        jmp     done0

done2_skip:
        shr     ebx, 16
        mov     [edi + ecx - 2], bx

        jmp     done0
opaque_last:
        mov     [edi + ecx - 4], eax

done0:
        test    ecx, ecx
        jnz     done
                                        ;high 16 of ebx are guaranteed 0 (I think)
fixup:
        mov     bx, [esi]
        cmp     ebx, ebp
        jz      done
        mov     [edi], bx
done:
        pop     edi
        pop     esi
        pop     ebp
popbye:
        pop     ecx
bye:
        ret

_TEXT ends
end
