; $Header: x:/prj/tech/libsrc/dev2d/RCS/f16f16c.asm 1.2 1997/10/09 12:12:43 KEVIN Exp $

.386
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc

assume ds:_DATA
_DATA segment
last    dd      ?
_DATA ends

_TEXT segment

; takes eax = dest, edx = src, ebx = count, ecx = clut
_FUNCDEF flat16_flat16_opaque_clut_il, 4
        test    ebx, ebx
        jz      punt

        push    ebp
        push    esi

        push    edi
        mov     ebp, eax

        mov     esi, edx
        mov     edi, ecx

        xor     eax, eax
        xor     edx, edx

        lea     ecx, [esi + 2*ebx - 2]
        mov     ebx, last               ;save old last, to be interrupt safe

        mov     last, ecx
        push    ebx

        test    ebp, 2                  ;is destination aligned?
        jz      skip_fixup

fixup:
        mov     al, [esi]
        mov     dl, [esi+1]

        add     ebp, 2
        add     esi, 2

        mov     bl, [edi + 2*eax]
        mov     cl, [edi + 2*edx + 512]

        mov     bh, [edi + 2*eax + 1]
        mov     ch, [edi + 2*edx + 513]

        add     ebx, ecx
        mov     ecx, last

        mov     [ebp-2], bx             ;pander to 3dfx. grr.
;        mov     [ebp-2], bl
;        mov     [ebp-1], bh

        cmp     esi, ecx
        ja      done

skip_fixup:
        cmp     esi, ecx                ;are we on the last pixel?
        jz      fixup

        mov     al, [esi+2]
        mov     dl, [esi+3]

        jmp     tl_loop_entry

tl_loop:
        mov     al, [esi+2]
        mov     dl, [esi+3]

        mov     [ebp], ebx
        add     ebp, 4

tl_loop_entry:
        mov     bl, [edi + 2*eax]
        mov     cl, [edi + 2*edx + 512]

        mov     bh, [edi + 2*eax + 1]
        mov     ch, [edi + 2*edx + 513]

        add     ebx, ecx
        xor     ecx, ecx

        mov     al, [esi]
        mov     dl, [esi+1]

        ror     ebx, 16
        add     esi, 4

        mov     bl, [edi + 2*eax]
        mov     cl, [edi + 2*edx + 512]

        mov     bh, [edi + 2*eax + 1]
        mov     ch, [edi + 2*edx + 513]

        add     ebx, ecx
        mov     ecx, last

        cmp     esi, ecx
        jb      tl_loop

        mov     [ebp], ebx
        lea     ebp, [ebp+4]

        jz      fixup

done:
        pop     eax
        pop     edi

        pop     esi
        pop     ebp

        mov     last, eax
punt:
        ret

_TEXT ends
end
