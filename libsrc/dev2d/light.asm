; $Header: x:/prj/tech/libsrc/dev2d/RCS/light.asm 1.2 1997/02/18 14:30:57 KEVIN Exp $

.386
include type.inc
include cseg.inc
include dseg.inc
include thunks.inc
include lightd.inc

assume ds:_DATA
_DATA segment
dither  dd      ?
_DATA ends

_TEXT segment

; takes eax = light, edx = dl, ebx = count
_FUNCDEF light_il, 3
        dec     ebx
        js      punt

        push    ecx
        push    edi

        push    esi
        mov     esi, dither

        neg     ebx
        jz      fixup
        
        mov     edi, ebx

light_loop:
        add     esi, eax
        add     eax, edx

        mov     ecx, esi
        and     esi, 0ffffh

        shr     ecx, 16
        add     esi, eax

        mov     ebx, esi
        and     esi, 0ffffh

        shr     ebx, 16
        add     ecx, ecx

        add     ebx, ebx
        mov     light_buffer_last[edi], cl

        add     eax, edx
        mov     light_buffer_last[edi+1], bl

        add     edi, 2
        js      light_loop

fixup:
        lea     esi, [esi + eax]
        jnz     done

        mov     ecx, esi
        and     esi, 0ffffh

        shr     ecx, 16
        mov     dither, esi

        add     ecx, ecx

        mov     b ds:light_buffer_last, cl

done:
        pop     esi
        pop     edi
        pop     ecx
punt:
        ret

_TEXT ends
end
