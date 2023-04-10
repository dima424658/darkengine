; $Header: x:/prj/tech/libsrc/dev2d/RCS/f16set.asm 1.3 1996/10/29 14:53:46 KEVIN Exp $
;
; 16 bit memset.
;

.386

include cseg.inc
include dseg.inc
include type.inc
include msmacros.inc

assume ds:_DATA
_DATA segment
_DATA ends

_TEXT segment

;takes:
;       ax = pix
;       edx = dest
;       ebx = count

_FUNCDEF flat16_memset, 3
        test    ebx,ebx
        jz      ms_punt

        push    edi
        lea     edi, [edx + 2*ebx]

        cmp     ebx,1
        jz      ms_one

        mov     edx, eax
        sub     ebx, 3

        shl     eax, 010h
        and     edx, 0ffffh

        or      eax, edx
        neg     ebx

        jns     ms_2_or_3

ms_loop:
        mov     [edi+2*ebx-6], eax
        mov     [edi+2*ebx-2], eax

        add     ebx, 4
        jnc     ms_loop

        cmp     ebx, 2
        jae     ms_fixup

ms_2_or_3:
        mov     [edi+2*ebx-6],eax
        add     ebx, 2

ms_fixup:
        cmp     ebx, 3
        je      ms_done

ms_one:
        mov     [edi-2],ax
ms_done:
        pop     edi
ms_punt:
        ret

_TEXT ends
end
