; $Header: x:/prj/tech/libsrc/dev2d/RCS/f16f16l.asm 1.1 1997/02/17 14:47:29 KEVIN Exp $

.386
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc
include lightd.inc

assume ds:_DATA
_DATA segment
src     dd      ?
_DATA ends

_TEXT segment

; takes eax = dest, edx = src, ebx = count, ecx = ltab
_FUNCDEF flat16_flat16_opaque_light_il, 4
        dec     ebx
        js      punt

        push    ebp
        push    esi

        push    edi
        mov     edi, ecx

        lea     ecx, [edx + 2*ebx]
        xor     edx, edx

        lea     ebp, [eax + 2*ebx - 4]
        neg     ebx

        mov     src, ecx
        mov     esi, ebx

        test    eax, 2                  ;is destination aligned?

        mov     eax, 0
        jz      skip_fixup

fixup:
        mov     ah, light_buffer_last[esi]
        mov     dh, light_buffer_last[esi]

        mov     al, [ecx + 2*esi]
        mov     dl, [ecx + 2*esi +1]

        mov     bl, [edi + 2*eax]       ; AGI
        mov     cl, [edi + 2*edx + 512]

        mov     bh, [edi + 2*eax + 1]
        mov     ch, [edi + 2*edx + 513]

        add     ebx, ecx
        mov     ecx, src

        mov     [ebp + 2*esi + 4], bl
        mov     [ebp + 2*esi + 5], bh

        inc     esi
        jg      done

skip_fixup:
        test    esi, esi                ;are we on the last pixel?
        jz      fixup

        mov     ah, light_buffer_last[esi+1]
        mov     dh, light_buffer_last[esi+1]

        mov     al, [ecx + 2*esi + 2]
        mov     dl, [ecx + 2*esi + 3]

        jmp     tl_loop_entry

tl_loop:
        mov     bl, [edi + 2*eax]
        mov     cl, [edi + 2*edx + 512]

        mov     bh, [edi + 2*eax + 1]
        mov     ch, [edi + 2*edx + 513]

        add     ebx, ecx
        mov     ecx, src

        mov     ah, light_buffer_last[esi+1]
        mov     dh, light_buffer_last[esi+1]

        mov     al, [ecx + 2*esi + 2]
        mov     dl, [ecx + 2*esi + 3]

        mov     [ebp + 2*esi], ebx


tl_loop_entry:
        mov     bl, [edi + 2*eax]
        mov     cl, [edi + 2*edx + 512]

        mov     bh, [edi + 2*eax + 1]
        mov     ch, [edi + 2*edx + 513]

        add     ebx, ecx
        mov     ecx, src

        mov     ah, light_buffer_last[esi]
        mov     dh, light_buffer_last[esi]

        mov     al, [ecx + 2*esi]
        mov     dl, [ecx + 2*esi + 1]

        ror     ebx, 16
        xor     ecx, ecx

        add     esi, 2
        js      tl_loop

        mov     bl, [edi + 2*eax]
        mov     cl, [edi + 2*edx + 512]

        mov     bh, [edi + 2*eax + 1]
        mov     ch, [edi + 2*edx + 513]

        add     ebx, ecx
        mov     [ebp + 2*esi], ebx

        test    esi, esi
        jnz     done

        mov     ecx, src
        jmp     fixup

done:
        pop     edi
        pop     esi

        pop     ebp
punt:
        ret

_TEXT ends
end
